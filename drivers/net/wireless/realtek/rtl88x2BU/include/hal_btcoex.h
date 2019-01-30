/******************************************************************************
 *
 * Copyright(c) 2013 - 2017 Realtek Corporation.
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
#ifndef __HAL_BTCOEX_H__
#define __HAL_BTCOEX_H__

#include <drv_types.h>

/* Some variables can't get from outsrc BT-Coex,
 * so we need to save here */
typedef struct _BT_COEXIST {
	u8 bBtExist;
	u8 btTotalAntNum;
	u8 btChipType;
	u8 bInitlized;
	u8 btAntisolation;
} BT_COEXIST, *PBT_COEXIST;

void DBG_BT_INFO_22b(u8 *dbgmsg);

void hal_btcoex_SetBTCoexist_22b(PADAPTER padapter, u8 bBtExist);
u8 hal_btcoex_IsBtExist_22b(PADAPTER padapter);
u8 hal_btcoex_IsBtDisabled_22b(PADAPTER);
void hal_btcoex_SetChipType_22b(PADAPTER padapter, u8 chipType);
void hal_btcoex_SetPgAntNum_22b(PADAPTER padapter, u8 antNum);

u8 hal_btcoex_Initialize_22b(PADAPTER padapter);
void hal_btcoex_PowerOnSetting_22b(PADAPTER padapter);
void hal_btcoex_AntInfoSetting(PADAPTER padapter);
void hal_btcoex_PowerOffSetting(PADAPTER padapter);
void hal_btcoex_PreLoadFirmware_22b(PADAPTER padapter);
void hal_btcoex_InitHwConfig_22b(PADAPTER padapter, u8 bWifiOnly);

void hal_btcoex_IpsNotify_22b(PADAPTER padapter, u8 type);
void hal_btcoex_LpsNotify_22b(PADAPTER padapter, u8 type);
void hal_btcoex_ScanNotify_22b(PADAPTER padapter, u8 type);
void hal_btcoex_ConnectNotify_22b(PADAPTER padapter, u8 action);
void hal_btcoex_MediaStatusNotify_22b(PADAPTER padapter, u8 mediaStatus);
void hal_btcoex_SpecialPacketNotify_22b(PADAPTER padapter, u8 pktType);
void hal_btcoex_IQKNotify_22b(PADAPTER padapter, u8 state);
void hal_btcoex_BtInfoNotify_22b(PADAPTER padapter, u8 length, u8 *tmpBuf);
void hal_btcoex_BtMpRptNotify(PADAPTER padapter, u8 length, u8 *tmpBuf);
void hal_btcoex_SuspendNotify_22b(PADAPTER padapter, u8 state);
void hal_btcoex_HaltNotify_22b(PADAPTER padapter, u8 do_halt);
void hal_btcoex_SwitchBtTRxMask_22b(PADAPTER padapter);

void hal_btcoex_Hanlder_22b(PADAPTER padapter);

s32 hal_btcoex_IsBTCoexRejectAMPDU_22b(PADAPTER padapter);
s32 hal_btcoex_IsBTCoexCtrlAMPDUSize_22b(PADAPTER padapter);
u32 hal_btcoex_GetAMPDUSize_22b(PADAPTER padapter);
void hal_btcoex_SetManualControl_22b(PADAPTER padapter, u8 bmanual);
u8 hal_btcoex_1Ant_22b(PADAPTER padapter);
u8 hal_btcoex_IsBtControlLps_22b(PADAPTER);
u8 hal_btcoex_IsLpsOn_22b(PADAPTER);
u8 hal_btcoex_RpwmVal_22b(PADAPTER);
u8 hal_btcoex_LpsVal_22b(PADAPTER);
u32 hal_btcoex_GetRaMask_22b(PADAPTER);
void hal_btcoex_RecordPwrMode_22b(PADAPTER padapter, u8 *pCmdBuf, u8 cmdLen);
void hal_btcoex_DisplayBtCoexInfo_22b(PADAPTER, u8 *pbuf, u32 bufsize);
void hal_btcoex_SetDBG_22b(PADAPTER, u32 *pDbgModule);
u32 hal_btcoex_GetDBG_22b(PADAPTER, u8 *pStrBuf, u32 bufSize);
u8 hal_btcoex_IncreaseScanDeviceNum_22b(PADAPTER);
u8 hal_btcoex_IsBtLinkExist_22b(PADAPTER);
void hal_btcoex_SetBtPatchVersion_22b(PADAPTER, u16 btHciVer, u16 btPatchVer);
void hal_btcoex_SetHciVersion_22b(PADAPTER, u16 hciVersion);
void hal_btcoex_SendScanNotify(PADAPTER, u8 type);
void hal_btcoex_StackUpdateProfileInfo_22b(void);
void hal_btcoex_pta_off_on_notify(PADAPTER padapter, u8 bBTON);
void hal_btcoex_SetAntIsolationType_22b(PADAPTER padapter, u8 anttype);
#ifdef CONFIG_LOAD_PHY_PARA_FROM_FILE
	int hal_btcoex_AntIsolationConfig_ParaFile_22b(IN PADAPTER	Adapter, IN char *pFileName);
	int hal_btcoex_ParseAntIsolationConfigFile_22b(PADAPTER Adapter, char	*buffer);
#endif /* CONFIG_LOAD_PHY_PARA_FROM_FILE */
u16 hal_btcoex_btreg_read(PADAPTER padapter, u8 type, u16 addr, u32 *data);
u16 hal_btcoex_btreg_write(PADAPTER padapter, u8 type, u16 addr, u16 val);
void hal_btcoex_set_rfe_type(u8 type);
void hal_btcoex_switchband_notify(u8 under_scan, u8 band_type);
void hal_btcoex_WlFwDbgInfoNotify(PADAPTER padapter, u8* tmpBuf, u8 length);
void hal_btcoex_rx_rate_change_notify(PADAPTER padapter, u8 is_data_frame, u8 rate_id);

#ifdef CONFIG_RF4CE_COEXIST
void hal_btcoex_set_rf4ce_link_state(u8 state);
u8 hal_btcoex_get_rf4ce_link_state(void);
#endif
#endif /* !__HAL_BTCOEX_H__ */
