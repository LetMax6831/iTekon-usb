/*
 * iTekon-usb.c
 *
 *  Created on: 2018年6月3日
 *      Author: grool
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include "libusb.h"
#include "iTekon-usb.h"
#include "rbuf.h"
#include "sm4.h"

#define HTONL(v) ( ((v) << 24) | (((v) >> 24) & 255) | (((v) << 8) & 0xff0000) | (((v) >> 8) & 0xff00) )
#define HTONS(v) ( (((v) << 8) & 0xff00) | (((v) >> 8) & 255) )

//static struct libusb_device_handle *usb_p=NULL;
libusb_context *contex = NULL;

//rbuf_t *rbuf_handle[2];
pthread_t recvthread_handle[2];

typedef struct usbinfo {
    struct libusb_device_handle *usb_p;
    rbuf_t *rbuf_handle[2];
    int use_flag;
    int error;
}USB_INFO_T;

USB_INFO_T usbinfo_handle[2];

int usbinit_flag = 0;
unsigned int error = 0;
int exit_flag = 0;

int self_readusb_info_ed1(struct libusb_device_handle * usb_p, unsigned char *write_msg, int write_len, unsigned char *read_msg, int read_max)
{
    if (usb_p == NULL) {
        return -1;
    }
    int recv_len = 0;
    int ret = 0;
    ret = libusb_interrupt_transfer (usb_p, 0x1, write_msg, write_len, &recv_len, 1000);
    if (ret < 0) {
        printf("LINE:%d\tret %s recv %d\n", __LINE__, libusb_strerror(ret), recv_len);
        goto end;
    }
    ret = libusb_interrupt_transfer (usb_p, 0x81, read_msg, read_max, &recv_len, 1000);
    if (ret < 0) {
        printf("LINE:%d\tret %s recv %d\n", __LINE__, libusb_strerror(ret), recv_len);
        goto end;
    }
    ret = recv_len;
end:
    return ret;
}

int self_usb_sendmsg_ed2(struct libusb_device_handle * usb_p, unsigned char *write_msg, int write_len)
{
    if (usb_p == NULL) {
        return -1;
    }
    int recv_len = 0;
    int ret = libusb_bulk_transfer(usb_p, 0x2, write_msg, write_len, &recv_len, 1000);
    if (ret < 0) {
        printf("LINE:%d\tret %s recv %d\n", __LINE__, libusb_strerror(ret), recv_len);
        goto end;
    }
end:
    return ret;
}

int self_usb_recvmsg_ed2 (struct libusb_device_handle * usb_p, unsigned char *recv_msg, int recv_max)
{
    if (usb_p == NULL) {
        return -1;
    }
    int recv_len = 0;
    int ret = libusb_bulk_transfer(usb_p, 0x82, recv_msg, recv_max, &recv_len, 10);
    if (ret < 0 && ret != LIBUSB_ERROR_OVERFLOW && ret != LIBUSB_ERROR_TIMEOUT) {
        goto end;
    }
    ret = recv_len;
end:
    return ret;
}

void * handle_recv(void *arg)
{
    int DevIndex = *((int *)arg);
    *(int *)arg = 100;
    unsigned char recv_data[19*3] = {0};
    int recv_len = 0;
    int i;
    int count = 0;
    int real_write = 0;
    while (1) {
        if (exit_flag == 1) {
            exit_flag = 2;
            break;
        }

        if (DevIndex >=2)
        {
            printf ("内置接收程序启动失败\n");
            return NULL;
        }
        memset(recv_data, 0, sizeof(recv_data));
        recv_len = self_usb_recvmsg_ed2 (usbinfo_handle[DevIndex].usb_p, recv_data, sizeof(recv_data));
        count = recv_len/19;
        for (i=0; i<recv_len/19; i++) {
            if (((recv_data+(i*19))[6]&0x10) == 0x10) {
                real_write = rbuf_write(usbinfo_handle[DevIndex].rbuf_handle[1], recv_data+(i*19), 19);
            }
            else {
                real_write = rbuf_write(usbinfo_handle[DevIndex].rbuf_handle[0], recv_data+(i*19), 19);
            }
            if (real_write != 19) {
                usbinfo_handle[DevIndex].error = 0x0800;
                printf("LINE:%d,\t 缓冲区已满\n", __LINE__);
            }
        }
    }
    return NULL;
}

DWORD __stdcall VCI_UsbInit ()
{
    if (usbinit_flag != 0) {
        error = 1;
        return 0;
    }
    int i;
    for (i=0; i<sizeof(usbinfo_handle)/sizeof(usbinfo_handle[0]); i++) {
        memset (&usbinfo_handle[i], 0, sizeof(USB_INFO_T));
        usbinfo_handle[i].usb_p = NULL;
    }

    memset (recvthread_handle, 0, sizeof(recvthread_handle));

    libusb_init(&contex);
//    libusb_set_debug(contex, 4);
    usbinit_flag = 1;
    exit_flag = 0;
    error = 0;
    return 1;
}

DWORD __stdcall VCI_UsbExit ()
{
    int i;
    for (i=0; i<sizeof(usbinfo_handle)/sizeof(usbinfo_handle[0]); i++) {
        memset (&usbinfo_handle[i], 0, sizeof(USB_INFO_T));
    }

    libusb_exit(contex);
    usbinit_flag = 0;
    error = 0;
    return 1;
}


DWORD __stdcall VCI_OpenDevice(DWORD DevType, DWORD DevIndex, DWORD Reserved)
{
    if (usbinit_flag != 1) {
        error = 2;
        return 0;
    }
    //遍历所有usb设备，先找到符合vid pid 和索引值的设备
    uint16_t vendor_id, product_id;
    if (DevType == 4) {
        vendor_id = 0x0471;
        product_id = 0x1200;
    }
    else {
        usbinfo_handle[DevIndex].error = 1;
        return 0;
    }

    if (usbinfo_handle[DevIndex].use_flag == 1) {
        usbinfo_handle[DevIndex].error = 0x0100;
        return 0;
    }

    ssize_t cnt;
    libusb_device *dev;
    libusb_device **devs;
    libusb_device *devlist[2];
    int dev_num = 0;
    int i = 0;

    cnt = libusb_get_device_list(NULL, &devs);
    if (cnt < 0)
        return (int) cnt;
    while ((dev = devs[i++]) != NULL) {
        struct libusb_device_descriptor desc;
        int r = libusb_get_device_descriptor(dev, &desc);
        if (r < 0) {
            usbinfo_handle[DevIndex].error = 0x0200;
            fprintf(stderr, "打开设备错误");
            libusb_free_device_list(devs, 1);
            return 0;
        }
        if ((desc.idVendor == vendor_id) &&(desc.idProduct == product_id)) {
            if (dev_num >= 2)
                continue;
            devlist[dev_num] = dev;
            dev_num++;
        }
    }
    if (dev_num == 0) {
        usbinfo_handle[DevIndex].error = 0x1000;
        printf ("此设备不存在\n");
        usbinfo_handle[DevIndex].error = 1;
        libusb_free_device_list(devs, 1);
        return 0;
    }
    if (DevIndex > dev_num) {
        printf ("超出可用设备\n");
        usbinfo_handle[DevIndex].error = 1;
        libusb_free_device_list(devs, 1);
        return 0;
    }
    dev = devlist[DevIndex];
    int ret = libusb_open(dev, &usbinfo_handle[DevIndex].usb_p);
    if (ret < 0) {
        usbinfo_handle[DevIndex].error = 0x0200;
        libusb_free_device_list(devs, 1);
        return 0;
    }
    usbinfo_handle[DevIndex].use_flag = 1;

    libusb_free_device_list(devs, 1);
    //初始化list链表
    usbinfo_handle[DevIndex].rbuf_handle[0] = rbuf_create(19*100000);
    usbinfo_handle[DevIndex].rbuf_handle[1] = rbuf_create(19*100000);

    srand((unsigned int)time(NULL));
    //创建数据接收线程，准备接收数据。
    ret = pthread_create(&(recvthread_handle[DevIndex]), NULL, handle_recv, (void *)&DevIndex);
    usbinfo_handle[DevIndex].error = 0;
    while (1) {
        if (DevIndex == 100)
            break;
        usleep (2);
    }

    return 1;
}

DWORD __stdcall VCI_CloseDevice(DWORD DevType, DWORD DevIndex)
{
    if ((usbinfo_handle[DevIndex].use_flag != 1) || (usbinfo_handle[DevIndex].usb_p == NULL)) {
        usbinfo_handle[DevIndex].error = 0x0400;
        return 0;
    }
    unsigned char CloseCan_data[5] = {0x12, 0x0f, 0x02, 0x00, 0x00};
    unsigned char recvdata[100] = {0};
    int i = 0;
    int recv_len = 0;

    if (recvthread_handle[DevIndex] != 0) {
        exit_flag = 1;
    }
    while (1) {
        sleep (1);
        if (exit_flag != 1)
            break;
    }

    recv_len = self_readusb_info_ed1 (usbinfo_handle[DevIndex].usb_p, CloseCan_data, sizeof(CloseCan_data), recvdata, sizeof(recvdata));
    CloseCan_data[4] = 0x10;
    recv_len = self_readusb_info_ed1 (usbinfo_handle[DevIndex].usb_p, CloseCan_data, sizeof(CloseCan_data), recvdata, sizeof(recvdata));

    libusb_close(usbinfo_handle[DevIndex].usb_p);
    usbinfo_handle[DevIndex].usb_p = NULL;
    usbinfo_handle[DevIndex].use_flag = 0;

    //list链表去初始化
    rbuf_destroy(usbinfo_handle[DevIndex].rbuf_handle[0]);
    rbuf_destroy(usbinfo_handle[DevIndex].rbuf_handle[1]);


    pthread_cancel (recvthread_handle[DevIndex]);
    pthread_join (recvthread_handle[DevIndex], NULL);
    usbinfo_handle[DevIndex].error = 0;
    return 1;
}

DWORD __stdcall VCI_InitCan(DWORD DevType, DWORD DevIndex, DWORD CANIndex, PVCI_INIT_CONFIG pInitConfig)
{
    int ret = 0;
    int recv_len = 0;
    unsigned char ModeBot_data[9] = {0x12, 0x23, 0x06, 0x00, 0x00, 0x00, 0x45, 0x02, 0x57};
    unsigned char FilterAccMask_data[14] = {0x12, 0x24, 0x0b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    unsigned char recvdata[100] = {0};

    if ((usbinfo_handle[DevIndex].use_flag != 1) || (usbinfo_handle[DevIndex].usb_p == NULL)) {
        usbinfo_handle[DevIndex].error = 0x0400;
        return 0;
    }

    if (CANIndex > 1) {
        usbinfo_handle[DevIndex].error = 1;
        printf("CAN索引参数异常 %lu\n", CANIndex);
        return 0;
    }

    ret = libusb_claim_interface (usbinfo_handle[DevIndex].usb_p, 0);

    // 硬件sm4认证
    unsigned char key [16] = "itekon2012usbcan";
    unsigned char authent [20] = {0x13, 0xb0, 0x11, 0x00, 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10};
    unsigned char output [16] = {0};
    int i = 0;
#if 1
    for (i=4; i<20; i++) {
        authent[i] = rand()/256;
    }

    sm4_context ctx;
    sm4_setkey_enc (&ctx, key);
    sm4_crypt_ecb (&ctx, 1, 16, authent+4, output);
#endif
    recv_len = self_readusb_info_ed1(usbinfo_handle[DevIndex].usb_p, authent, sizeof(authent), recvdata, sizeof(recvdata));

    for (i=0; i<sizeof(output); i++) {
        if ( recvdata[4+i] != output[i] ) {
            printf ("硬件验证失败\n");
            printf ("input : ");
            int j = 0;
            for (j=0; j<16; j++) {
                printf ("%02x ", authent[4+j]);
            }
            printf ("\n");
            printf ("output: ");
            for (j=0; j<16; j++) {
                printf ("%02x ", recvdata[4+j]);
            }
            printf ("\nrealput:");
            for (j=0; j<16; j++) {
                printf ("%02x ", output[j]);
            }
            printf ("\n");
            libusb_release_interface(usbinfo_handle[DevIndex].usb_p, 0);
            return 0;
        }
    }

    if (pInitConfig->Mode == MODE_NORMAL) {
        ModeBot_data[4] = ModeBot_data[4] | 0x00;
    }
    else if (pInitConfig->Mode == MODE_ONLYLISTEN) {
        ModeBot_data[4] = ModeBot_data[4] | 0x80;
    }
    else {
        usbinfo_handle[DevIndex].error = 1;
        printf ("未知类型 %d\n", pInitConfig->Mode);
        libusb_release_interface(usbinfo_handle[DevIndex].usb_p, 0);
        return 0;
    }

    ModeBot_data[4] = ModeBot_data[4] | (((unsigned char)CANIndex)<<4);
    UINT *Mode_tmp = (UINT *)&ModeBot_data[5];
    *Mode_tmp = HTONL(pInitConfig->BotRate);

    recv_len = self_readusb_info_ed1(usbinfo_handle[DevIndex].usb_p, ModeBot_data, sizeof(ModeBot_data), recvdata, sizeof(recvdata));
    if (0x81 == recvdata[2]) {
//        printf ("设置CAN%lu波特率成功\n", CANIndex);
    }else {
        error = 8;
        printf ("设置CAN%lu波特率失败\n", CANIndex);
        usbinfo_handle[DevIndex].error = 1;
        libusb_release_interface(usbinfo_handle[DevIndex].usb_p, 0);
        return 0;
    }

    for (i=0; i<14; i++) {
        FilterAccMask_data[3] = (unsigned char )i;

        FilterAccMask_data[4] = 0x00;
        FilterAccMask_data[4] = FilterAccMask_data[4] | (1<<7) | ((unsigned char)CANIndex);

        recv_len = self_readusb_info_ed1(usbinfo_handle[DevIndex].usb_p, FilterAccMask_data, sizeof(FilterAccMask_data), recvdata, sizeof(recvdata));
        if (0x81 == recvdata[2]) {
//            printf ("设置CAN%lu滤波方式，验收码，屏蔽码成功\n", CANIndex);
        }else {
            printf ("设置CAN%lu滤波方式，验收码，屏蔽码失败\n", CANIndex);
            usbinfo_handle[DevIndex].error = 1;
            libusb_release_interface(usbinfo_handle[DevIndex].usb_p, 0);
            return 0;
        }
    }

    libusb_release_interface(usbinfo_handle[DevIndex].usb_p, 0);
    usbinfo_handle[DevIndex].error = 0;
    return 1;
}

DWORD __stdcall VCI_ReadBoardInfo(DWORD DevType, DWORD DevIndex, PVCI_BOARD_INFO pInfo)
{
    int ret = 1;
    int recv_len = 0;
    unsigned char hw_data[4] = {0x12, 0x01, 0x01, 0x00};
    unsigned char fw_data[4] = {0x12, 0x12, 0x01, 0x00};
    unsigned char cannum_data[4] = {0x12, 0x13, 0x01, 0x00};
    unsigned char SerialNum_data[4] = {0x12, 0x14, 0x01, 0x00};
    unsigned char hwType_data[4] = {0x12, 0x15, 0x01, 0x00};
    unsigned char recvdata[100] = {0};
    if ((usbinfo_handle[DevIndex].use_flag != 1) || (usbinfo_handle[DevIndex].usb_p == NULL)) {
        printf("设备未启用\n");
        usbinfo_handle[DevIndex].error = 0x0400;
        return 0;
    }

    ret = libusb_claim_interface (usbinfo_handle[DevIndex].usb_p, 0);
    if (ret < 0)
    {
        printf("USB接口获取失败 %s\n", libusb_strerror(ret));
        usbinfo_handle[DevIndex].error = 1;
        return 0;
    }
    recv_len = self_readusb_info_ed1(usbinfo_handle[DevIndex].usb_p, hw_data, sizeof(hw_data), recvdata, sizeof(recvdata));
    if (recv_len != 6) {
        printf("LINE:%d\t 获取板卡信息失败\n", __LINE__);
        libusb_release_interface(usbinfo_handle[DevIndex].usb_p, 0);
        usbinfo_handle[DevIndex].error = 1;
        return 0;
    }

    pInfo->hw_Version = (((USHORT)recvdata[5])<<8)|((USHORT)recvdata[4]);

    memset(recvdata, 0, sizeof(recvdata));
    recv_len = self_readusb_info_ed1(usbinfo_handle[DevIndex].usb_p, fw_data, sizeof(fw_data), recvdata, sizeof(recvdata));
    pInfo->fw_Version = (((USHORT)recvdata[5])<<8)|((USHORT)recvdata[4]);

    memset(recvdata, 0, sizeof(recvdata));
    recv_len = self_readusb_info_ed1(usbinfo_handle[DevIndex].usb_p, cannum_data, sizeof(cannum_data), recvdata, sizeof(recvdata));
    pInfo->can_Num = recvdata[recv_len-1];

    memset(recvdata, 0, sizeof(recvdata));
    recv_len = self_readusb_info_ed1(usbinfo_handle[DevIndex].usb_p, SerialNum_data, sizeof(SerialNum_data), recvdata, sizeof(recvdata));
    unsigned char byte2 = recvdata[2];
    int data_len = ((int)byte2)&0xf;
    memcpy(pInfo->str_Serial_Num, recvdata+4, data_len-1);

    memset(recvdata, 0, sizeof(recvdata));
    recv_len = self_readusb_info_ed1(usbinfo_handle[DevIndex].usb_p, hwType_data, sizeof(hwType_data), recvdata, sizeof(recvdata));
    byte2= recvdata[2];
    data_len = ((int)byte2)&0xf;
    memcpy(pInfo->str_hw_Type, recvdata+4, data_len-1);

    libusb_release_interface(usbinfo_handle[DevIndex].usb_p, 0);
    usbinfo_handle[DevIndex].error = 0;
    return 1;
}

DWORD __stdcall VCI_ReadErrInfo(DWORD DevType, DWORD DevIndex, DWORD CANIndex, PVCI_ERR_INFO pErrInfo)
{
    //TODO
    pErrInfo->ErrCode = usbinfo_handle[DevIndex].error;
    return 0;
}

DWORD __stdcall VCI_ReadCanStatus(DWORD DevType, DWORD DevIndex, DWORD CANIndex, PVCI_CAN_STATUS pCANStatus)
{
    //TODO
    return 0;
}

DWORD __stdcall VCI_GetReference(DWORD DevType, DWORD DevIndex, DWORD CANIndex, DWORD RefType, PVOID pData)
{
    //TODO
    return 0;
}

DWORD __stdcall VCI_SetReference(DWORD DevType, DWORD DevIndex, DWORD CANIndex, DWORD RefType, PVOID pData)
{
    //TODO
    return 0;
}

ULONG __stdcall VCI_GetReceiveNum(DWORD DevType, DWORD DevIndex, DWORD CANIndex)
{
    ULONG recvnum = 0;
    if ((usbinfo_handle[DevIndex].use_flag != 1) || (usbinfo_handle[DevIndex].usb_p == NULL)) {
        printf("设备未启用\n");
        usbinfo_handle[DevIndex].error = 0x0400;
        return 0;
    }

    if (CANIndex <= 1)
    {
        printf("%d\n", rbuf_used(usbinfo_handle[DevIndex].rbuf_handle[CANIndex]));
        recvnum = rbuf_used(usbinfo_handle[DevIndex].rbuf_handle[CANIndex])/19;
    }
    else {
        recvnum = 0;
        printf("输入CANIndex错误 %lu\n", CANIndex);
    }
    usbinfo_handle[DevIndex].error = 0;
    return recvnum;
}

DWORD __stdcall VCI_ClearBuffer(DWORD DevType, DWORD DevIndex, DWORD CANIndex)
{
    if ((usbinfo_handle[DevIndex].use_flag != 1) || (usbinfo_handle[DevIndex].usb_p == NULL)) {
        printf("设备未启用\n");
        usbinfo_handle[DevIndex].error = 0x0400;
        return 0;
    }

    if (CANIndex <= 1)
        rbuf_clear(usbinfo_handle[DevIndex].rbuf_handle[CANIndex]);
    else {
        printf("输入CANIndex错误 %lu\n", CANIndex);
        usbinfo_handle[DevIndex].error = 1;
    }
    usbinfo_handle[DevIndex].error = 0;
    return 0;
}

DWORD __stdcall VCI_StartCAN(DWORD DevType, DWORD DevIndex, DWORD CANIndex)
{
    unsigned char StartCan_data[5] = {0x12, 0x0e, 0x02, 0x00, 0x00};
    unsigned char recvdata[100] = {0};
    int recv_len = 0;
    if ((usbinfo_handle[DevIndex].use_flag != 1) || (usbinfo_handle[DevIndex].usb_p == NULL)) {
        printf("设备未启用\n");
        usbinfo_handle[DevIndex].error = 0x0400;
        return 0;
    }

    StartCan_data[4] = StartCan_data[4] | (((unsigned char )CANIndex)<<4);
    recv_len = self_readusb_info_ed1 (usbinfo_handle[DevIndex].usb_p, StartCan_data, sizeof(StartCan_data), recvdata, sizeof(recvdata));

    if (0x81 == recvdata[2]) {
//        printf ("启动CAN%d成功\n", CANIndex);
    }else {
        printf ("启动CAN%lu失败\n", CANIndex);
        usbinfo_handle[DevIndex].error = 1;
        return 0;
    }
    usbinfo_handle[DevIndex].error = 0;
    return 1;
}

DWORD __stdcall VCI_ResetCAN(DWORD DevType, DWORD DevIndex, DWORD CANIndex)
{
    unsigned char ResetCan_data[5] = {0x12, 0x0f, 0x02, 0x00, 0x00};
    unsigned char recvdata[100] = {0};
    int i = 0;
    int recv_len = 0;
    if ((usbinfo_handle[DevIndex].use_flag != 1) || (usbinfo_handle[DevIndex].usb_p == NULL)) {
        printf("设备未启用\n");
        usbinfo_handle[DevIndex].error = 0x0400;
        return 0;
    }

    ResetCan_data[4] = ResetCan_data[4] | (((unsigned char )CANIndex)<<4);
    recv_len = self_readusb_info_ed1 (usbinfo_handle[DevIndex].usb_p, ResetCan_data, sizeof(ResetCan_data), recvdata, sizeof(recvdata));
    if (0x81 == recvdata[2]) {
        rbuf_clear(usbinfo_handle[DevIndex].rbuf_handle[CANIndex]);
        printf ("%d-关闭CAN%d成功\n", __LINE__, CANIndex);
    }else {
        printf ("%d-关闭CAN%lu失败\n", __LINE__, CANIndex);
        usbinfo_handle[DevIndex].error = 1;
        return 0;
    }
    usbinfo_handle[DevIndex].error = 0;
    return 1;
}


ULONG __stdcall VCI_Transmit(DWORD DevType, DWORD DevIndex, DWORD CANIndex, PVCI_CAN_OBJ pSend, ULONG Len)
{
    //设备最大读取性能为三帧数据，多出会出异常。所以sendall空间默认为3*19长度
    unsigned char sendall[57] = {0};
    unsigned char Transmit_templet[19] = {0x01, 0x02, 0x03, 0x04, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    unsigned char Transmit_data[19] = {1};
    int i = 0, j = 0;

    if ((usbinfo_handle[DevIndex].use_flag != 1) || (usbinfo_handle[DevIndex].usb_p == NULL)) {
        printf("设备未启用\n");
        usbinfo_handle[DevIndex].error = 0x0400;
        return 0;
    }

    if (CANIndex > 1) {
        printf ("CAN索引错误 %lu\n", CANIndex);
        usbinfo_handle[DevIndex].error = 1;
        return 0;
    }

    int send_num = 0;
    int quotient = Len/3;
    int remainder = Len%3;
    for (j=0; j<quotient; j++)
    {
        memset(sendall, 0, sizeof(sendall));
        for (i = 0; i < 3; i++) {
            memset(Transmit_data, 0, sizeof(Transmit_data));
            memcpy(Transmit_data, Transmit_templet, sizeof(Transmit_templet));

            UINT *tmp = (UINT *)&Transmit_data[0];
            *tmp = HTONL(pSend[send_num].TimeStamp);
            Transmit_data[4] = pSend[send_num].TimeFlag;
            Transmit_data[5] = pSend[send_num].SendType;

            Transmit_data[6] = Transmit_data[6] | CANIndex<<4;

            if (pSend[send_num].RemoteFlag <= 1)
                Transmit_data[6] = Transmit_data[6] | pSend[send_num].RemoteFlag<<6;
            else {
                printf ("帧类型错误 %d\n", pSend[send_num].RemoteFlag);
                usbinfo_handle[DevIndex].error = 1;
                return 0;
            }

            if (pSend[send_num].ExternFlag <= 1)
                Transmit_data[6] = Transmit_data[6] | pSend[send_num].ExternFlag<<7;
            else {
                printf ("帧格式错误 %d\n", pSend[send_num].ExternFlag);
                usbinfo_handle[DevIndex].error = 1;
                return 0;
            }

            if (pSend[send_num].DataLen > 8) {
                printf("数据长度过长 %d\n", pSend[send_num].DataLen);
                usbinfo_handle[DevIndex].error = 1;
                return 0;
            }
            memcpy(Transmit_data+11, pSend[send_num].Data, pSend[send_num].DataLen);
            Transmit_data[6] = Transmit_data[6] | (pSend[send_num].DataLen&0x0f);

            UINT *tmp_id = NULL;
            tmp_id = (UINT *) &Transmit_data[7];
            *tmp_id = (pSend[send_num].ID);
            send_num++;

            memcpy(sendall + i * sizeof(Transmit_templet), Transmit_data, sizeof(Transmit_templet));
        }

        self_usb_sendmsg_ed2 (usbinfo_handle[DevIndex].usb_p, sendall, sizeof(Transmit_templet)*3);

        unsigned char read_msg[10] = {0};
        int recv_len = 0;
        int ret = 0;
        ret = libusb_interrupt_transfer (usbinfo_handle[DevIndex].usb_p, 0x81, read_msg, 10, &recv_len, 1000);
        if (ret < 0) {
            printf("LINE:%d\tret %s recv %d\n", __LINE__, libusb_strerror(ret), recv_len);
        }
    }

    memset(sendall, 0, sizeof(sendall));
    for (i = 0; i < remainder; i++) {
        memset(Transmit_data, 0, sizeof(Transmit_data));
        memcpy(Transmit_data, Transmit_templet, sizeof(Transmit_templet));

        UINT *tmp = (UINT *)&Transmit_data[0];
        *tmp = HTONL(pSend[send_num].TimeStamp);
        Transmit_data[4] = pSend[send_num].TimeFlag;
        Transmit_data[5] = pSend[send_num].SendType;

        Transmit_data[6] = Transmit_data[6] | CANIndex<<4;

        if (pSend[send_num].RemoteFlag <= 1)
            Transmit_data[6] = Transmit_data[6] | pSend[send_num].RemoteFlag<<6;
        else {
            printf ("帧类型错误 %d\n", pSend[send_num].RemoteFlag);
            usbinfo_handle[DevIndex].error = 1;
            return 0;
        }

        if (pSend[send_num].ExternFlag <= 1)
            Transmit_data[6] = Transmit_data[6] | pSend[send_num].ExternFlag<<7;
        else {
            printf ("帧格式错误 %d\n", pSend[send_num].ExternFlag);
            usbinfo_handle[DevIndex].error = 1;
            return 0;
        }

        if (pSend[send_num].DataLen > 8) {
            printf("数据长度过长 %d\n", pSend[send_num].DataLen);
            usbinfo_handle[DevIndex].error = 1;
            return 0;
        }
        memcpy(Transmit_data+11, pSend[send_num].Data, pSend[send_num].DataLen);
        Transmit_data[6] = Transmit_data[6] | (pSend[send_num].DataLen&0x0f);

        UINT *tmp_id = NULL;
        tmp_id = (UINT *) &Transmit_data[7];
        *tmp_id = (pSend[send_num].ID);
        send_num++;

        memcpy(sendall + i * sizeof(Transmit_templet), Transmit_data, sizeof(Transmit_templet));
    }

    self_usb_sendmsg_ed2 (usbinfo_handle[DevIndex].usb_p, sendall, sizeof(Transmit_templet)*remainder);

    unsigned char read_msg[10] = {0};
    int recv_len = 0;
    int ret = 0;
    ret = libusb_interrupt_transfer (usbinfo_handle[DevIndex].usb_p, 0x81, read_msg, 10, &recv_len, 1000);
    if (ret < 0) {
        printf("LINE:%d\tret %s recv %d\n", __LINE__, libusb_strerror(ret), recv_len);
    }


    usbinfo_handle[DevIndex].error = 0;

    return 1;
}

ULONG __stdcall VCI_Receive(DWORD DevType, DWORD DevIndex, DWORD CANIndex, PVCI_CAN_OBJ pReceive, ULONG Len, INT WaitTime)
{
    int i, j;
    unsigned char buf[19] = {0};
    if ((usbinfo_handle[DevIndex].use_flag != 1) || (usbinfo_handle[DevIndex].usb_p == NULL)) {
        printf("设备未启用\n");
        usbinfo_handle[DevIndex].error = 0x0400;
        return 0;
    }

    for (i=0; i< Len; i++) {
        rbuf_read(usbinfo_handle[DevIndex].rbuf_handle[CANIndex], buf, sizeof(buf));
        pReceive[i].TimeStamp = *((UINT *)&(buf[0]));
        pReceive[i].TimeFlag = buf[4];
        pReceive[i].SendType = buf[5];
        pReceive[i].RemoteFlag = buf[6]&(1<<6);
        pReceive[i].ExternFlag = buf[6]&(1<<7);
        pReceive[i].ID = *((UINT *)&(buf[7]));
        pReceive[i].DataLen = buf[6]&0x0f;
        memcpy(pReceive[i].Data, buf+11, 8);
    }
    usbinfo_handle[DevIndex].error = 0;
    return 1;
}
