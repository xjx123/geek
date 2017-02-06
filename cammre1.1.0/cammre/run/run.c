#include <stdio.h>
#include <strings.h>
#include "sys/types.h"
#include "fcntl.h"
#include "gprs.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#define NO 0
#define YES 1

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

void init_pthread();

static int button = 1; // 0：关防 1：开防

static val sql;      //信息结构体

static int information = 0; // 0：不查看安防信息 1: 查看安防信息

static int fire = 0;  //0：没有发生火灾  1:发生火灾 

static int jpg = NO;   //照片发送允许位  

// GPRS(汉字转化unicode编码)  
int change(char a[100], char save[400])
{
	int len = strlen(a); // 求传进来参数的长度
	char number[10] = {'0','1','2','3','4','5','6','7','8','9'};
	char lowercase[26] = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z'}; // 小写字母
	char majuscule[26] = {'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z'}; // 大写字母
 	char sixteen[6] = {'A','B','C','D','E','F'};
	int n = 0; // 循环变量
	int i = 0, j = 0; // 循环变量
	char buff[50] = {'\0'};

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

		if(a[n]>='0' && a[n]<='9')
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
		else if(a[n]>='a' && a[n]<='z')
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
		else if(a[n]>='A' && a[n]<='Z')
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

void exif_date_changer()
{
	init_camera_module();
	init_camera();
	mainloop();
	close_camera();
}

void* gprsmsg_receive(void *arg)
{
	while(1)
	{
		char buff[100] = {'\0'};
		
		GPRS_receive(buff);

		// 
		//printf("buff::%s\n",buff);
		if (strcmp(buff, "look") == 0) // 收到看照片指令
		{
			exif_date_changer();
			pthread_cond_signal(cond);
		//	printf("gprs_receive OK abc !");
			jpg = YES;
			
		}
		else if(strcmp(buff, "see") == 0) // 收到查看安防信息指令
		{
		//	printf("gprs_receive OK look!");
			information = 1;
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

// 合成value结构体(发送安防信息)
void synthetic_value(char buff[256])
{	
	char sbuf[100]={'\0'};
	char tbuf[400]={'\0'};
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
	if(sql.light == 1)
	// 红外传感器没有检测到人
	{
		//strcat(buff,"dang qian mei you jian ce dao ren\n");
		strcat(buff,"5f53524D6CA1670968C06D4B52304EBA000A");
		// 当前没有检测到人
	}
	else if(sql.light == 0)
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

// gprs发送线程函数
void* gprsmsg_send(void *arg)
{
	//相异
	int a = 0;
	int b = 0;
	while(1)
	{
		char buff[256] = {'\0'};
		if(fire == 1 && button == 1 )
		{
			//GPRS_send(9648);
			//GPRS_send("chen");
			//GPRS_send("陈");
			if(a == 0)
			{
				sleep(1);
				GPRS_send("60A876845BB691CC53D1751F4E86706B707EFF01");
				fire = 0;
			}
			else
			{
				GPRS_send("60A876845BB691CC53D1751F4E86706B707EFF01");
				b = 0;
				fire = 0;
			}
			
		}
		else
		{
			b = 1;
		}

		if(information == 1 && button == 1)  // 0：关防 1：开防
		{
			synthetic_value(buff);
			if (b == 1)
			{
				GPRS_send(buff);
				a = 0;
			}
			else
			{
				sleep(1);
				GPRS_send(buff);
			}
			
			
			information = 0;
		}
		else
		{
			a = 1;
		}

	}
}

//问题1：如何实现socket断开后重连。
//问题2：如何让JPG文件断点重连。
//问题3：怎么实现文件选择发送。
//问题4：怎样找到图片文件目录。   摄像头在拍摄照片后，用一个文件保存文件路径名。
void* socketmsg(void *arg)
{
	int  ret;

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

	//seraddr.sin_addr.s_addr=inet_addr("192.168.1.100"); 
	seraddr.sin_addr.s_addr=inet_addr("127.0.0.1"); 
	//连接服务器
	ret = connect(clientscoket, (struct sockaddr*)&seraddr, sizeof(struct sockaddr));

	while(ret==-1)
	{
		ret = connect(clientscoket, (struct sockaddr*)&seraddr, sizeof(struct sockaddr));
	}

	char buf[10] = "";

	

	while(1)
	{
		if(jpg == 0)
		{
			pthread_cond_wait(cond, mutex);									//等待命令到来
		}

		int fd = open("/mnt/current.jpg", O_RDONLY,0777);
		if (fd == -1)
		{
			printf("current.jpg  open  error\n");
		}
		else
		{
			printf("current.jpg  open  susses\n");
		}

		//ret = send(clientscoket, "jpg!!", 5, 0);   							//发送一个字符串
		//ret = sendto(clientscoket,buf,strlen(buf),0,(struct sockaddr*)&seraddr,sizeof(seraddr));
		bzero(buf,10);


		int num = read(fd, &buf[0], 1);
		if (num == 0)
		{
			jpg = 0;

			close(fd);

			printf("传输完毕！！！！\n");
		}
		else if(num > 0)
		{
			ret = sendto(clientscoket,&buf[0],1,0,(struct sockaddr*)&seraddr,sizeof(seraddr));
			if (ret < 0)
			{
			int see = connect(clientscoket, (struct sockaddr*)&seraddr, sizeof(struct sockaddr));
			if (see == -1)
			{
				printf("连接失败！！\n");
				sleep(5);
			}

			}
		}
		//1,等待信号的到来
		//2,开始发送照片
	}
}

void init_pthread()
{
	int ret;
	printf("创建开始\n");
	mutex = (pthread_mutex_t*)malloc(1*sizeof(pthread_mutex_t));

   	while(mutex==NULL)
   	{
    	mutex = (pthread_mutex_t*) malloc(1*sizeof(pthread_mutex_t));
   	}
   	
   	cond = (pthread_cond_t*) malloc(1*sizeof(pthread_cond_t));

   	while(cond==NULL)
    {
    	cond=(pthread_cond_t*)malloc(1*sizeof(pthread_cond_t));
    }

    pthread_mutex_init(mutex,NULL);

    pthread_cond_init(cond,NULL);
	
	ret = pthread_create(&gprsreceive, NULL, gprsmsg_receive, NULL);

	while(ret == -1)
	{
		ret = pthread_create(&gprsreceive, NULL, gprsmsg_receive, NULL);
	}

	ret = pthread_create(&socketpthread, NULL, socketmsg, NULL);

	while(ret == -1)
	{
	  	ret = pthread_create(&socketpthread, NULL, socketmsg, NULL);
	}	

	ret = pthread_create(&gprssend, NULL, gprsmsg_send, NULL);

	while(ret == -1)
	{
			ret = pthread_create(&gprssend, NULL, gprsmsg_send, NULL);
	}
	printf("创建完成\n");
}


int main(int argc, char const *argv[])
{
	GPRS_init();
 	init_pthread();         //创建线程，锁，条件变量
 	
 	
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

		if(fd < 0)
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

    	read(fd, buff, sizeof(buff));

    	sql.temp = (int)buff[0] + (float)(buff[1]*0.01);   

    	sql.smoke = buff[2];

    	sql.light = buff[3];

    	sql.vibrate = buff[4];

    	printf("temp:%.2f°C smoke:%d light:%d vibrate:%d\n", sql.temp, sql.smoke, sql.light, sql.vibrate);

    	if (sql.smoke == 0 && sql.temp>57)
    	{
    		fire = 1;

    		ioctl(fp,1);

    		printf("fire  fire  fire  \n");


    	}
	}
	
    pthread_join(gprsreceive,NULL);

    pthread_join(gprssend,NULL);

	pthread_join(socketpthread,NULL);

	return 0;
}


