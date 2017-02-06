/***************************************
	文件：gprs.h
	作者：谢剑雄
	功能：GPRS初始化、发送、接收
****************************************/

#include "gprs.h"
#include <stdio.h>
#include <string.h> // memset  strcpy
#include <errno.h> // perror
#include <sys/types.h>  // open
#include <sys/stat.h> // open
#include <fcntl.h> // open
#include <termios.h> // struct termios
#include <unistd.h> // struct termios  sleep
#include <stdlib.h> // malloc


struct GPRS_MODULE gprs_module; 
// gprs.h 中的结构体

/*
function：
	从文件描述符fd中读数据到recBuf中，并计算读到的个数
parameter:
	void
return:
*/
int GPRS_number()
{
	int ret = 0; 
	// 判断是否读到数据
	gprs_module.nread = 0; 
	// 清空读到的个数
	int nread = 0;
	// 读到的个数

	memset(gprs_module.recBuf, 0, sizeof(gprs_module.recBuf));
	// 清空接收缓冲区

	ret = read(gprs_module.fd, gprs_module.recBuf, sizeof(gprs_module.recBuf)); 
	//从fd中读sizeof(recBuf)个数据到recBuf中

	if(0 != ret)
	// 判断是否读到文件末尾  读到文件末尾跳出或者 没读到数据跳出
	{
		printf("recbuf:%s\n", gprs_module.recBuf); // 打印 接收到短信的缓冲区
		nread = ret; // 把读到的个数保存
	}
	gprs_module.nread = nread;
	// 读到的文件描述符中的字数

	return 0;
}

/*
function：
	 等待接收短信 
parameter:
	char rbuf[100]  接收到消息
return:
*/
int GPRS_receive(char rbuf[100])
{
	char *temp = NULL; // 保存接收到的数据
	char *tempNum = NULL; // 保存接收到的号码
	int z;  // 循环变量
	memset(gprs_module.recBuf, 0, sizeof(gprs_module.recBuf));
	// 清空接收数据缓冲区
	write(gprs_module.fd,"AT+CNMA\r", 8);
	// 新消息应答 
	sleep(1);
	read(gprs_module.fd, gprs_module.recBuf, sizeof(gprs_module.recBuf));  
	// 读接收到的数据
	if(strstr(gprs_module.recBuf, "OK"))
	// 判断是否接收到
	{
		printf("Receive OK\n");
	}
	else if(strstr(gprs_module.recBuf, "CMT"))
	// 判断是否是接收到的短信
	{
		//if(strstr(gprs_module.recBuf, "18569499009"))
			// 限定号码
		//{
			temp = strtok(gprs_module.recBuf, "\""); 
			// 截取 " 之前的数据
			for(z=0; z<4; z++)
			{
				if(z==1)
				{
					tempNum = temp;  // 保存号码
				}
				temp = strtok(NULL, "\""); // 截取到发过来的信息
			}
		//	printf("size = %d\n", strlen(temp));

			temp = memcpy(temp, temp + 2, strlen(temp)); 
			// 去掉前面的 '\n' '\r'
			temp = strtok(temp, "\n");
			// 去掉后面的 '\n'
			temp = strtok(temp, "\r");
			// 去掉后面的 '\r'
		//	printf("size1 = %d\n", strlen(temp));

			printf("电话号码:%s\n", tempNum);
					printf("接收到消息:%s\n", temp);
			strcpy(rbuf, temp);	 
			// 通过传参返回接收到的数据

			// if(strcmp(temp, "abc") == 0)
			// {
			// 	printf("==========1\n");
			// }
			//return temp;
		//}
	//	else
	//	{
	//		printf("垃圾短信:\n");
	//	}
	}

	return 0;
}
/*
function：
	写短信并发送 
parameter:
	char *cbuf: 发信息的内容
return:
*/
int GPRS_send(char *cbuf)
{	
	memset(gprs_module.sndBuf, 0, sizeof(gprs_module.sndBuf)); 
	// 清空之前发送的数据
	strcpy(gprs_module.sndBuf, "AT+CMGS=");
	// 发送短信的命令
	strcat(gprs_module.sndBuf, gprs_module.userNum);
	// 加 发送短信的号码
	strcat(gprs_module.sndBuf, "\r");

	strcat(gprs_module.sndBuf, cbuf);
	// 把发送的内容写进去
	strcat(gprs_module.sndBuf, "\032");
	// AT+CMGS=XXXXXXX<CF>***<Ctrl+Z> (***表示4内容)  <CF> 表示回车  <Ctrl+Z>  "\032"

	printf("send data: %s\n", gprs_module.sndBuf);
	// 打印发送的内容
	write(gprs_module.fd, gprs_module.sndBuf, strlen(gprs_module.sndBuf));
	// 把要发送的内容写入到文件描述符中
	//memset(cbuf, 0 , sizeof(cbuf)); 
	// 清空发送的内容
	//memset(gprs_module.recBuf, 0, sizeof(gprs_module.recBuf));
	return 1;
}

