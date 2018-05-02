/*
 *  altvipfb.c -- Altera Video and Image Processing(VIP) Frame Reader driver
 *
 *  This is based on a driver made by Thomas Chou <thomas@wytron.com.tw> and
 *  Walter Goossens <waltergoossens@home.nl> This driver supports the Altera VIP
 *  Frame Reader component.  More info on the hardware can be found in
 *  the Altera Video and Image Processing Suite User Guide at this address
 *  http://www.altera.com/literature/ug/ug_vip.pdf.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#include <linux/dma-mapping.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#define PALETTE_SIZE	256
#define DRIVER_NAME	"altvipfb"

struct altvipfb_dev {
	struct platform_device *pdev;
	struct fb_info info;
	struct resource *reg_res;
	void __iomem *base;
	u32 pseudo_palette[PALETTE_SIZE];
};

static int altvipfb_setcolreg(unsigned regno, unsigned red, unsigned green,
			   unsigned blue, unsigned transp, struct fb_info *info)
{
	/*
	 *  Set a single color register. The values supplied have a 32 bit
	 *  magnitude.
	 *  Return != 0 for invalid regno.
	 */

	if (regno > 255)
		return 1;

	red >>= 8;
	green >>= 8;
	blue >>= 8;

	if (regno < 255) {
		((u32 *)info->pseudo_palette)[regno] =
		((red & 255) << 16) | ((green & 255) << 8) | (blue & 255);
	}

	return 0;
}

static struct fb_ops altvipfb_ops = {
	.owner = THIS_MODULE,
	.fb_fillrect = cfb_fillrect,
	.fb_copyarea = cfb_copyarea,
	.fb_imageblit = cfb_imageblit,
	.fb_setcolreg = altvipfb_setcolreg,
};

static u32 width = 0, height = 0;
module_param(width,  uint, 0444);
module_param(height, uint, 0444);

static u32 bgwidth = 0, bgheight = 0;
module_param(bgwidth,  uint, 0444);
module_param(bgheight, uint, 0444);

static u32 aspect = 1;
module_param(aspect, uint, 0444);

static char *video_mode = "0";
module_param(video_mode, charp, 0444);

static u32 video_mode_int = 0;
 
static u32 format = 8888;
module_param(format, uint, 0444);

static u32 bgr = 0;
module_param(bgr, uint, 0444);

static u32 outw = 0, outh = 0;

#define ALTVIPFB_FB32BASE   0x0000
#define ALTVIPFB_FB16BASE   0x0040
#define ALTVIPFB_FB16BASE2  0x0080
#define ALTVIPFB_SWAPBASE   0x0088
#define ALTVIPFB_PLLBASE    0x0100
#define ALTVIPFB_MIXERBASE  0x0200
#define ALTVIPFB_SCALERBASE 0x0400
#define ALTVIPFB_OUTPUTBASE 0x0800

#define SET_REG(regbase,reg,val) writel(val, fbdev->base + (regbase) + ((reg)*4))

static u32 vmodes[][9] =
{
	{ 1280, 110,  40, 220,  720,  5,  5, 20,  74250 },
	{ 1024,  24, 136, 160,  768,  3,  6, 29,  65000 },
	{  720,  16,  62,  60,  480,  9,  6, 30,  27000 },
	{  720,  12,  64,  68,  576,  5,  5, 39,  27000 },
	{ 1280,  48, 112, 248, 1024,  1,  3, 38, 108000 },
	{  800,  40, 128,  88,  600,  1,  4, 23,  40000 },
	{  640,  16,  96,  48,  480, 10,  2, 33,  25175 },
	{ 1280, 440,  40, 220,  720,  5,  5, 20,  74250 },
	{ 1920,  88,  44, 148, 1080,  4,  5, 36, 148500 },
	{ 1920, 528,  44, 148, 1080,  4,  5, 36, 148500 },
	{ 1366,  70, 143, 213,  768,  3,  3, 24,  85500 },
	{ 1024,  40, 104, 144,  600,  1,  3, 18,  48960 },
};
#define VMODES_NUM (sizeof(vmodes) / sizeof(vmodes[0]))

static u32 getPLLdiv(u32 div)
{
	if (div & 1) return 0x20000 | (((div / 2) + 1) << 8) | (div / 2);
	return ((div / 2) << 8) | (div / 2);
}

