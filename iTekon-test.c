
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include "lib/iTekon-usb.h"

int breakflag = 0;
void *handle_send(void *arg)
{
    int ret = 0;
    int i = 0;
    VCI_CAN_OBJ send_msg[8];
    unsigned char test[8] = {1,2,3,4,5,6,'a','1'};
    memset (send_msg, 0 ,sizeof(VCI_CAN_OBJ) * 5);
    while (1) 
    {
	    i++;
	    if (breakflag) {
		    breakflag = 2;
		    return NULL;
	    }
	    usleep (1000*1000);
	    if (i>0 && i<50)
	    {
		    test[7]++;
		    test[6] = 'a';
		    memcpy(send_msg[0].Data, test, sizeof(test));
		    send_msg[0].ID = 1;
		    send_msg[0].DataLen = sizeof(test);
		    send_msg[0].ExternFlag = 0;
		    send_msg[0].RemoteFlag = 0;
		    send_msg[0].SendType = 0;
		    send_msg[0].TimeFlag = 1;
		    send_msg[0].TimeStamp = 0x12345678;

		    test[7]++;
		    send_msg[1].ID = 2;
		    memcpy(send_msg[1].Data, test, sizeof(test));
		    send_msg[1].DataLen = sizeof(test);
		    send_msg[1].ExternFlag = 0;
		    send_msg[1].RemoteFlag = 0;
		    send_msg[1].SendType = 0;
		    send_msg[1].TimeFlag = 1;
		    send_msg[1].TimeStamp = 0x12345678;

		    test[7]++;
		    send_msg[2].ID = 3;
		    memcpy(send_msg[2].Data, test, sizeof(test));
		    send_msg[2].DataLen = sizeof(test);
		    send_msg[2].ExternFlag = 0;
		    send_msg[2].RemoteFlag = 0;
		    send_msg[2].SendType = 0;
		    send_msg[2].TimeFlag = 1;
		    send_msg[2].TimeStamp = 0x12345678;
		    
		    test[7]++;
		    send_msg[3].ID = 4;
		    memcpy(send_msg[3].Data, test, sizeof(test));
		    send_msg[3].DataLen = sizeof(test);
		    send_msg[3].ExternFlag = 0;
		    send_msg[3].RemoteFlag = 0;
		    send_msg[3].SendType = 0;
		    send_msg[3].TimeFlag = 1;
		    send_msg[3].TimeStamp = 0x12345678;

		    test[7]++;
		    send_msg[4].ID = 5;
		    memcpy(send_msg[4].Data, test, sizeof(test));
		    send_msg[4].DataLen = sizeof(test);
		    send_msg[4].ExternFlag = 0;
		    send_msg[4].RemoteFlag = 0;
		    send_msg[4].SendType = 0;
		    send_msg[4].TimeFlag = 1;
		    send_msg[4].TimeStamp = 0x12345678;

            send_msg[5].ID = 5;
		    memcpy(send_msg[5].Data, test, sizeof(test));
		    send_msg[5].DataLen = sizeof(test);
		    send_msg[5].ExternFlag = 0;
		    send_msg[5].RemoteFlag = 0;
		    send_msg[5].SendType = 0;
		    send_msg[5].TimeFlag = 1;
		    send_msg[5].TimeStamp = 0x12345678;
            
            send_msg[6].ID = 5;
		    memcpy(send_msg[6].Data, test, sizeof(test));
		    send_msg[6].DataLen = sizeof(test);
		    send_msg[6].ExternFlag = 0;
		    send_msg[6].RemoteFlag = 0;
		    send_msg[6].SendType = 0;
		    send_msg[6].TimeFlag = 1;
		    send_msg[6].TimeStamp = 0x12345678;

            send_msg[7].ID = 5;
		    memcpy(send_msg[7].Data, test, sizeof(test));
		    send_msg[7].DataLen = sizeof(test);
		    send_msg[7].ExternFlag = 0;
		    send_msg[7].RemoteFlag = 0;
		    send_msg[7].SendType = 0;
		    send_msg[7].TimeFlag = 1;
		    send_msg[7].TimeStamp = 0x12345678;

//            send_msg[8].ID = 5;
//		    memcpy(send_msg[8].Data, test, sizeof(test));
//		    send_msg[8].DataLen = sizeof(test);
//		    send_msg[8].ExternFlag = 0;
//		    send_msg[8].RemoteFlag = 0;
//		    send_msg[8].SendType = 0;
//		    send_msg[8].TimeFlag = 1;
//		    send_msg[8].TimeStamp = 0x12345678;



		    ret =VCI_Transmit (254, 0,1, send_msg, 8);
	    }
	    else
		    i=0;
    }
    return NULL;
}

