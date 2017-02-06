#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <mach/gpio.h>
#include <mach/regs-gpio.h>
#include <plat/gpio-cfg.h>

#define DEVICE_NAME "relay"  // 设备名


static long mini210_relay_ioctl(struct file *filp, unsigned int cmd)
{
	if (cmd > 0 )
	{
		cmd = 1;
	}
	else
	{
		cmd = 0;
	}

	gpio_set_value(S5PV210_GPD1(0),cmd);

	return 0;
}

struct file_operations fops = 
{
	.owner = THIS_MODULE,
	.unlocked_ioctl	= mini210_relay_ioctl,
};

struct miscdevice myrelay_misc ={
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEVICE_NAME,
	.fops = &fops,
};

static int __init relay_init(void)
{
	int ret;

	ret=gpio_request(S5PV210_GPD1(0),"rel");
	// 请求控制引脚

	if (ret)
	{

		printk(" error:%d!\n",ret);

		return -EFAULT;
			
	}

	s3c_gpio_cfgpin(S5PV210_GPD1(0),S3C_GPIO_OUTPUT);
	// 设置为输出模式

	gpio_set_value(S5PV210_GPD1(0),0);
	
	ret = misc_register(&myrelay_misc);
	// 注册混杂设备字符

	if (ret)
    {
    	printk("inout ->>>>>file\n");
    }
    else
    {
    	printk("inout ->>>>>sccuss\n");
    }

	return ret;
}

static void __exit relay_exit(void)
{
	
	gpio_free(S5PV210_GPD1(0));

	misc_deregister(&myrelay_misc);
	// 注销混杂设备字符
}

module_init(relay_init);
module_exit(relay_exit);
MODULE_LICENSE("GPL");