#define PLL_FREF 100000
static void setPLL(struct altvipfb_dev *fbdev, u32 freq)
{
	u32 fvco, m;
	u64 k;
	u32 c = 1;

	while ((fvco = freq*c) < 500000) c++;

	m = fvco / PLL_FREF;

	k = ((u64)(fvco-(m*PLL_FREF)))<<32;
	do_div(k, PLL_FREF);
	if (!k) k = 1;

	//printk("**** Calculate PLL for %uKHz: Fvco=%uKHz, C=%u, M=%d, K=%llu\n", freq, fvco, c, m, k);

	SET_REG(ALTVIPFB_PLLBASE, 0, 0);
	SET_REG(ALTVIPFB_PLLBASE, 4, getPLLdiv(m));
	SET_REG(ALTVIPFB_PLLBASE, 3, 0x10000);
	SET_REG(ALTVIPFB_PLLBASE, 5, getPLLdiv(c));
	SET_REG(ALTVIPFB_PLLBASE, 9, 2);
	SET_REG(ALTVIPFB_PLLBASE, 8, 7);
	SET_REG(ALTVIPFB_PLLBASE, 7, (u32)k);
	SET_REG(ALTVIPFB_PLLBASE, 2, 0);
}

static void altvipfb_start_hw(struct altvipfb_dev *fbdev)
{
	u32 bgw = bgwidth, bgh = bgheight;
	u32 posx = 0, posy = 0, mixoff = 0;
	u32 *vmode = vmodes[video_mode_int];

	if(bgwidth || bgheight)
	{
		if(bgw <= fbdev->info.var.xres) bgw = fbdev->info.var.xres;
		else posx = (bgw - fbdev->info.var.xres)/2;

		if(bgh <= fbdev->info.var.yres) bgh = fbdev->info.var.yres;
		else posy = (bgh - fbdev->info.var.yres)/2;
	}
	else if(aspect)
	{
		u32 dx = (10000*outw)/fbdev->info.var.xres;
		u32 dy = (10000*outh)/fbdev->info.var.yres;

		u32 d  = (dx<dy) ? dx : dy;
		
		u32 w = (10000*outw)/d;
		u32 h = (10000*outh)/d;

		bgw = (w<fbdev->info.var.xres) ? fbdev->info.var.xres : w;
		posx = (bgw - fbdev->info.var.xres)/2;

		bgh = (h<fbdev->info.var.yres) ? fbdev->info.var.yres : h;
		posy = (bgh - fbdev->info.var.yres)/2;
	}
	else
	{
		bgw = fbdev->info.var.xres;
		bgh = fbdev->info.var.yres;
	}

	setPLL(fbdev, vmode[8]);

	if(format==1555 || format==565)
	{
		mixoff = 5;
		SET_REG(ALTVIPFB_FB16BASE2,  1, (format==565) ? 1 : 0); //Hi-Color format
		SET_REG(ALTVIPFB_FB16BASE2,  0, fbdev->info.fix.smem_start); //Start address
		SET_REG(ALTVIPFB_FB16BASE,   5, (fbdev->info.var.yres & 0x1FFF) | ((fbdev->info.var.xres & 0x1FFF) << 13)); //Frame resolution
		SET_REG(ALTVIPFB_FB16BASE,   6, 0); //Start address (virtual)
		SET_REG(ALTVIPFB_FB16BASE,   0, 1); //Go
	}
	else
	{
		mixoff = 0;
		SET_REG(ALTVIPFB_FB32BASE,   5, (fbdev->info.var.yres & 0x1FFF) | ((fbdev->info.var.xres & 0x1FFF) << 13)); //Frame resolution
		SET_REG(ALTVIPFB_FB32BASE,   6, fbdev->info.fix.smem_start); //Start address
		SET_REG(ALTVIPFB_FB32BASE,   0, 1); //Go
	}

	SET_REG(ALTVIPFB_SWAPBASE,    0, (bgr) ? 1 : 0); //RGB/BGR

	SET_REG(ALTVIPFB_OUTPUTBASE,  4, 0); //Bank
	SET_REG(ALTVIPFB_OUTPUTBASE, 30, 0); //Valid
	SET_REG(ALTVIPFB_OUTPUTBASE,  5, 0); //Progressive/Interlaced
	SET_REG(ALTVIPFB_OUTPUTBASE,  6, vmode[0]); //Active pixel count
	SET_REG(ALTVIPFB_OUTPUTBASE,  7, vmode[4]); //Active line count
	SET_REG(ALTVIPFB_OUTPUTBASE,  9, vmode[1]); //Horizontal Front Porch
	SET_REG(ALTVIPFB_OUTPUTBASE, 10, vmode[2]); //Horizontal Sync Length
	SET_REG(ALTVIPFB_OUTPUTBASE, 11, vmode[1]+vmode[2]+vmode[3]); //Horizontal Blanking (HFP+HBP+HSync)
	SET_REG(ALTVIPFB_OUTPUTBASE, 12, vmode[5]); //Vertical Front Porch
	SET_REG(ALTVIPFB_OUTPUTBASE, 13, vmode[6]); //Vertical Sync Length
	SET_REG(ALTVIPFB_OUTPUTBASE, 14, vmode[5]+vmode[6]+vmode[7]); //Vertical blanking (VFP+VBP+VSync)
	SET_REG(ALTVIPFB_OUTPUTBASE, 30, 1); //Valid
	SET_REG(ALTVIPFB_OUTPUTBASE, 00, 1); //Go

	SET_REG(ALTVIPFB_MIXERBASE,   3, bgw); //Bkg Width
	SET_REG(ALTVIPFB_MIXERBASE,   4, bgh); //Bkg Height
	SET_REG(ALTVIPFB_MIXERBASE,   8+mixoff, posx); //Pos X
	SET_REG(ALTVIPFB_MIXERBASE,   9+mixoff, posy); //Pos Y
	SET_REG(ALTVIPFB_MIXERBASE,  10+mixoff, 1); //Enable Video 0/1
	SET_REG(ALTVIPFB_MIXERBASE,   0, 1); //Go

	SET_REG(ALTVIPFB_SCALERBASE,  3, vmode[0]); //Output Width
	SET_REG(ALTVIPFB_SCALERBASE,  4, vmode[4]); //Output Height
	SET_REG(ALTVIPFB_SCALERBASE,  0, 1); //Go
}

