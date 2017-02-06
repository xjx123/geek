#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <mach/hardware.h>
#include <mach/gpio.h>
#include <mach/regs-gpio.h>
#include <plat/gpio-cfg.h>

#define NUM 4
#define DEVICE_NAME "tmp_out"  // 设备名
static int value = 36;
 ssize_t my_read (struct file *filp, char __user *buf,
		size_t count, loff_t *f_pos) ;
static unsigned int mytemp[4] = {  // 定义4个端口
	S5PV210_GPH2(0),
	S5PV210_GPH2(1),
	S5PV210_GPH2(2),
	S5PV210_GPH2(3)
};

// 温度传感器的初始化
void temp_reset(void)
{
    s3c_gpio_cfgpin(S5PV210_GPH2(0), S3C_GPIO_OUTPUT);  //设置总线端为GPH2(0) 并使端口为输出模式
    gpio_set_value(S5PV210_GPH2(0), 1);    // 给高电平
    udelay(100); // 100 微秒  下一次操作需要的延时



    gpio_set_value(S5PV210_GPH2(0), 0);  // 低电平
    udelay(600); // 600 微秒   大于480us 
    gpio_set_value(S5PV210_GPH2(0), 1); // 高电平



    udelay(100);  // 100微秒  缓冲时间
    s3c_gpio_cfgpin(S5PV210_GPH2(0), S3C_GPIO_INPUT);  // 输入模式
}


void temp_write_byte(unsigned char data)
{
    int i;

    s3c_gpio_cfgpin(S5PV210_GPH2(0), S3C_GPIO_OUTPUT);  //设置总线端为GPH2(0) 并使端口为输出模式

    // 把每一位写进去
    for (i = 0; i < 8; ++i)
    {
        gpio_set_value(S5PV210_GPH2(0), 0);
        udelay(1);

        // 判断这一位是否为'1'
        if (data & 0x01)
        {
            gpio_set_value(S5PV210_GPH2(0), 1);
        }
        udelay(60);
        gpio_set_value(S5PV210_GPH2(0), 1);
        udelay(15);
        data >>= 1;
    }
    gpio_set_value(S5PV210_GPH2(0), 1);
}

unsigned char temp_read_byte(void)
{
    int i;
    unsigned char ret = 0;

    for (i = 0; i < 8; ++i)
    {
       s3c_gpio_cfgpin(S5PV210_GPH2(0), S3C_GPIO_OUTPUT);  //设置总线端为GPH2(0) 并使端口为输出模式
        gpio_set_value(S5PV210_GPH2(0), 0); // 给个低脉冲
        udelay(1);  // 1微妙后拉高脉冲
        gpio_set_value(S5PV210_GPH2(0), 1);  // 给个高脉冲   
        // 从高电平

        s3c_gpio_cfgpin(S5PV210_GPH2(0), S3C_GPIO_INPUT);  // 设置为输入模式
        ret >>= 1;  // 右移1位
        if (gpio_get_value(S5PV210_GPH2(0))) // 获取总线的值   如果这个值为1
        {
            ret |= 0x80;     // 则把ret高位置1
        }
        udelay(60); // 读时序必须大于60us
    }
    s3c_gpio_cfgpin(S5PV210_GPH2(0), S3C_GPIO_OUTPUT);  //设置总线端为GPH2(0) 并使端口为输出模式
    return ret;
}


 ssize_t my_read (struct file *filp, char __user *buf,size_t count, loff_t *f_pos) 
{
	unsigned char low, high;
	unsigned char buff[6] ={0x00,0x00,0x00,0x00,0x00,0x00};
	int err;
	static int smoke = 0 ;	//烟雾
	static int light = 0 ;	//红外
	static int vibrate = 0; 	//振动
	//static int temp = 0 ;		//温度
	s3c_gpio_cfgpin(S5PV210_GPH2(1), S3C_GPIO_INPUT);
	smoke = gpio_get_value(S5PV210_GPH2(1));

	s3c_gpio_cfgpin(S5PV210_GPH2(2), S3C_GPIO_INPUT);
	light = gpio_get_value(S5PV210_GPH2(2));

	s3c_gpio_cfgpin(S5PV210_GPH2(3), S3C_GPIO_INPUT);
	vibrate = gpio_get_value(S5PV210_GPH2(3));

    temp_reset();  // 初始化
    udelay(420);  // 延时给温度寄存器一个缓冲时间
    temp_write_byte(0xcc); // 忽略ROM指令  跳过它  不用它
    temp_write_byte(0x44); // 温度转换指令  18b20取值到寄存器里（16进制）

    mdelay(750);  // 等750毫秒   转换温度需要的时间
    temp_reset();  // 初始化（复位）
    udelay(400); // 缓冲
    temp_write_byte(0xcc); // 跳过
    temp_write_byte(0xbe); // 读指令 读温度

    low = temp_read_byte();
    high = temp_read_byte();

    buff[0] = low / 16 + high * 16; //  整数部分是前12位

    buff[1] = (low & 0x0f) * 10 / 16 + (high & 0x0f) * 100 / 16 % 10;

    buff[2] = smoke;
    buff[3] = light;
    buff[4] = vibrate;
    err = copy_to_user(buf, &buff, sizeof(buff));
    // 如果数据拷贝成功，则返回零；否则，返回没有拷贝成功的数据字节数
    if (err)
    {
    	printk("inout ->>>>>file\n");
    }
    else
    {
    	printk("inout ->>>>>sccuss\n");
    }
    return 0;

}

struct file_operations fops = {
	.owner = THIS_MODULE,
	.read = my_read,
};

struct miscdevice myleds_misc ={
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEVICE_NAME,
	.fops = &fops,
};

static int __init temp_init(void)
{
	int i; // 循环变量
	int ret;
	// 0.请求控制引脚
	for(i=0;i<NUM;i++)
	{
		ret = gpio_request(mytemp[i],"temp");
		if(ret)
		{
			
			return -EFAULT;
		}
		// 1.设置引脚模式
		s3c_gpio_cfgpin(mytemp[i],S3C_GPIO_INPUT); // 设置引脚为输入模式
		
	}

	// 3.注册混杂字符设备
	ret = misc_register(&myleds_misc);
	if(ret){
		printk("error\n");
	} else{
		printk("sccuss\n");
	}

	return ret;
}


// 注销
static void __exit temp_exit(void)
{
	int i;
	for(i=0;i<NUM;i++)
	{
		gpio_free(mytemp[i]);
	}
	misc_deregister(&myleds_misc);
}

module_init(temp_init);
module_exit(temp_exit);
MODULE_LICENSE("GPL");
