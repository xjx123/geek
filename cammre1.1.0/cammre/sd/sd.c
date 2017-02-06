/**************************************************************
       文件：SD.c
	作者：李达、李峡
	日期：2012-8-1
	描述：
	函数列表：主要函数及其功能
	1.
	历史修改记录：
	<作者>        <日期>        <描述>
         李达          2012-8-1       建立文件
         李峡          2012-8-3       修改文件
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
{//挂载
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
{//将filename移动至sd卡中
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
{//关闭sd模块
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

