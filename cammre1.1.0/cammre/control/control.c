/***************************************
	文件：control.c
	作者：谢剑雄
	功能：控制端
****************************************/

#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "../gprs/gprs.h"
#include "control.h"

/*
function：
	GPRS(汉字转化unicode编码) 
parameter:
	char buf[100]   传进来的参数
	char save[400]   保存转换以后的编码
return:
*/ 
int change(char buf[100], char save[400])
{
	char *a = buf;
	int len = strlen(a); // 求传进来参数的长度
	char number[10] = {'0','1','2','3','4','5','6','7','8','9'};
	char lowercase[26] = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z'}; // 小写字母
	char majuscule[26] = {'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z'}; // 大写字母
 	char sixteen[6] = {'A','B','C','D','E','F'};
	int n = 0; // 循环变量
	int i = 0, j = 0; // 循环变量
	char buff[50] = {'\0'}; // 保存临时unicode编码

	while(n < len)
	{	
		switch(a[n])  // 判断是否为符号
		{
			case '.': strcat(save,"002E"); break;
			case ',': strcat(save,"002C"); break;
			case '?': strcat(save,"003F"); break;
			case '!': strcat(save,"0021"); break;
			case '>': strcat(save,"003E"); break;
			case '<': strcat(save,"003C"); break;
			case '=': strcat(save,"003D"); break;
			case '-': strcat(save,"002D"); break;
			case '+': strcat(save,"002B"); break;
		}

		if(a[n]>='0' && a[n]<='9') // 数字
		{	
			for(i=0; i<10; i++)
			{
				memset(buff, 0, sizeof(buff));
				if(a[n] == number[i])
				{	
					sprintf(buff,"003%d",i);
					strcat(save,buff);
				}
			}
		}
		else if(a[n]>='a' && a[n]<='z') // 小写字母
		{
			for(i=0; i<=25; i++) // 循环求字母
			{
				memset(buff, 0, sizeof(buff)); // 清空
				if(a[n] == lowercase[i]) // 判断是具体的哪个小写字母
				{
					if(i<=8)
					{
						sprintf(buff,"006%d",i+1);
						strcat(save,buff);
					}
					else if(i>=9 && i<=14)
					{
						j = i-9;
						sprintf(buff,"006%c",sixteen[j]);
						strcat(save,buff);
					}
					else if(i>=15 && i<=24)
					{
						j = i-15;
						sprintf(buff,"007%d",j);
						strcat(save,buff);
					}
					else if(i == 25)
					{
						sprintf(buff,"007%c",sixteen[0]);
						strcat(save,buff);
					}
				}
			}
		}
		else if(a[n]>='A' && a[n]<='Z') // 大写字母
		{
			for(i=0; i<=25; i++) // 循环求字母
			{
				memset(buff, 0, sizeof(buff)); // 清空
				if(a[n] == majuscule[i]) // 判断是具体的哪个小写字母
				{
					if(i<=8)
					{
						sprintf(buff,"004%d",i+1);
						strcat(save,buff);
					}
					else if(i>=9 && i<=14)
					{
						j = i-9;
						sprintf(buff,"004%c",sixteen[j]);
						strcat(save,buff);
					}
					else if(i>=15 && i<=24)
					{
						j = i-15;
						sprintf(buff,"005%d",j);
						strcat(save,buff);
					}
					else if(i == 25)
					{
						sprintf(buff,"005%c",sixteen[0]);
						strcat(save,buff);
					}
				}
			}
		}
		n++;
	}
}

/*
function：
	 拍照 
parameter:
	void
return:
*/
void exif_date_changer()
{
	init_camera_module();
	init_camera();
	mainloop();
	close_camera();
}

