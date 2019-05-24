/*
 *  MiSTer_fb.c -- MiSTer Frame Reader driver
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
#include <linux/interrupt.h>
#include <linux/of_irq.h>
#include <linux/uaccess.h>

#define PALETTE_SIZE       256
#define DRIVER_NAME        "MiSTer_fb"
#define VSYNC_TIMEOUT_MSEC 50

static u32 width = 0, height = 0, stride = 0, format = 8888, rb = 1, frame_count = 0;
module_param(width,  uint, 0444);
module_param(height, uint, 0444);
module_param(stride, uint, 0444);
module_param(format, uint, 0444);
module_param(rb,     uint, 0444);
module_param(frame_count, uint, 0444);

struct fb_dev {
	struct platform_device *pdev;
	struct fb_info info;
	struct resource *fb_res;
	void __iomem *fb_base;
	int irq;
	u32 pseudo_palette[PALETTE_SIZE];
};

static wait_queue_head_t vs_wait;
static irqreturn_t irq_handler(int irq, void *par)
{
	frame_count++;
	wake_up_interruptible(&vs_wait);
	return IRQ_HANDLED;
}

static int fb_setcolreg(unsigned regno, unsigned red, unsigned green,
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
		((u32 *)info->pseudo_palette)[regno] = rb ? ((red & 255) << 16) | ((green & 255) << 8) | (blue & 255) :
		((blue & 255) << 16) | ((green & 255) << 8) | (red & 255);
	}

	return 0;
}

static int fb_wait_for_vsync(u32 arg)
{
	u32 count;
	int ret;

	if (arg != 0) return -ENODEV;

	count = frame_count;
	ret = wait_event_interruptible_timeout(vs_wait,
				       count != frame_count,
				       msecs_to_jiffies(VSYNC_TIMEOUT_MSEC));

	if (ret == 0)
		return -ETIMEDOUT;

	return 0;
}

static int fb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
	int ret;
	u32 ioarg;

	switch (cmd)
	{
	case FBIO_WAITFORVSYNC:
		if (get_user(ioarg, (u32 __user *)arg))
		{
			ret = -EFAULT;
			break;
		}

		ret = fb_wait_for_vsync(ioarg);
		break;
	default:
		ret = -ENOTTY;
	}

	return ret;
}


static struct fb_ops ops = {
	.owner = THIS_MODULE,
	.fb_fillrect = cfb_fillrect,
	.fb_copyarea = cfb_copyarea,
	.fb_imageblit = cfb_imageblit,
	.fb_setcolreg = fb_setcolreg,
	.fb_ioctl = fb_ioctl,
};

static int setup_fb_info(struct fb_dev *fbdev)
{
	struct fb_info *info = &fbdev->info;

	if(format != 1555) format = 8888;
	if(!width) width = 640;
	if(!height) height = 480;
	if(!stride) stride = (width*4 + 255) & ~255;

	info->var.xres = width;
	info->var.yres = height;
	info->fix.line_length = stride;

	dev_info(&fbdev->pdev->dev, "width = %u, height = %u\n", info->var.xres, info->var.yres);

	strcpy(info->fix.id, DRIVER_NAME);
	info->fix.type = FB_TYPE_PACKED_PIXELS;
	info->fix.visual = FB_VISUAL_TRUECOLOR;
	info->fix.accel = FB_ACCEL_NONE;

	info->fbops = &ops;
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

		info->var.red.offset = 0;
		info->var.green.offset = 5;
		info->var.blue.offset = 10;

		info->var.red.length = 5;
		info->var.green.length = 5;
		info->var.blue.length = 5;
	}
	else
	{
		/* settings for 32bit pixels */
		info->var.bits_per_pixel = 32;

		info->var.red.offset = 8;
		info->var.green.offset = 8;
		info->var.blue.offset = 16;

		info->var.red.length = 8;
		info->var.green.length = 8;
		info->var.blue.length = 8;
	}

	if(rb)
	{
		u32 tmp = info->var.red.offset;
		info->var.red.offset = info->var.blue.offset;
		info->var.blue.offset = tmp;
	}

	info->fix.smem_len = info->fix.line_length * info->var.yres;

	info->pseudo_palette = fbdev->pseudo_palette;
	info->flags = FBINFO_FLAG_DEFAULT;

	return 0;
}

