/******************************************************************************
 *
 * Copyright(c) 2016 - 2017 Realtek Corporation.
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
#include <hal_btcoex_wifionly.h>

#if (CONFIG_BTCOEX_SUPPORT_WIFI_ONLY_CFG == 1)

#include "btc/mp_precomp.h"

struct  wifi_only_cfg GLBtCoexist_22bWifiOnly_22b;

void halwifionly_write1byte_22b(PVOID pwifionlyContext, u32 RegAddr, u8 Data)
{
	struct wifi_only_cfg *pwifionlycfg = (struct wifi_only_cfg *)pwifionlyContext;
	PADAPTER		Adapter = pwifionlycfg->Adapter;

	rtw_write8(Adapter, RegAddr, Data);
}

void halwifionly_write2byte_22b(PVOID pwifionlyContext, u32 RegAddr, u16 Data)
{
	struct wifi_only_cfg *pwifionlycfg = (struct wifi_only_cfg *)pwifionlyContext;
	PADAPTER		Adapter = pwifionlycfg->Adapter;

	rtw_write16(Adapter, RegAddr, Data);
}

void halwifionly_write4byte_22b(PVOID pwifionlyContext, u32 RegAddr, u32 Data)
{
	struct wifi_only_cfg *pwifionlycfg = (struct wifi_only_cfg *)pwifionlyContext;
	PADAPTER		Adapter = pwifionlycfg->Adapter;

	rtw_write32(Adapter, RegAddr, Data);
}

u8 halwifionly_read1byte_22b(PVOID pwifionlyContext, u32 RegAddr)
{
	struct wifi_only_cfg *pwifionlycfg = (struct wifi_only_cfg *)pwifionlyContext;
	PADAPTER		Adapter = pwifionlycfg->Adapter;

	return rtw_read8(Adapter, RegAddr);
}

u16 halwifionly_read2byte_22b(PVOID pwifionlyContext, u32 RegAddr)
{
	struct wifi_only_cfg *pwifionlycfg = (struct wifi_only_cfg *)pwifionlyContext;
	PADAPTER		Adapter = pwifionlycfg->Adapter;

	return rtw_read16(Adapter, RegAddr);
}

u32 halwifionly_read4byte_22b(PVOID pwifionlyContext, u32 RegAddr)
{
	struct wifi_only_cfg *pwifionlycfg = (struct wifi_only_cfg *)pwifionlyContext;
	PADAPTER		Adapter = pwifionlycfg->Adapter;

	return rtw_read32(Adapter, RegAddr);
}

void halwifionly_bitmaskwrite1byte_22b(PVOID pwifionlyContext, u32 regAddr, u8 bitMask, u8 data)
{
	u8 originalValue, bitShift = 0;
	u8 i;

	struct wifi_only_cfg *pwifionlycfg = (struct wifi_only_cfg *)pwifionlyContext;
	PADAPTER		Adapter = pwifionlycfg->Adapter;

	if (bitMask != 0xff) {
		originalValue = rtw_read8(Adapter, regAddr);
		for (i = 0; i <= 7; i++) {
			if ((bitMask >> i) & 0x1)
				break;
		}
		bitShift = i;
		data = ((originalValue) & (~bitMask)) | (((data << bitShift)) & bitMask);
	}
	rtw_write8(Adapter, regAddr, data);
}

void halwifionly_phy_set_rf_reg_22b(PVOID pwifionlyContext, enum rf_path eRFPath, u32 RegAddr, u32 BitMask, u32 Data)
{
	struct wifi_only_cfg *pwifionlycfg = (struct wifi_only_cfg *)pwifionlyContext;
	PADAPTER		Adapter = pwifionlycfg->Adapter;

	phy_set_rf_reg(Adapter, eRFPath, RegAddr, BitMask, Data);
}

void halwifionly_phy_set_bb_reg_22b(PVOID pwifionlyContext, u32 RegAddr, u32 BitMask, u32 Data)
{
	struct wifi_only_cfg *pwifionlycfg = (struct wifi_only_cfg *)pwifionlyContext;
	PADAPTER		Adapter = pwifionlycfg->Adapter;

	phy_set_bb_reg(Adapter, RegAddr, BitMask, Data);
}

void hal_btcoex_wifionly_switchband_notify_22b(PADAPTER padapter)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	u8 is_5g = _FALSE;

	if (pHalData->current_band_type == BAND_ON_5G)
		is_5g = _TRUE;

	if (IS_HARDWARE_TYPE_8822B(padapter)) {
#ifdef CONFIG_RTL8822B
		ex_hal8822b_wifi_only_switchbandnotify_22b(&GLBtCoexist_22bWifiOnly_22b, is_5g);
#endif
	}

#ifdef CONFIG_RTL8821C
	else if (IS_HARDWARE_TYPE_8821C(padapter))
		ex_hal8821c_wifi_only_switchbandnotify(&GLBtCoexist_22bWifiOnly_22b, is_5g);
#endif
}

void hal_btcoex_wifionly_scan_notify_22b(PADAPTER padapter)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	u8 is_5g = _FALSE;

	if (pHalData->current_band_type == BAND_ON_5G)
		is_5g = _TRUE;

	if (IS_HARDWARE_TYPE_8822B(padapter)) {
#ifdef CONFIG_RTL8822B
		ex_hal8822b_wifi_only_scannotify_22b(&GLBtCoexist_22bWifiOnly_22b, is_5g);
#endif
	}

#ifdef CONFIG_RTL8821C
	else if (IS_HARDWARE_TYPE_8821C(padapter))
		ex_hal8821c_wifi_only_scannotify(&GLBtCoexist_22bWifiOnly_22b, is_5g);
#endif
}

void hal_btcoex_wifionly_hw_config_22b(PADAPTER padapter)
{
	struct wifi_only_cfg *pwifionlycfg = &GLBtCoexist_22bWifiOnly_22b;

	if (IS_HARDWARE_TYPE_8723B(padapter)) {
#ifdef CONFIG_RTL8723B
		ex_hal8723b_wifi_only_hw_config(pwifionlycfg);
#endif
	}

#ifdef CONFIG_RTL8822B
	else if (IS_HARDWARE_TYPE_8822B(padapter))
		ex_hal8822b_wifi_only_hw_config_22b(pwifionlycfg);
#endif

#ifdef CONFIG_RTL8821C
	else if (IS_HARDWARE_TYPE_8821C(padapter))
		ex_hal8821c_wifi_only_hw_config(pwifionlycfg);
#endif
}

void hal_btcoex_wifionly_initlizevariables_22b(PADAPTER padapter)
{
	struct wifi_only_cfg		*pwifionlycfg = &GLBtCoexist_22bWifiOnly_22b;
	struct wifi_only_haldata	*pwifionly_haldata = &pwifionlycfg->haldata_info;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);

	_rtw_memset_22b(&GLBtCoexist_22bWifiOnly_22b, 0, sizeof(GLBtCoexist_22bWifiOnly_22b));

	pwifionlycfg->Adapter = padapter;

#ifdef CONFIG_PCI_HCI
	pwifionlycfg->chip_interface = WIFIONLY_INTF_PCI;
#elif defined(CONFIG_USB_HCI)
	pwifionlycfg->chip_interface = WIFIONLY_INTF_USB;
#elif defined(CONFIG_SDIO_HCI) || defined(CONFIG_GSPI_HCI)
	pwifionlycfg->chip_interface = WIFIONLY_INTF_SDIO;
#else
	pwifionlycfg->chip_interface = WIFIONLY_INTF_UNKNOWN;
#endif

	pwifionly_haldata->customer_id = CUSTOMER_NORMAL;
}

void hal_btcoex_wifionly_AntInfoSetting(PADAPTER padapter)
{
	struct wifi_only_cfg		*pwifionlycfg = &GLBtCoexist_22bWifiOnly_22b;
	struct wifi_only_haldata	*pwifionly_haldata = &pwifionlycfg->haldata_info;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);

	pwifionly_haldata->efuse_pg_antnum = pHalData->EEPROMBluetoothAntNum;
	pwifionly_haldata->efuse_pg_antpath = pHalData->ant_path;
	pwifionly_haldata->rfe_type = pHalData->rfe_type;
	pwifionly_haldata->ant_div_cfg = pHalData->AntDivCfg;
}

#endif