/*
function：
	 判断GPRS接收到的短信做相应的动作 
parameter:
	void
return:
*/
void* gprsmsg_receive(void *arg)
{
	while(1)
	{
		char buff[100] = {'\0'}; // 保存接收到的短信  用来判断是否是对应的指令
		
		GPRS_receive(buff);
		// 调用接收短信函数

		//printf("buff::%s\n",buff);
		if (strcmp(buff, "look") == 0) // 收到看照片指令
		{
			exif_date_changer();
			// 调用拍照函数
			pthread_cond_signal(cond);
			//  pthread_cond_signal函数的作用是发送一个信号给另外一个正在处于阻塞等待状态的线程,使其脱离阻塞状态,继续执行.如果没有线程处在阻塞等待状态,pthread_cond_signal也会成功返回
			// pthread_cond_signal调用最多发信一次

			jpg = 1;
			// 照片允许位
			
		}
		else if(strcmp(buff, "see") == 0) // 收到查看安防信息指令
		{
		//	printf("gprs_receive OK look!");
			information = 1;  // 查看安防信息
		}
		else if(strcmp(buff, "close") == 0) // 收到关防信息指令
		{
			button = 0;
		}
		else if(strcmp(buff, "open") == 0) // 收到开防信息指令
		{
			button = 1;
		}
		//1,检测GPRS接收到的信息
		//2,根据GPRS接收到的信息做相应的动作
		//发照片需要告诉服务器端  解决 用信号的方式
	}
}

/*
function：
	 合成value结构体(发送安防信息)
parameter:
	 char buff[256] 传进来一个参数 使用它获得返回值
return:
*/
void synthetic_value(char buff[256])
{	
	char sbuf[100]={'\0'}; // 保存温度
	char tbuf[400]={'\0'}; // 保存unicode编码

	strcat(buff,"5f53524d6e295ea6"); 
	// 当前温度为
	sprintf(sbuf,"%0.2f",sql.temp);
	// 把随机的温度写到sbuf中
	change(sbuf,tbuf);
	// 通过调用函数 change  GPRS(汉字转化unicode编码) 
	strcat(buff,tbuf);
	// 把温度拼接到buff中
	strcat(buff,"2103000A");
	// ℃ \n
	if(sql.smoke == 1)
	// 没有烟雾
	{
		//strcat(buff,"dang qian mei you jian ce dao yan wu\n");
		strcat(buff,"5f53524D6CA1670968C06D4B523070DF96FE000A");
		// 当前没有检测到烟雾
	}
	else if(sql.smoke == 0)
	// 有烟雾
	{
		//strcat(buff,"dang qian yi jing jian ce dao yan wu\n");
		strcat(buff,"5f53524D5DF27ECF68C06D4B523070DF76FE000A");
		// 当前已经检测到烟雾
	}
	if(sql.light == 0)
	// 红外传感器没有检测到人
	{
		//strcat(buff,"dang qian mei you jian ce dao ren\n");
		strcat(buff,"5f53524D6CA1670968C06D4B52304EBA000A");
		// 当前没有检测到人
	}
	else if(sql.light == 1)
	// 红外传感器有检测到人
	{
		//strcat(buff,"dang qian yi jing jian ce dao ren\n");
		strcat(buff,"5f53524D5DF27ECF68C06D4B52304EBA000A");
		// 当前已经检测到人
	}
	if(sql.vibrate == 1)
	// 振动传感器没有检测到振动
	{
		//strcat(buff,"dang qian mei you jian ce dao bo li po sui");
		strcat(buff,"5f53524D6CA1670968C06D4B523073BB74837834788E");
		// 当前没有检测到玻璃破碎
	}
	else if(sql.vibrate == 0)
	// 振动传感器已经检测到振动
	{
		//strcat(buff,"dang qian yi jing jian ce dao bo li po sui");
		strcat(buff,"5f53524D5DF27ECF68C06D4B523073BB74837834788E");
		// 当前已经检测到玻璃破碎
	}
}
 
/*
function：
	 gprs发送线程函数
parameter:
	 void *arg 参数  （可以接收任意类型的值）
return:
*/
void* gprsmsg_send(void *arg)
{
	int a = 0;
	int b = 0;
	// 用来解决同时发送的问题(同时发送只能接收1条信息)

	while(1)
	{
		char buff[256] = {'\0'};
		if(fire == 1 && button == 1 ) 
		// fire 是否发生火灾 1：发生    
		// button 开关防 0：关防 1：开防
		{
			if(a == 0)
			{	
				GPRS_send("60A876845BB691CC53D1751F4E86706B707EFF01");
				// 您的家里发送了火灾!
				b = 1;
				fire = 0; // 置0
			}
			else
			{
				sleep(1); // 延时1秒发送，解决同时发送问题
				GPRS_send("60A876845BB691CC53D1751F4E86706B707EFF01");
				fire = 0;
			}
			
		}
		else
		{
			b = 0;
		}

		if(information == 1 && button == 1)  
		// information == 1 查看安防信息
		// 0：关防 1：开防
		{
			synthetic_value(buff);
			// 合成value结构体(发送安防信息)

			if (b == 0)
			{
				GPRS_send(buff);  // 发送value结构体的信息(传感器信息)
				a = 1;
			}
			else
			{
				sleep(1); // 延时1秒发送，解决同时发送问题
				GPRS_send(buff);
			}
			
			
			information = 0;  // 置0
		}
		else
		{
			a = 0;
		}

	}
}

