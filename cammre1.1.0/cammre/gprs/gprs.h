/***************************************
	文件：gprs.h
	作者：谢剑雄
	功能：定义GPRS所需的变量
****************************************/

#ifndef __GPRS_H__  // 防止头文件被重复调用
#define __GPRS_H__

// 定义一个GPRS的结构体
struct GPRS_MODULE
{	
	int fd; // 文件描述符
	int nread;  // 读到的文件描述符中的字数
	char userNum[20]; // 保存用户手机号码
	char recBuf[1024]; // 接收到短信的缓冲区
	char sndBuf[1024]; // 发送短信的缓冲区
};


#endif