/*
function：
	设置端口属性(配置串口参数函数)
parameter:
	fd: 文件描述符
	nSpeed: 设置波特率
	nBits：数据位设置
	nEvent：奇偶校验位
	nStop：停止位
return:
*/
int set_opt(int fd, int nSpeed, int nBits, char nEvent, int nStop)
{	
	struct termios newtio, oldtio;
	// 通过设置termios类型的数据结构中的值和使用一小组函数调用，你就可以对终端接口进行控制

	if(tcgetattr(fd, &oldtio) != 0)
	// tcgetattr(fd, &oldtio) 保存原先有串口配置 
	{
		perror ("tcgetattr failed");
		return -1;
	}
	bzero(&newtio, sizeof(newtio)); 
	//先将新串口配置清0(对结构初始化)

	newtio.c_cflag |= CLOCAL | CREAD;
	// 激活选项CLOCAL和CREAD
	// 用于本地连接和接收使能
	// CLOCAL 忽略 modem 控制线
	// CREAD 只能接收字元
	// c_cflag 控制标识

	newtio.c_cflag &= ~CSIZE;
	// 设置数据位，需使用掩码设置
	// CSIZE 字符长度掩码。取值为 CS5, CS6, CS7, 或 CS8
	switch(nBits)
	{
		case 7:
			newtio.c_cflag |= CS7;
			break;
		case 8:
			newtio.c_cflag |= CS8;
			break;
	}

	
	switch(nEvent)
	// 设置奇偶校验位
	{	
		case 'O':
		// 奇数
			newtio.c_cflag |= PARENB;
			// PARENB 允许输出产生奇偶信息以及输入的奇偶校验
			newtio.c_cflag |= PARODD;
			// PARODD 输入和输出是奇校验
			newtio.c_iflag |= (INPCK | ISTRIP);
			// INPCK 启用输入奇偶检测
			// ISTRIP 去掉第八位
			break;
		case 'E':
		// 偶数
			newtio.c_iflag |= (INPCK | ISTRIP);
			// INPCK 启用输入奇偶检测
			// ISTRIP 去掉第八位
			newtio.c_cflag |= PARENB;
			// PARENB 允许输出产生奇偶信息以及输入的奇偶校验
			newtio.c_cflag &= ~PARODD;
			// ~PARODD 输入和输出是偶校验
			break;
		case 'N':
		// 无奇偶校验位
			newtio.c_cflag &= ~PARENB;
			// ~PARENB 不允许输出产生奇偶信息以及输入的奇偶校验
			break;
	}

	switch (nSpeed) 
	// 设置波特率
	{
		case 2400:
			cfsetispeed (&newtio, B2400);
			// 设置 termios 结构中存储的输入波特率为 nSpeed。如果输入波特率被设为0，实际输入波特率将等于输出波特率
			cfsetospeed (&newtio, B2400);
			// 设置 termios_p 指向的 termios 结构中存储的输出波特率为 nSpeed
			break;
		case 4800:
			cfsetispeed (&newtio, B4800);
			cfsetospeed (&newtio, B4800);
			break;
		case 9600:
			cfsetispeed (&newtio, B9600);
			cfsetospeed (&newtio, B9600);
			break;
		case 115200:
			cfsetispeed (&newtio, B115200);
			cfsetospeed (&newtio, B115200);
			break;
		case 460800:
			cfsetispeed (&newtio, B460800);
			cfsetospeed (&newtio, B460800);
			break;
		default:
			cfsetispeed (&newtio, B9600);
			cfsetospeed (&newtio, B9600);
			break;
	}

	if(nStop == 1)
	// 设置停止位
	{	
		newtio.c_cflag &= ~CSTOPB;
		// 停止位为 1
	}
	else if(nStop == 2)
	{
		newtio.c_cflag |= CSTOPB;
		// 停止位为 0
	}

	newtio.c_cc[VTIME] = 0;
	// 设置等待时间
	// c_cc 数组定义了特殊的控制字符
	// VTIME 设置最小接受字符
	newtio.c_cc[VMIN] = 0;
	// VMIN 处理为接收字符
	
	tcflush (fd, TCIFLUSH);
	// 丢弃要写入 引用的对象，但是尚未传输的数据，或者收到但是尚未读取的数据
	// TCIFLUSH 刷清输入队列

	if((tcsetattr(fd, TCSANOW, &newtio)) != 0)
	// tcsetattr 激活新配置
	// TCSANOW 不等数据传输完毕就立即改变属性
	{
		perror("com set error!\n");
		return -1;
	}
	printf("set done!\n");
	return 0;
}