/*
function：
	 socket线程函数(客户端)
parameter:
	 void *arg 参数  （可以接收任意类型的值）
return:
*/
void* socketmsg(void *arg)
{
	int  ret;

	struct sockaddr_in  seraddr;  //创建协议地址族

	int clientscoket = socket(PF_INET, SOCK_STREAM,0); // 创建socket   SOCK_STREAM 数据流形式发送
	while(clientscoket==-1)   // 防止没创建成功
	{
		clientscoket=socket(PF_INET, SOCK_STREAM, 0);
	}

	bzero(&seraddr,sizeof(seraddr)); 

	//绑定协议地址族
	seraddr.sin_family=AF_INET;

	seraddr.sin_port=htons(8087);

	seraddr.sin_addr.s_addr=inet_addr("192.168.1.101"); 
	//seraddr.sin_addr.s_addr=inet_addr("127.0.0.1"); 
	
	char buf[10] = {'\0'}; // 保存发送的数据
	int enable_picture = 1; // 打开图片使能位
	int enable_connect = 1; // 连接服务器使能位

	int fd = open("/mnt/current.jpg", O_RDONLY,0777);  // 读照片
	
	int num1 = lseek(fd,0,SEEK_END); 
	printf("%d\n", num1);
	lseek(fd,0,SEEK_SET);
	// 计算文件大小

	while(1)
	{
		if(jpg == 0)  //照片发送允许位   0：不允许
		{
			pthread_cond_wait(cond, mutex);
			//   pthread_cond_wait() 用于阻塞当前线程，等待别的线程使用pthread_cond_signal()或pthread_cond_broadcast来唤醒它									
			//等待命令到来
		}
		if(enable_picture == 1)  // 打开图片使能位
		{
			fd = open("/mnt/current.jpg", O_RDONLY,0777);
			if (fd == -1)
			{
				printf("current.jpg  open  error\n");
			}
			else
			{
				printf("current.jpg  open  susses\n");
			}
			enable_picture = 0; // 打开图片使能位
		}
		//ret = send(clientscoket, "jpg!!", 5, 0);   							//发送一个字符串
		//ret = sendto(clientscoket,buf,strlen(buf),0,(struct sockaddr*)&seraddr,sizeof(seraddr));
		bzero(buf,10);

		int num = read(fd, &buf[0], 1);
		if (num == 0)
		{
			jpg = 0;

			close(fd);
			enable_picture = 1; // // 打开图片使能位
			printf("传输完毕！！！！\n");
			close(clientscoket);
			enable_connect = 1; // 连接服务器使能位

			struct sockaddr_in  seraddr;  //创建协议地址族

			int clientscoket = socket(PF_INET, SOCK_STREAM,0); // 创建socket 

			while(clientscoket==-1)  
			{
				clientscoket=socket(PF_INET, SOCK_STREAM, 0);
			}

			bzero(&seraddr,sizeof(seraddr)); 
			//绑定协议地址族
			seraddr.sin_family=AF_INET;
			seraddr.sin_port=htons(8087);
			seraddr.sin_addr.s_addr=inet_addr("192.168.1.101"); 
			//seraddr.sin_addr.s_addr=inet_addr("127.0.0.1"); 
		}
		else if(num > 0)
		{
			if(enable_connect == 1) // 连接服务器使能位
			{
				// 连接服务器
				ret = connect(clientscoket, (struct sockaddr*)&seraddr, sizeof(struct sockaddr));
				while(ret==-1)
				{
					printf("连接失败\n");
					ret = connect(clientscoket, (struct sockaddr*)&seraddr, sizeof(struct sockaddr));
				}
				printf("连接成功\n");
				enable_connect = 0; // 连接服务器使能位
			}
		

			ret = sendto(clientscoket,&buf[0],1,0,(struct sockaddr*)&seraddr,sizeof(seraddr));
			// 每次发一位

			// if (ret < 0)
			// {
			// int see = connect(clientscoket, (struct sockaddr*)&seraddr, sizeof(struct sockaddr));
			
		}
	}
}