static void altvipfb_disable_hw(struct altvipfb_dev *fbdev)
{
	/* set the control register to 0 to stop streaming */
	SET_REG(ALTVIPFB_FB16BASE, 0, 0);
	SET_REG(ALTVIPFB_FB32BASE, 0, 0);
}

static void parse_vmode(struct altvipfb_dev *fbdev)
{
	u32 mode[16];
	int cnt;

	video_mode_int = 0;

	if(!video_mode || !strlen(video_mode))
	{
		dev_info(&fbdev->pdev->dev, "empty video_mode given. Assume video_mode=0\n");
		return;
	}
	
	dev_info(&fbdev->pdev->dev, "video_mode parameter: ""%s""\n", video_mode);
	cnt = sscanf(video_mode, "%u,%u,%u,%u,%u,%u,%u,%u,%u", &mode[0], &mode[1], &mode[2], &mode[3], &mode[4], &mode[5], &mode[6], &mode[7], &mode[8]);

	if(cnt == 1 && mode[0]<VMODES_NUM)
	{
		video_mode_int = mode[0];
		return;
	}

	if(cnt != 9)
	{
		dev_err(&fbdev->pdev->dev, "invalid format or number of values in video_mode parameter: ""%s""\n", video_mode);
		return;
	}

	for(cnt=0; cnt<9; cnt++) vmodes[0][cnt] = mode[cnt];
}

static int altvipfb_setup_fb_info(struct altvipfb_dev *fbdev)
{
	struct fb_info *info = &fbdev->info;

	outw = vmodes[video_mode_int][0];
	outh = vmodes[video_mode_int][4];

	if(!width) width = outw;
	info->var.xres = width;

	if(!height) height = outh;
	info->var.yres = height;

	dev_info(&fbdev->pdev->dev, "native width = %u, native height = %u\n", outw, outh);
	dev_info(&fbdev->pdev->dev, "used width   = %u, used height   = %u\n", width, height);

	strcpy(info->fix.id, DRIVER_NAME);
	info->fix.type = FB_TYPE_PACKED_PIXELS;
	info->fix.visual = FB_VISUAL_TRUECOLOR;
	info->fix.accel = FB_ACCEL_NONE;

	info->fbops = &altvipfb_ops;
	info->var.activate = FB_ACTIVATE_NOW;
	info->var.height = -1;
	info->var.width = -1;
	info->var.vmode = FB_VMODE_NONINTERLACED;

	info->var.xres_virtual = info->var.xres,
	info->var.yres_virtual = info->var.yres;

	info->var.red.msb_right = 0;
	info->var.green.msb_right = 0;
	info->var.blue.msb_right = 0;

	if(format==1555)
	{
		/* settings for 16bit pixels */
		info->var.bits_per_pixel = 16;

		info->var.red.offset = 10;
		info->var.green.offset = 5;
		info->var.blue.offset = 0;

		info->var.red.length = 5;
		info->var.green.length = 5;
		info->var.blue.length = 5;
	}
	else if(format==565)
	{
		/* settings for 16bit pixels */
		info->var.bits_per_pixel = 16;

		info->var.red.offset = 11;
		info->var.green.offset = 5;
		info->var.blue.offset = 0;

		info->var.red.length = 5;
		info->var.green.length = 6;
		info->var.blue.length = 5;
	}
	else
	{
		/* settings for 32bit pixels */
		info->var.bits_per_pixel = 32;

		info->var.red.offset = 16;
		info->var.green.offset = 8;
		info->var.blue.offset = 0;

		info->var.red.length = 8;
		info->var.green.length = 8;
		info->var.blue.length = 8;
	}

	if(bgr)
	{
		u32 tmp;
		tmp = info->var.red.offset;
		info->var.red.offset = info->var.blue.offset;
		info->var.blue.offset = tmp;
	}

	info->fix.line_length = (info->var.xres * (info->var.bits_per_pixel >> 3));
	info->fix.smem_len = info->fix.line_length * info->var.yres;

	info->pseudo_palette = fbdev->pseudo_palette;
	info->flags = FBINFO_FLAG_DEFAULT;

	return 0;
}