void exit_can(int flag)
{
    if (1 != flag)
    {
        //关闭发送线程。
        breakflag = 1;
        while(1)
        {
            if((breakflag == 2) || (breakflag == 0)) {
                break;
            }
        }
        breakflag = 0;
    }
    VCI_ResetCAN(254, 0, 0);
    VCI_ResetCAN(254, 0, 1);
    VCI_CloseDevice (254, 0);
    VCI_UsbExit();
    exit(0);
}

void signal_handle()
{
    struct sigaction sig_handle;
    memset(&sig_handle, 0, sizeof(sig_handle));
    sigemptyset(&sig_handle.sa_mask);
    sig_handle.sa_flags = SA_SIGINFO;
    sig_handle.sa_sigaction = exit_can;
    sigaction(SIGINT, &sig_handle, NULL);
}

#if 1
int main(void)
{
    pthread_t recv_handle, send_handle;
	VCI_BOARD_INFO board_info;
	VCI_INIT_CONFIG init_config;
    int ret = 0;
    signal_handle ();
    ret = VCI_UsbInit ();
    if (ret == 0) {
        exit_can(1);
        return 0;
    }
    ret = VCI_OpenDevice(4, 0, 0);
    if (ret == 0) {
        exit_can(1);
        return 0;
    }

	bzero(&init_config, sizeof(VCI_INIT_CONFIG));
	init_config.AccCode = 0;
	init_config.AccMask = 0;
	init_config.Filter = 0;
	init_config.Mode = MODE_NORMAL;
	init_config.BotRate =BOT_1000K;

    ret = VCI_InitCan(254, 0, 0, &init_config);
    if (ret == 0) {
        exit_can(1);
        return 0;
    }

    ret = VCI_InitCan(254, 0, 1, &init_config);
    if (ret == 0) {
        exit_can(1);
        return 0;
    }

	bzero(&board_info, sizeof(board_info));
    ret = VCI_ReadBoardInfo(254, 0, &board_info);
    if (ret == 0) {
        exit_can(1);
        return 0;
    }

    ret = VCI_StartCAN (254, 0, 0);
    if (ret == 0) {
        exit_can(1);
        return 0;
    }

    ret = VCI_StartCAN (254, 0, 1);
    if (ret == 0) {
        exit_can(1);
        return 0;
    }

    ret = pthread_create(&send_handle, NULL, handle_send, NULL);

	VCI_CAN_OBJ recv_msg[1];
	memset(recv_msg, 0, sizeof(VCI_CAN_OBJ)*1);
	while (1) {
		int num = VCI_GetReceiveNum(254, 0, 0);
		printf("----%d--can0 存储空间中有 %d条数据\n", __LINE__, num);
		VCI_Receive(254, 0, 0, recv_msg, 1, 0);
		int i,j;
		for (i=0; i<1; i++) {
			printf("ID: %08x DataLen:%02x ExternFlag:%02x RemoteFlag: %02x data:", recv_msg[i].ID, recv_msg[i].DataLen, recv_msg[i].ExternFlag, recv_msg[i].RemoteFlag);
			for (j=0; j< recv_msg[i].DataLen; j++) {
				printf("%02x ", recv_msg[i].Data[j]);
			}
			printf("\n");

		}
		num = VCI_GetReceiveNum(254, 0, 1);
		printf("----%d--can1 存储空间中有 %d条数据\n", __LINE__, num);
		VCI_Receive(254, 0, 1, recv_msg, 1, 0);
		for (i=0; i<1; i++) {
			printf("ID: %08x DataLen:%02x ExternFlag:%02x RemoteFlag: %02x data:", recv_msg[i].ID, recv_msg[i].DataLen, recv_msg[i].ExternFlag, recv_msg[i].RemoteFlag);
			for (j=0; j< recv_msg[i].DataLen; j++) {
				printf("%02x ", recv_msg[i].Data[j]);
			}
			printf("\n");

		}
		sleep(1);
	}
	goto selfstop;

	VCI_ResetCAN (254, 0, 0);
selfstop:
	VCI_CloseDevice (254, 0);
	return 0;
}

#endif
