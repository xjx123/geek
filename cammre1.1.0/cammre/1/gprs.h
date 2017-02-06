#if 0
	文件：gprs.h
	作者：谢剑雄
	日期：2016-06-05
#endif

#ifndef __GPRS_H__  // 防止头文件被重复调用
#define __GPRS_H__

// 定义一个GPRS的结构体
struct GPRS_MODULE
{	
	int fd; // 文件描述符
	int isBusy; // 判断是否有接收到短信
	int nread;  // 读到的文件描述符中的字数
	char userNum[20]; // 保存用户手机号码
	char recBuf[1024]; // 接收到短信的缓冲区
	char sndBuf[1024]; // 发送短信的缓冲区
};


#endif