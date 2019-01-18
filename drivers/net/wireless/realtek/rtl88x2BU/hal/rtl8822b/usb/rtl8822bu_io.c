/******************************************************************************
 *
 * Copyright(c) 2015 - 2017 Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 *****************************************************************************/
#define _RTL8822BU_IO_C_

#include <drv_types.h>		/* PADAPTER and etc. */

void rtl8822bu_set_intf_ops(struct _io_ops *pops)
{

	_rtw_memset_22b((u8 *)pops, 0, sizeof(struct _io_ops));

	pops->_read8 = &usb_read8_22b;
	pops->_read16 = &usb_read16_22b;
	pops->_read32 = &usb_read32_22b;
	pops->_read_mem = &usb_read_mem_22b;
	pops->_read_port = &usb_read_port_22b;

	pops->_write8 = &usb_write8_22b;
	pops->_write16 = &usb_write16_22b;
	pops->_write32 = &usb_write32_22b;
	pops->_writeN = &usb_writeN_22b;

#ifdef CONFIG_USB_SUPPORT_ASYNC_VDN_REQ
	pops->_write8_async = &usb_async_write8;
	pops->_write16_async = &usb_async_write16;
	pops->_write32_async = &usb_async_write32;
#endif
	pops->_write_mem = &usb_write_mem_22b;
	pops->_write_port = &usb_write_port_22b;

	pops->_read_port_cancel = &usb_read_port_22b_cancel_22b;
	pops->_write_port_cancel = &usb_write_port_22b_cancel;

#ifdef CONFIG_USB_INTERRUPT_IN_PIPE
	pops->_read_interrupt = &usb_read_interrupt;
#endif


}
