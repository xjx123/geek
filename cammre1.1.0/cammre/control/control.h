/***************************************
	文件：control.h
	作者：谢剑雄
	功能：定义control所需的变量
****************************************/

#ifndef __CONTROL_H__  // 防止头文件被重复调用
#define __CONTROL_H__

// 保存传感器的数据
typedef struct value 
{
	float temp;   //温度
	int smoke;   //烟雾
	int light;   //红外
	int vibrate;  //振动
}val;

pthread_mutex_t * mutex = NULL;//互斥锁
pthread_cond_t  *cond = NULL;//条件变量
pthread_t gprsreceive;   // gprs接收线程
pthread_t socketpthread; // socket线程
pthread_t gprssend; // gprs发送线程

static int button = 1; // 0：关防 1：开防

static val sql;      //信息结构体

static int information = 0; // 0：不查看安防信息 1: 查看安防信息

static int fire = 0;  //0：没有发生火灾  1:发生火灾 

static int jpg = 0;   //照片发送允许位   0：不允许 1：允许


#endif