/*
function：
	打开设备文件
parameter:
	fd: 文件描述符
	port: 端口
return:
	返回文件描述符中的个数
*/
int open_port(int fd, int port)
{
	if(port == 1)
	{	
		fd = open("/dev/ttyUSB0", O_RDWR|O_NOCTTY|O_NDELAY);
		// 接 USB0
		if(-1 == fd)
		{
			perror("Can't Open Serial Port");
			// 不能打开串口
			return -1;
		}
	}
	else if(port == 2)
	{   
		fd = open("/dev/ttySAC3", O_RDWR|O_NOCTTY|O_NDELAY);
		// 接串口 COM3 
		// O_RDWR 读写方式 
		// O_NOCTTY:通知linux系统，这个程序不会成为这个端口的控制终端.
        // O_NDELAY:通知linux系统不关心DCD信号线(DCD信号通常来自串口连结线的另一端)所处的状态(端口的另一端是否激活或者停止).

		if(-1 == fd)
        // 打开文件错误
		{
			perror("Can't Open Serial Port");
			// 不能打开串口
			return -1;
		}
	}

	if(fcntl(fd, F_SETFL, 0) < 0)
	//然后恢复串口的状态为阻塞状态，用于等待串口数据的读入，用fcntl函数:
	//F_SETFL：设置文件flag为0，即默认，即阻塞状态
	{
		printf("fcntl failed!\n");
	}
	else
	{
		printf("fcntl = %d\n", fcntl(fd, F_SETFL, 0));
	}

	if(isatty(STDIN_FILENO) == 0)
	// 接着测试打开的文件描述符是否应用一个终端设备，以进一步确认串口是否正确打开.
	{
		printf("isatty fail!\n");
	}
	else
	{
		printf("isatty success!\n");
	}
	printf("fd = %d\n", fd); 
	// 打印fd里的字的个数
	return fd;
}

