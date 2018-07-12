/*
 * iTekon-usb.h
 *
 *  Created on: 2018年6月3日
 *      Author: grool
 */

#ifndef ITEKON_USB_H_
#define ITEKON_USB_H_
#ifdef __cplusplus
extern "C" {
#endif
//#####################################################################################//
//类型定义
#define USHORT      unsigned short
#define BYTE        unsigned char
#define CHAR        char
#define UINT        unsigned int
#define UCHAR       unsigned char
#define DWORD       unsigned long
#define PVOID       void *
#define ULONG       unsigned long
#define INT         int
#define __stdcall


//错误码定义
#define ERR_CAN_OVERFLOW        0x00000001
#define ERR_CAN_ERRALARM        0x00000002
#define ERR_CAN_PASSIVE         0x00000004
#define ERR_CAN_LOSE            0x00000008
#define ERR_CAN_BUSERR          0x00000010
#define ERR_DEVICEOPENED        0x00000100
#define ERR_DEVICEOPEN          0x00000200
#define ERR_DEVICENOTOPEN       0x00000400
#define ERR_BUFFEROVERFLOW      0x00000800
#define ERR_DEVICENOTEXIST      0x00001000
#define ERR_LOADKERNELDLL       0x00002000
#define ERR_CMDFAILED           0x00004000
#define ERR_BUFFERCREATE        0x00008000
#define ERR_CANETE_PORTOPENED	0x00010000
#define ERR_CANETE_INDEXUSED 	0x00020000

//数据结构定义
typedef struct _VCI_BOARD_INFO {
    USHORT  hw_Version;
    USHORT  fw_Version;
    USHORT  dr_Version;
    USHORT  in_Version;
    USHORT  irq_Num;
    BYTE    can_Num;
    CHAR    str_Serial_Num[20];
    CHAR    str_hw_Type[40];
    USHORT  Reserved[4];
} VCI_BOARD_INFO, *PVCI_BOARD_INFO;

typedef struct _VCI_CAN_OBJ {
    UINT    ID;
    UINT    TimeStamp;
    BYTE    TimeFlag;
    BYTE    SendType;
    BYTE    RemoteFlag;
    BYTE    ExternFlag;
    BYTE    DataLen;
    BYTE    Data[8];
    BYTE    Reserved[3];
} VCI_CAN_OBJ, *PVCI_CAN_OBJ;

typedef struct _VCI_CAN_STATUS {
    UCHAR   ErrInterrupt;
    UCHAR   regMode;
    UCHAR   regStatus;
    UCHAR   regALCapture;
    UCHAR   regECCapture;
    UCHAR   regEWLimit;
    UCHAR   regRECounter;
    UCHAR   regTECounter;
    DWORD   Reserved;
} VCI_CAN_STATUS, *PVCI_CAN_STATUS;

typedef struct _ERR_INFO {
    UINT ErrCode;
    BYTE Passive_ErrData[3];
    BYTE ArLost_ErrData;
} VCI_ERR_INFO, *PVCI_ERR_INFO;

typedef struct _INIT_CONFIG {
    DWORD   AccCode;
    DWORD   AccMask;
    DWORD   Reserved;
    UCHAR   Filter;
    UCHAR   Timing0;
    UCHAR   Timing1;
    UCHAR   Mode;
    UINT   BotRate;
} VCI_INIT_CONFIG, *PVCI_INIT_CONFIG;

#define MODE_NORMAL 0
#define MODE_ONLYLISTEN 1
#define BOT_5K      0x00450257
#define BOT_10K     0x00120257
#define BOT_20K 	0x0012012B
#define BOT_40K     0x00780031
#define BOT_50K 	0x0067002C
#define BOT_80K 	0x004B0018
#define BOT_100K	0x0012003B
#define BOT_125K	0x0012002F
#define BOT_200K	0x0027000E
#define BOT_250K	0x00120017
#define BOT_400K	0x00160008
#define BOT_500K	0x0012000B
#define BOT_666K	0x00240005
#define BOT_800K	0x00240004
#define BOT_1000K	0x00120005

//接口库函数说明
int self_usb_sendmsg_ed2(struct libusb_device_handle * usb_p, unsigned char *write_msg, int write_len);
int self_usb_recvmsg_ed2 (struct libusb_device_handle * usb_p, unsigned char *recv_msg, int recv_max);

DWORD __stdcall VCI_UsbInit();
DWORD __stdcall VCI_UsbExit();
DWORD __stdcall VCI_OpenDevice(DWORD DevType, DWORD DevIndex, DWORD Reserved);
DWORD __stdcall VCI_CloseDevice(DWORD DevType, DWORD DevIndex);
DWORD __stdcall VCI_InitCan(DWORD DevType, DWORD DevIndex, DWORD CANIndex, PVCI_INIT_CONFIG pInitConfig);
DWORD __stdcall VCI_ReadBoardInfo(DWORD DevType, DWORD DevIndex, PVCI_BOARD_INFO pInfo);
DWORD __stdcall VCI_ReadErrInfo(DWORD DevType, DWORD DevIndex, DWORD CANIndex, PVCI_ERR_INFO pErrInfo);
DWORD __stdcall VCI_ReadCanStatus(DWORD DevType, DWORD DevIndex, DWORD CANIndex, PVCI_CAN_STATUS pCANStatus);
DWORD __stdcall VCI_GetReference(DWORD DevType, DWORD DevIndex, DWORD CANIndex, DWORD RefType, PVOID pData);
DWORD __stdcall VCI_SetReference(DWORD DevType, DWORD DevIndex, DWORD CANIndex, DWORD RefType, PVOID pData);
ULONG __stdcall VCI_GetReceiveNum(DWORD DevType, DWORD DevIndex, DWORD CANIndex);
DWORD __stdcall VCI_ClearBuffer(DWORD DevType, DWORD DevIndex, DWORD CANIndex);
DWORD __stdcall VCI_StartCAN(DWORD DevType, DWORD DevIndex, DWORD CANIndex);
DWORD __stdcall VCI_ResetCAN(DWORD DevType, DWORD DevIndex, DWORD CANIndex);
ULONG __stdcall VCI_Transmit(DWORD DevType, DWORD DevIndex, DWORD CANIndex, PVCI_CAN_OBJ pSend, ULONG Len);
ULONG __stdcall VCI_Receive(DWORD DevType, DWORD DevIndex, DWORD CANIndex, PVCI_CAN_OBJ pReceive, ULONG Len, INT WaitTime);

//#####################################################################################//
#ifdef __cplusplus
}
#endif
#endif /* ITEKON_USB_H_ */