static int altvipfb_probe(struct platform_device *pdev)
{
	int retval;
	void *fbmem_virt;
	struct altvipfb_dev *fbdev;

	fbdev = devm_kzalloc(&pdev->dev, sizeof(*fbdev), GFP_KERNEL);
	if (!fbdev)
		return -ENOMEM;

	fbdev->pdev = pdev;
	fbdev->reg_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!fbdev->reg_res)
		return -ENODEV;

	fbdev->base = devm_ioremap_resource(&pdev->dev, fbdev->reg_res);
	if (IS_ERR(fbdev->base)) {
		dev_err(&pdev->dev, "devm_ioremap_resource failed\n");
		return PTR_ERR(fbdev->base);
	}

	parse_vmode(fbdev);
	retval = altvipfb_setup_fb_info(fbdev);

	fbmem_virt = dma_alloc_coherent(NULL,
					fbdev->info.fix.smem_len,
					(void *)&(fbdev->info.fix.smem_start),
					GFP_KERNEL);
	if (!fbmem_virt) {
		dev_err(&pdev->dev,
			"altvipfb: unable to allocate %d Bytes fb memory\n",
			fbdev->info.fix.smem_len);
		return retval;
	}

	fbdev->info.screen_base = fbmem_virt;

	retval = fb_alloc_cmap(&fbdev->info.cmap, PALETTE_SIZE, 0);
	if (retval < 0)
		goto err_dma_free;

	platform_set_drvdata(pdev, fbdev);

	altvipfb_start_hw(fbdev);

	retval = register_framebuffer(&fbdev->info);
	if (retval < 0)
		goto err_dealloc_cmap;

	dev_info(&pdev->dev, "fb%d: %s frame buffer device at 0x%x+0x%x\n",
		 fbdev->info.node, fbdev->info.fix.id,
		 (unsigned)fbdev->info.fix.smem_start,
		 fbdev->info.fix.smem_len);

	return 0;

err_dealloc_cmap:
	fb_dealloc_cmap(&fbdev->info.cmap);
err_dma_free:
	dma_free_coherent(NULL, fbdev->info.fix.smem_len, fbmem_virt,
			  fbdev->info.fix.smem_start);
	return retval;
}

static int altvipfb_remove(struct platform_device *dev)
{
	struct altvipfb_dev *fbdev = platform_get_drvdata(dev);

	if (fbdev) {
		unregister_framebuffer(&fbdev->info);
		fb_dealloc_cmap(&fbdev->info.cmap);
		dma_free_coherent(NULL, fbdev->info.fix.smem_len,
				  fbdev->info.screen_base,
				  fbdev->info.fix.smem_start);
		altvipfb_disable_hw(fbdev);
	}
	return 0;
}


static struct of_device_id altvipfb_match[] = {
	{ .compatible = "MiSTer,VIP" },
	{},
};
MODULE_DEVICE_TABLE(of, altvipfb_match);

static struct platform_driver altvipfb_driver = {
	.probe = altvipfb_probe,
	.remove = altvipfb_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = DRIVER_NAME,
		.of_match_table = altvipfb_match,
	},
};
module_platform_driver(altvipfb_driver);

MODULE_DESCRIPTION("Altera VIP Frame Reader framebuffer driver (MiSTer)");
MODULE_AUTHOR("Chris Rauer <crauer@altera.com>, Sorgelig@MiSTer");
MODULE_LICENSE("GPL v2");