/*
function：
	 socket线程函数(客户端)
parameter:
	 void *arg 参数  （可以接收任意类型的值）
return:
*/
void init_pthread()
{
	int ret;
	printf("创建开始\n");
	mutex = (pthread_mutex_t*)malloc(1*sizeof(pthread_mutex_t));
	// 开辟互斥锁内存

   	while(mutex==NULL)
   	{
    	mutex = (pthread_mutex_t*) malloc(1*sizeof(pthread_mutex_t));
   	}
   	
   	cond = (pthread_cond_t*) malloc(1*sizeof(pthread_cond_t));
   	// 开辟条件变量内存

   	while(cond==NULL)
    {
    	cond=(pthread_cond_t*)malloc(1*sizeof(pthread_cond_t));
    }

    pthread_mutex_init(mutex,NULL);
    // 初始化互斥锁
    // 加锁(lock)后，别人就无法打开，只有当锁没有关闭(unlock)的时候才能访问资源

    pthread_cond_init(cond,NULL);
    // 初始化条件变量
    // 设置条件变量是进程内可用还是进程间可用
    // 默认进程内可用 默认值是PTHREAD_ PROCESS_PRIVATE，即此条件变量被同一进程内的各个线程使用
	
	ret = pthread_create(&gprsreceive, NULL, gprsmsg_receive, NULL);
	// 创建GPRS接收线程
	// 成功则返回0，否则返回出错编号

	while(ret == -1)
	{
		ret = pthread_create(&gprsreceive, NULL, gprsmsg_receive, NULL);
	}

	ret = pthread_create(&socketpthread, NULL, socketmsg, NULL);
	// 创建socket线程

	while(ret == -1)
	{
	  	ret = pthread_create(&socketpthread, NULL, socketmsg, NULL);
	}	

	ret = pthread_create(&gprssend, NULL, gprsmsg_send, NULL);
	// 创建GPRS发送线程

	while(ret == -1)
	{
			ret = pthread_create(&gprssend, NULL, gprsmsg_send, NULL);
	}
	printf("创建完成\n");
}


int main(int argc, char const *argv[])
{
	GPRS_init(); // 初始化GPRS
	init_pthread(); //创建线程，锁，条件变量
 	
 	
	int fd = open("dev/tmp_out",O_RDONLY,0777);   //打开，传感器驱动文件
	if(fd < 0)
	{
    	printf("tmp_out Open error!\n");

    	return -1;
	}
	else
	{
    	printf("tmp_out Open susses!\n");
	}

    int fp = open("dev/relay",O_RDONLY,0777);   //打开，继电器驱动文件 
	if(fp < 0)
	{
    	printf("relay Open error!\n");

    	return -1;
	}
	else
	{
    	printf("relay Open susses!\n");
	}
	ioctl(fp,0);          //初始化，继电器

	while(1)
	{
		
		char buff[6] = {0};

    	bzero(buff,6);

    	read(fd, buff, sizeof(buff)); // 读传感器驱动文件

    	sql.temp = (int)buff[0] + (float)(buff[1]*0.01);   // 温度

		sql.smoke = buff[2];  // 烟雾

    	sql.light = buff[3]; // 红外

    	sql.vibrate = buff[4]; // 振动

    	printf("temp:%.2f°C smoke:%d light:%d vibrate:%d\n", sql.temp, sql.smoke, sql.light, sql.vibrate); 

    	// 判断是否发生火灾
    	if (sql.smoke == 0 && sql.temp>57)
    	{
    		fire = 1;
    		ioctl(fp,1);  // 关闭继电器
    		printf("fire  fire  fire  \n");
    		break;
    	}
	}
	
    pthread_join(gprsreceive,NULL); 
    // 等待线程结束 

    pthread_join(gprssend,NULL);

	pthread_join(socketpthread,NULL);

	return 0;
}


