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

struct altvipfb_type;

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
module_param(width,  uint, 0644);
module_param(height, uint, 0644);

static u32 bgwidth = 0, bgheight = 0;
module_param(bgwidth,  uint, 0644);
module_param(bgheight, uint, 0644);

#define ALTVIPFB_FBBASE     0x0000
#define ALTVIPFB_MIXERBASE  0x0200
#define SET_REG(regbase,reg,val) writel(val, fbdev->base + (regbase) + ((reg)*4))

static void altvipfb_start_hw(struct altvipfb_dev *fbdev)
{
	u32 bgw = bgwidth, bgh = bgheight;
	u32 posx = 0, posy = 0;

	if(bgw <= fbdev->info.var.xres) bgw = fbdev->info.var.xres;
	else posx = (bgw - fbdev->info.var.xres)/2;

	if(bgh <= fbdev->info.var.yres) bgh = fbdev->info.var.yres;
	else posy = (bgh - fbdev->info.var.yres)/2;

	SET_REG(ALTVIPFB_FBBASE,     5, (fbdev->info.var.yres & 0x1FFF) | ((fbdev->info.var.xres & 0x1FFF) << 13)); //Frame resolution
	SET_REG(ALTVIPFB_FBBASE,     6, fbdev->info.fix.smem_start); //Start address
	SET_REG(ALTVIPFB_FBBASE,     0, 1); //Go

	SET_REG(ALTVIPFB_MIXERBASE,  3, bgw); //Bkg Width
	SET_REG(ALTVIPFB_MIXERBASE,  4, bgh); //Bkg Height
	SET_REG(ALTVIPFB_MIXERBASE,  8, posx); //Pos X
	SET_REG(ALTVIPFB_MIXERBASE,  9, posy); //Pos Y
	SET_REG(ALTVIPFB_MIXERBASE, 10, 1); //Enable Video 0
	SET_REG(ALTVIPFB_MIXERBASE,  0, 1); //Go
}

static void altvipfb_disable_hw(struct altvipfb_dev *fbdev)
{
	/* set the control register to 0 to stop streaming */
	SET_REG(ALTVIPFB_FBBASE, 0, 0);
}

static int altvipfb_setup_fb_info(struct altvipfb_dev *fbdev)
{
	struct fb_info *info = &fbdev->info;

	u32 w = readl(fbdev->base + 0x80);
	u32 h = readl(fbdev->base + 0x88);

	w = (((w>>12)&0xf)*1000) + (((w>>8)&0xf)*100) + (((w>>4)&0xf)*10) + (w&0xf);
	h = (((h>>12)&0xf)*1000) + (((h>>8)&0xf)*100) + (((h>>4)&0xf)*10) + (h&0xf);

	dev_info(&fbdev->pdev->dev, "native width = %u, native height = %u\n", w, h);
	dev_info(&fbdev->pdev->dev, "kparam width = %u, kparam height = %u\n", width, height);

	info->var.xres = width  ? width  : w;
	info->var.yres = height ? height : h;

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
	info->var.bits_per_pixel = 32;

	/* settings for 32bit pixels */
	info->var.red.offset = 16;
	info->var.red.length = 8;
	info->var.red.msb_right = 0;
	info->var.green.offset = 8;
	info->var.green.length = 8;
	info->var.green.msb_right = 0;
	info->var.blue.offset = 0;
	info->var.blue.length = 8;
	info->var.blue.msb_right = 0;

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
	{ .compatible = "altr,vip-frame-reader-1.0" },
	{ .compatible = "altr,vip-frame-reader-9.1" },
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

MODULE_DESCRIPTION("Altera VIP Frame Reader framebuffer driver");
MODULE_AUTHOR("Chris Rauer <crauer@altera.com>");
MODULE_LICENSE("GPL v2");