static struct fb_dev *p_fbdev = 0;

static int fb_probe(struct platform_device *pdev)
{
	int retval;
	struct fb_dev *fbdev;

	fbdev = devm_kzalloc(&pdev->dev, sizeof(*fbdev), GFP_KERNEL);
	if (!fbdev)	return -ENOMEM;

	fbdev->pdev = pdev;

	fbdev->fb_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!fbdev->fb_res) return -ENODEV;

	fbdev->fb_base = devm_ioremap_resource(&pdev->dev, fbdev->fb_res);
	if (IS_ERR(fbdev->fb_base))
	{
		dev_err(&pdev->dev, "devm_ioremap_resource fb failed\n");
		return PTR_ERR(fbdev->fb_base);
	}

	retval = setup_fb_info(fbdev);
	fbdev->info.fix.smem_start = fbdev->fb_res->start;
	fbdev->info.screen_base = fbdev->fb_base;

	retval = fb_alloc_cmap(&fbdev->info.cmap, PALETTE_SIZE, 0);
	if (retval < 0) return retval;

	platform_set_drvdata(pdev, fbdev);

	retval = register_framebuffer(&fbdev->info);
	if (retval < 0) goto err_dealloc_cmap;

	dev_info(&pdev->dev, "fb%d: %s frame buffer device at 0x%x+0x%x, virt: 0x%p\n",
		 fbdev->info.node, fbdev->info.fix.id,
		 (unsigned)fbdev->info.fix.smem_start,
		 fbdev->info.fix.smem_len, fbdev->fb_base);

	fbdev->irq = irq_of_parse_and_map(pdev->dev.of_node, 0);
	dev_info(&pdev->dev, "fb%d: IRQ=%d\n", fbdev->info.node, fbdev->irq);

	init_waitqueue_head(&vs_wait);

	if (fbdev->irq == NO_IRQ || request_irq(fbdev->irq, &irq_handler, IRQF_SHARED, "MiSTer_fb", fbdev))
	{
		dev_err(&pdev->dev, "request_irq failed. fb won't use IRQ.\n");
	}

	p_fbdev = fbdev;
	return 0;

err_dealloc_cmap:
	fb_dealloc_cmap(&fbdev->info.cmap);
	return retval;
}

static int fb_remove(struct platform_device *dev)
{
	struct fb_dev *fbdev = platform_get_drvdata(dev);

	if (fbdev)
	{
		unregister_framebuffer(&fbdev->info);
		fb_dealloc_cmap(&fbdev->info.cmap);
		if(fbdev->irq != NO_IRQ) free_irq(fbdev->irq, dev);
	}
	return 0;
}

static int mode_set(const char *val, const struct kernel_param *kp)
{
	if(p_fbdev)
	{
		sscanf(val, "%u %u %u %u %u", &format, &rb, &width, &height, &stride);
		unregister_framebuffer(&p_fbdev->info);
		setup_fb_info(p_fbdev);
		register_framebuffer(&p_fbdev->info);
	}
	return 0;
}

static int mode_get(char *val, const struct kernel_param *kp)
{
	if(p_fbdev)
	{
		sprintf(val, "%u %u %u %u %u", format, rb, width, height, stride);
		return strlen(val);
	}
	return 0;
}

static const struct kernel_param_ops param_ops = {
	.set	= mode_set,
	.get	= mode_get,
};

module_param_cb(mode, &param_ops, NULL, 0664);


static struct of_device_id fb_match[] = {
	{ .compatible = "MiSTer_fb" },
	{},
};
MODULE_DEVICE_TABLE(of, fb_match);

static struct platform_driver fb_driver = {
	.probe = fb_probe,
	.remove = fb_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = DRIVER_NAME,
		.of_match_table = fb_match,
	},
};
module_platform_driver(fb_driver);

MODULE_DESCRIPTION("MiSTer Frame Reader framebuffer driver");
MODULE_AUTHOR("Sorgelig@MiSTer");
MODULE_LICENSE("GPL v2");