/*
function：
	初始化GPRS所需要的数据更函数
parameter:
return:
*/
int GPRS_init(int argc, char const *argv[])
{
	memset(gprs_module.userNum, 0, sizeof(gprs_module.userNum)); 
	// 初始化清空 保存用户手机号码
	memset(gprs_module.recBuf, 0, sizeof(gprs_module.recBuf)); 
	// 初始化清空 接收到短信的缓冲区
	memset(gprs_module.sndBuf, 0, sizeof(gprs_module.sndBuf)); 
	// 初始化清空 发送短信的缓冲区

	
	strcpy(gprs_module.userNum, "18229876570"); 
	// strcpy(gprs_module.userNum, "18569499009"); 
	// strcpy(gprs_module.userNum, "15673133133");  
	// 设置用户固定的号码

	
	if((gprs_module.fd = open_port(gprs_module.fd, 2)) < 0)
	// 调用上面的open_port函数  打开设备文件
	// 接串口 COM3
	{
		perror("open_port error");  
		// 打印错误
		return -1;
	}

	if((set_opt(gprs_module.fd, 9600, 8, 'N', 1))<0)
	// 调用 配置串口参数函数
	{
		perror("set_opt error");  // 打印错误
		return;
	}

	write(gprs_module.fd, "AT\r", 3);
	// 测试AT命令(初始化AT命令)
	sleep(1); 
	GPRS_number(); 
	// 调用函数 从文件描述符fd中读数据到recBuf中，并计算读到的个数
	if(strstr(gprs_module.recBuf, "OK") == NULL) 
	// 判断是否接受到OK  
	{
		printf("the AT is fail!!\n");
	}
	else
	{
		printf("nreadthe AT is success recBuf=%s\n", gprs_module.recBuf);
	}

	write(gprs_module.fd, "AT+CNMI=1,1\r", 12);
	// 接收短信主动提示，初始化设置如下命令 "\r" 回车
	sleep(1); 
	GPRS_number(); 
	// 调用函数
	if(strstr(gprs_module.recBuf, "OK") == NULL) 
	// 判断是否接受到OK  
	{
		printf("the AT+CNMI=1,1 is fail!!\n");
	}
	else
	// 接收到"OK"消息
	{
		printf("the AT+CNMI=1,1 is success recBuf=%s\n", gprs_module.recBuf);
	}

	write(gprs_module.fd, "AT+CSMS=1\r", 10);
	// 接收短信从串口输出
	sleep(1); 
	GPRS_number(); 
	// 调用函数
	if(strstr(gprs_module.recBuf, "OK") == NULL) 
	// 判断是否接受到OK  
	{
		printf("the AT+CSMS=1 is fail!!\n");
	}
	else
	// 接收到"OK"消息
	{
		printf("the AT+CSMS=1 is success recBuf=%s\n", gprs_module.recBuf);
	}

	write(gprs_module.fd, "AT+CNMI=2,2\r", 12);
	// 接收短信不存入SIM卡初始化设置
	sleep(1); 
	GPRS_number(); 
	// 调用函数
	if(strstr(gprs_module.recBuf, "OK") == NULL) 
	// 判断是否接受到OK  
	{
		printf("the AT+CNMI=2,2 is fail!!\n");
	}
	else
	// 接收到"OK"消息
	{
		printf("the AT+CNMI=2,2 is success recBuf=%s\n", gprs_module.recBuf);
	}

	write(gprs_module.fd, "AT+CSMP=17,167,0,8\r", 19);
	// 接收短信不存入SIM卡初始化设置
	sleep(1); 
	GPRS_number(); 
	// 调用函数
	if(strstr(gprs_module.recBuf, "OK") == NULL) 
	// 判断是否接受到OK  
	{
		printf("the AT+CSMP=17,167,0,8 is fail!!\n");
	}
	else
	// 接收到"OK"消息
	{
		printf("the AT+CSMP=17,167,0,8 is success recBuf=%s\n", gprs_module.recBuf);
	}

	write(gprs_module.fd, "AT+CMGF=1\r", 10);
	// 接收短信不存入SIM卡初始化设置
	sleep(1); 
	GPRS_number(); 
	// 调用函数
	if(strstr(gprs_module.recBuf, "OK") == NULL) 
	// 判断是否接受到OK  
	{
		printf("the AT+CMGF=1 is fail!!\n");
	}
	else
	// 接收到"OK"消息
	{
		printf("the AT+CMGF=1 is success recBuf=%s\n", gprs_module.recBuf);
	}
	//memset(gprs_module.recBuf, 0, sizeof(gprs_module.recBuf));


	//memset(gprs_module.fd, 0, strlen(gprs_module.fd));
	//　清空接收短信的缓冲区

//	GPRS_send("wodetian");
	// 调用发送短信命令函数
	// 发送固定的短信 跳出send函数   如: exit
	//sleep(1);
/*	char* buff = NULL;
	while(1)
	{
		sleep(1);
		buff = GPRS_receive();
		printf("buff = %s\n", buff);
		if(strcmp(buff, "abc") == 0)
		{
			printf("==========1\n");
		}
		if(strcmp(buff, "exit") == 0)
		{
			break;
		}
	}
*/
	//GPRS_send("wodetian");
	

	return 0;
}