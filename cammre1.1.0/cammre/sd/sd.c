/**************************************************************
       �ļ���SD.c
	���ߣ�����Ͽ
	���ڣ�2012-8-1
	������
	�����б���Ҫ�������书��
	1.
	��ʷ�޸ļ�¼��
	<����>        <����>        <����>
         ���          2012-8-1       �����ļ�
         ��Ͽ          2012-8-3       �޸��ļ�
***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <malloc.h>
#include <sys/mman.h>
#include <stdint.h>

#include "sd.h"

struct SD_module SD_struct;

void init_SD(void)
{//����
    int i;
	char sbuf[64];
	
    SD_struct.sdStatus = sdClose;
/*	for(i = 1; i < 10; i++)
	{
	    memset(sbuf, 0, sizeof(sbuf));
	    sprintf(sbuf, "mount /dev/mmcblk%dp1 /mnt", i);
        system(sbuf);
	}
*/
    system("mount /dev/sdcard /mnt");
	return;
}

void save_to_SD(char* filename)
{//��filename�ƶ���sd����
    char sbuf[256];
    
    if(SD_struct.sdStatus == sdOpen)
	{
	    memset(sbuf, 0, sizeof(sbuf));
//    	sprintf(sbuf, "mv ./%s /mnt", filename);
        strcpy(sbuf, "mv *.jpg /mnt");
        system(sbuf);
	}
	SD_struct.sdStatus = sdClose;
	return;
}

void close_SD(void)
{//�ر�sdģ��
    int i;
	char sbuf[64];
	
	SD_struct.sdStatus = sdClose;
/*
	for(i = 1; i < 10; i++)
	{
	    memset(sbuf, 0, sizeof(sbuf));
	    sprintf(sbuf, "umount /dev/mmcblk%dp1", i);
        system(sbuf);
	}
*/
    system("umount /dev/sdcard");
}

void listen_SD(void)
{
    sleep(600);
	SD_struct.sdStatus = sdOpen;
//	sleep(1);
//	SD_struct.sdStatus = sdClose;
}

