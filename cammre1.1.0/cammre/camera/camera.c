/**************************************************************
   文件：camera.c
    作者：吴泰儒
    日期：2016-6-9
    描述：摄像头拍照并压缩、拍视频并压缩的接口函数
    其他内容的说明：
    函数列表：主要函数及其功能
    1.
    历史修改记录：
    <作者>        <日期>        <描述>
    李峡          2016-6-9      建立文件

***************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <getopt.h> /* getopt_long() */
#include <fcntl.h> /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h> /* for videodev2.h */
#include <linux/videodev2.h>
#include <signal.h>
#include "simplified_jpeg_encoder.h"
//#include "avi.h"
#include "camera.h"
//#include "../internet/internet.h" 
//#include "../sd/sd.h"
/*
1. 打开设备文件。 int fd=open(”/dev/video0″,O_RDWR);
2. 取得设备的capability，看看设备具有什么功能，比如是否具有视频输入,或者音频输入输出等。VIDIOC_QUERYCAP,struct v4l2_capability
3. 选择视频输入，一个视频设备可以有多个视频输入。VIDIOC_S_INPUT,struct v4l2_input
4. 设置视频的制式和帧格式，制式包括PAL，NTSC，帧的格式个包括宽度和高度等。
VIDIOC_S_STD,VIDIOC_S_FMT,struct v4l2_std_id,struct v4l2_format
5. 向驱动申请帧缓冲，一般不超过5个。struct v4l2_requestbuffers
6. 将申请到的帧缓冲映射到用户空间，这样就可以直接操作采集到的帧了，而不必去复制。mmap
7. 将申请到的帧缓冲全部入队列，以便存放采集到的数据.VIDIOC_QBUF,struct v4l2_buffer
8. 开始视频的采集。VIDIOC_STREAMON
9. 出队列以取得已采集数据的帧缓冲，取得原始采集数据。VIDIOC_DQBUF
10. 将缓冲重新入队列尾,这样可以循环采集。VIDIOC_QBUF
11. 停止视频的采集。VIDIOC_STREAMOFF
12. 关闭视频设备。close(fd);


struct v4l2_requestbuffers reqbufs;//向驱动申请帧缓冲的请求，里面包含申请的个数
struct v4l2_capability cap;//这个设备的功能，比如是否是视频输入设备
struct v4l2_input input; //视频输入
struct v4l2_standard std;//视频的制式，比如PAL，NTSC
struct v4l2_format fmt;//帧的格式，比如宽度，高度等

struct v4l2_buffer buf;//代表驱动中的一帧
v4l2_std_id stdid;//视频制式，例如：V4L2_STD_PAL_B
struct v4l2_queryctrl query;//查询的控制
struct v4l2_control control;//具体控制的值

*/
#define DEVICE_NAME     "/dev/video0"   //设备文件名称和目录

#define CLEAR(x) memset (&(x), 0, sizeof (x))
#define CAP_WIDH  (640/2)
#define CAP_HIGHT (480/2)
/*
1. USERPTR， 顾名思义是用户空间指针的意思，应用层负责分配需要的内存空间，
然后以指针的形式传递给V4L2驱动层，V4L2驱动会把capture的内容保存到指针所指的空间

一般来说，应用层需要确保这个内存空间物理上是连续的（IPU处理单元的需求）
，在Android系统可以通过PMEM驱动来分配大块的连续物理内存。应用层在不需要的时候要负责释放申请的PMEM内存。
2. MMAP方式，内存映射模式，应用调用VIDIOC_REQBUFS ioctl分配设备buffers
，参数标识需要的数目和类型。这个ioctl也可以用来改变buffers的数据以及释放分配的内存，
当然这个内存空间一般也是连续的。在应用空间能够访问这些物理地址之前，
必须调用mmap函数把这些物理空间映射为用户虚拟地址空间。

虚拟地址空间是通过munmap函数释放的； 而物理内存的释放是通过VIDIOC_REQBUFS来实现的(设置参数buf count为(0)），
物理内存的释放是实现特定的，mx51 v4l2是在关闭设备时进行释放的。
*/
typedef enum {//图片提取方式枚举
    IO_METHOD_READ,//IO方法读写
    IO_METHOD_MMAP,//内存映射方式
    IO_METHOD_USERPTR,//空间指针
} io_method;

struct buffer {//图片信息结构体
    void * start;//图片首地址
    size_t length;//长度
};

static char * dev_name = NULL;//设备名指针
static io_method io = IO_METHOD_MMAP;//定义枚举io并赋予初始值为内存映射方式
static int fd = -1;//文件描述符
struct buffer * buffers = NULL;//文件缓冲区指针里面包含文件的头指针和文件大小
static unsigned int n_buffers = 0;//缓冲区的个数
static __u32 g_pixformat;//图片保存格式

FILE* avi_fp;

struct Camera_module Camera_struct;// 摄像组件 包含工作状态 和图片缓冲区 视频缓冲区

void init_camera_module(void)//初始化摄像组件
{
	Camera_struct.taskStatus = cameraFree;
	Camera_struct.photoBuf = NULL;
	Camera_struct.videoBuf = NULL;
	return;
}

static void errno_exit (const char * s)//检测错误机制，返回错误信息
{
    fprintf (stderr, "%s error %d, %s\n",
    s, errno, strerror (errno));
    exit (EXIT_FAILURE);
}
static int xioctl (int fd,int request,void * arg)//设置和查看设备文件的各种信息
{
    int r;
    do r = ioctl (fd, request, arg);
    while (-1 == r && EINTR == errno);
    return r;
}
/*
static int saveFile(const char *prefix ,const void *data, size_t size)//保存图片
{
    static int fileCnt = 0;   
    
    avi_add(avi_fp, (u8*)data, size);
    fileCnt++;
       
    return 0;
    
}

*/
static int  process_image (const void * p, int size)//处理图片
{
    int written;

    char *outbuf = (char *)malloc(size);//图片缓冲区大小
	
	switch(g_pixformat)
	{
		case V4L2_PIX_FMT_YUYV: //图片格式
        //编码缓冲区图片
			written=s_encode_image((uint8_t *)p,outbuf,1024,FORMAT_CbCr422,CAP_WIDH,CAP_HIGHT,size);
			break;
			
		case V4L2_PIX_FMT_YUV422P: 
			//written=s_encode_image(vd->framebuffer,buffer,1024,FORMAT_CbCr422p,vd->width,vd->height,vd->framesizeIn);
			break;
			
		case V4L2_PIX_FMT_RGB565:
			//do inplace conversion
			//RGB565_2_YCbCr420(vd->framebuffer,vd->framebuffer,vd->width,vd->height);
			//written=s_encode_image(vd->framebuffer,buffer,1024,FORMAT_CbCr422,vd->width,vd->height,vd->framesizeIn);
			break;
			
		case V4L2_PIX_FMT_RGB24:
			//do inplace conversion
			//RGB24_2_YCbCr422(vd->framebuffer,vd->framebuffer,vd->width,vd->height);
			//written=s_encode_image(vd->framebuffer,buffer,1024,FORMAT_CbCr422,vd->width,vd->height,vd->framesizeIn);
			break;
			
		case V4L2_PIX_FMT_RGB32:
			//do inplace conversion
			//RGB32_2_YCbCr420(vd->framebuffer,vd->framebuffer,vd->width,vd->height);
			//written=s_encode_image(vd->framebuffer,buffer,1024,FORMAT_CbCr420,vd->width,vd->height,vd->framesizeIn);
			break;
			
		case V4L2_PIX_FMT_YVU420://WARNING: this is not properly implemented yet
		case V4L2_PIX_FMT_Y41P: //don't know how to handle
		case V4L2_PIX_FMT_GREY:
		default:
			//written=s_encode_image(vd->framebuffer,buffer,1024,FORMAT_CbCr400,vd->width,vd->height,vd->framesizeIn);
			break;
	}
    
    //if(SD_struct.sdStatus == sdOpen)//SD卡的状态
    //{
        //time_t  tt;//以时间命名jpg图片
        char jpg_name[] = "current";
	    char filename[64];//文件目录及文件名
		FILE* sdFp;//打开文件获得的文件指针
		//time(&tt);
		memset(filename, 0, sizeof(filename));//初始化filename指针
		sprintf(filename, "/mnt/%s.jpg", jpg_name);//连接字符串
		sdFp = fopen(filename,"wb");
		fwrite(outbuf, size,1, sdFp);//读文件指针到outbuf
//		save_to_SD(filename);
    //}
    //Ôö¼Ó  8-3 10:30 ÀîÏ¿
//    printf("writen =%d\n", written);

    /* Ôö¼Ó 8-3 14:35 ÀîÏ¿ */
    /*
   if(Internet_struct.interTask == video)//因特网调用模块
	{
        Camera_struct.taskStatus = videoFinish;
		Internet_struct.videoBuf = outbuf;
		Internet_struct.videoLen = written;
	}
	else
	{
	    Camera_struct.taskStatus = cameraFree;
		Internet_struct.videoBuf = NULL;
		Internet_struct.videoLen = 0;
	    free(outbuf);
	}*/

    return 0;
    
    //return saveFile("exmaple", outbuf, written);    //É¾³ý 8-3 15:15
    
}
static int  read_frame (void)//读取一帧内容
{
    struct v4l2_buffer buf;//设备文件内部结构体
    unsigned int i;
    switch (io)//根据io不用的模式做相应动作,模式介绍请看前面io枚举定义那里
    {
        case IO_METHOD_READ:
            if (-1 == read (fd, buffers[0].start, buffers[0].length))//读文件读到buffers
            {
                switch (errno)
                {
                    case EAGAIN:
                        return 0;
                    case EIO:
                        /* Could ignore EIO, see spec. */
                        /* fall through */
                        default:
                            errno_exit ("read");
                }
            }
            printf("%s\n", buffers[0].start);
            process_image (buffers[0].start, buf.bytesused);//把读到的指针头给处理函数处理
            break;
            
            case IO_METHOD_MMAP:
                CLEAR (buf);
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;//图片类型
                buf.memory = V4L2_MEMORY_MMAP;//内存开辟方式 使用映射的方式
                
                if (-1 == xioctl (fd, VIDIOC_DQBUF, &buf))//出列采集的帧缓冲
                {
                    switch (errno)
                    {
                        case EAGAIN:
                            return 0;
                        case EIO:
                            /* Could ignore EIO, see spec. */
                            /* fall through */
                        default:
                            return;
                    }
                }
                assert (buf.index < n_buffers);//该函数是一个错误机制处理函数，如果为假则终止函数运行
                process_image (buffers[buf.index].start,buf.bytesused);
                if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
                    return;
                break;
                
                case IO_METHOD_USERPTR:
                    CLEAR (buf);
                    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                    buf.memory = V4L2_MEMORY_USERPTR;
                    if (-1 == xioctl (fd, VIDIOC_DQBUF, &buf))
                    {
                        switch (errno)
                        {
                            case EAGAIN:
                                return 0;
                            case EIO:
                                /* Could ignore EIO, see spec. */
                                /* fall through */
                                default:
                                return;
                        }
                    }
                    for (i = 0; i < n_buffers; ++i)
                    if (buf.m.userptr ==(unsigned long) buffers[i].start
                    &&  buf.length == buffers[i].length)
                        break;
                        
                    assert (i < n_buffers);
                    process_image ((void *) buf.m.userptr, buf.bytesused);
                    
                    if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
                        return;
    
                    break;
    }

    return 1;
}

void mainloop (void)//函数跳转，主要作用是
{

	//if(Camera_struct.taskStatus != videotaping)
	//{
	//    return;
	//}
	
    for (;;)
    {
        fd_set fds;
        struct timeval tv;
        int r;
        FD_ZERO (&fds);//将指定的文件描述符集清空
        FD_SET (fd, &fds);//在文件描述符集合中增加一个新的文件描述符

        /* Timeout. */
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        r = select (fd + 1, &fds, NULL, NULL, &tv);//判断是否可读（即摄像头是否准备好），tv是定时
        if (-1 == r)
        {
            if (EINTR == errno)
                continue;
            return;
        }
        
        if (0 == r)
        {
            fprintf (stderr, "select timeout\n");
            return;
        }
        if (read_frame ())//如果可读，执行read_frame函数
            break;
        /* EAGAIN - continue select loop. */
    }

	return;
}

static void stop_capturing (void)//停止拍照
{
    enum v4l2_buf_type type;
    switch (io)
    {
        case IO_METHOD_READ:
            /* Nothing to do. */
            break;

        case IO_METHOD_MMAP:
        case IO_METHOD_USERPTR:
            type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            if (-1 == xioctl (fd, VIDIOC_STREAMOFF, &type))
                return;
            break;
    }
}

static void start_capturing (void)//开始拍照
{
    unsigned int i;
    enum v4l2_buf_type type;
    switch (io)
    {
        case IO_METHOD_READ:
            /* Nothing to do. */
            break;
        case IO_METHOD_MMAP:
            for (i = 0; i < n_buffers; ++i)
            {
                struct v4l2_buffer buf;
                CLEAR (buf);
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                buf.index = i;
                if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
                    return;
            }

            type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            if (-1 == xioctl (fd, VIDIOC_STREAMON, &type))//开始捕捉图像数据
                return;
            break;
        case IO_METHOD_USERPTR:     
            for (i = 0; i < n_buffers; ++i)
            {
                struct v4l2_buffer buf;
                CLEAR (buf);
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_USERPTR;
                buf.index = i;
                buf.m.userptr = (unsigned long) buffers[i].start;
                buf.length = buffers[i].length;
                if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
                return;
            }
            
            type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            if (-1 == xioctl (fd, VIDIOC_STREAMON, &type))
                return;
            break;
    }
}

static void uninit_device (void)//释放设备
{
    unsigned int i;
    switch (io)
    {
        case IO_METHOD_READ:
            free (buffers[0].start);
            break;
        case IO_METHOD_MMAP:
            for (i = 0; i < n_buffers; ++i)
            if (-1 == munmap (buffers[i].start, buffers[i].length))
                return;
            break;
        case IO_METHOD_USERPTR:
            for (i = 0; i < n_buffers; ++i)
                free (buffers[i].start);
            break;
    }
    
    free (buffers);
    
}

static void init_read (unsigned int buffer_size)//初始化读结构体
{
    buffers = calloc (1, sizeof (*buffers));//开辟内存并且初始化(赋0)
    if (!buffers)
    {
        fprintf (stderr, "Out of memory\n");
        return;
    }
    
    buffers[0].length = buffer_size;
    buffers[0].start = malloc (buffer_size);
    if (!buffers[0].start)
    {
        fprintf (stderr, "Out of memory\n");
        return;
    }
}
static void init_mmap (void)//初始化内存映射
{
    struct v4l2_requestbuffers req;
    CLEAR (req);
    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;//图片类型
    req.memory = V4L2_MEMORY_MMAP;//内存开辟方式
    if (-1 == xioctl (fd, VIDIOC_REQBUFS, &req))
    {
        if (EINVAL == errno)
        {
            fprintf (stderr, 
            "%s does not support ""memory mapping\n", dev_name);
            return;
        }
        else
        {
            return;
        }
    }
    
    if (req.count < 2)
    {
        fprintf (stderr, "Insufficient buffer memory on %s\n",
                dev_name);
        return;
    }
    
    buffers = calloc (req.count, sizeof (*buffers));

    if (!buffers)
    {
        fprintf (stderr, "Out of memory\n");
        return;
    }
    
    for (n_buffers = 0; n_buffers < req.count; ++n_buffers)//映射内存
    {
        struct v4l2_buffer buf;
        CLEAR (buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n_buffers;
        if (-1 == xioctl (fd, VIDIOC_QUERYBUF, &buf))
           return;
            
        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start =  mmap ( NULL /* start anywhere */,
                                           buf.length,
                                           PROT_READ | PROT_WRITE /* required */,
                                           MAP_SHARED /* recommended */,
                                           fd, buf.m.offset);
        if (MAP_FAILED == buffers[n_buffers].start)
            return;
    }
    
}


static void init_userp (unsigned int buffer_size)//初始化空间指针
{
    struct v4l2_requestbuffers req;//设备驱动内部结构体
    unsigned int page_size;
    page_size = getpagesize ();
    buffer_size = (buffer_size + page_size - 1) & ~(page_size - 1);
    CLEAR (req);
    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_USERPTR;
    if (-1 == xioctl (fd, VIDIOC_REQBUFS, &req))
    {
        if (EINVAL == errno)
        {
            fprintf (stderr, "%s does not support "
                    "user pointer i/o\n", dev_name);
            return;
        }
        else
        {
           return;
        }
    }
    
    buffers = calloc (4, sizeof (*buffers));
    if (!buffers)
    {
        fprintf (stderr, "Out of memory\n");
        return;
    }
    
    for (n_buffers = 0; n_buffers < 4; ++n_buffers)
    {
        buffers[n_buffers].length = buffer_size;
        buffers[n_buffers].start = memalign (/* boundary */ page_size,
                                            buffer_size);
        if (!buffers[n_buffers].start)
        {
            fprintf (stderr, "Out of memory\n");
            return;
        }
    }
    
}

static void init_device (void)//初始化设备
{
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
    unsigned int min;
    if (-1 == xioctl (fd, VIDIOC_QUERYCAP, &cap))
    {
        if (EINVAL == errno)
        {
            fprintf (stderr, "%s is no V4L2 device\n",dev_name);
            return;
        }
        else
        {
            return;
        }
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
        fprintf (stderr, "%s is no video capture device\n",dev_name);
        return;
    }
    
    switch (io)
    {
        case IO_METHOD_READ:
            if (!(cap.capabilities & V4L2_CAP_READWRITE))
            {
                fprintf (stderr, "%s does not support read i/o\n",dev_name);
                return;
            }
        break;
        
        case IO_METHOD_MMAP:
        case IO_METHOD_USERPTR:
            if (!(cap.capabilities & V4L2_CAP_STREAMING))
            {
                fprintf (stderr, "%s does not support streaming i/o\n",dev_name);
                return;
            }
        break;
    }
    
    /* Select video input, video standard and tune here. */
    CLEAR (cropcap);
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (0 == xioctl (fd, VIDIOC_CROPCAP, &cropcap))
    {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect; /* reset to default */
        if (-1 == xioctl (fd, VIDIOC_S_CROP, &crop))
        {
            switch (errno)
            {
                case EINVAL:
                    /* Cropping not supported. */
                    break;
                default:
                    /* Errors ignored. */
                break;
             }
        }   
    }
    else
    {
        /* Errors ignored. */
    }
    
    CLEAR (fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = CAP_WIDH;
    fmt.fmt.pix.height = CAP_HIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    //fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
    if (-1 == xioctl (fd, VIDIOC_S_FMT, &fmt))
    perror("VIDIOC_S_FMT"); ;//errno_exit ("VIDIOC_S_FMT");


    if(V4L2_PIX_FMT_YVU420 == fmt.fmt.pix.pixelformat)
    {
        printf("support FMT_YVU420\n");
    }
    else if(V4L2_PIX_FMT_JPEG == fmt.fmt.pix.pixelformat)
    {
        printf("support FMT_JPEG\n");
    }
    else if(V4L2_PIX_FMT_RGB24 == fmt.fmt.pix.pixelformat)
    {
        printf("support FMT_RGB24\n");
    }
    else if(V4L2_PIX_FMT_YUYV == fmt.fmt.pix.pixelformat)
    {
        printf("support FMT_YUYV\n");
    }

    g_pixformat = fmt.fmt.pix.pixelformat;
    
    
    
    
    /* Note VIDIOC_S_FMT may change width and height. */
    /* Buggy driver paranoia. */
    min = fmt.fmt.pix.width * 2;
    if (fmt.fmt.pix.bytesperline < min)
        fmt.fmt.pix.bytesperline = min;
        
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if (fmt.fmt.pix.sizeimage < min)
        fmt.fmt.pix.sizeimage = min;
        
    switch (io)
    {
        case IO_METHOD_READ:
            init_read (fmt.fmt.pix.sizeimage);
            break;
        case IO_METHOD_MMAP:
            init_mmap ();
            break;
        case IO_METHOD_USERPTR:
            init_userp (fmt.fmt.pix.sizeimage);
            break;
    }
    
}

static void close_device (void)//关闭设备
{
    if (-1 == close (fd))
        return;
    fd = -1;
}

static void open_device (void)//打开设备
{
    struct stat st;
    if (-1 == stat (DEVICE_NAME, &st))
    {
        fprintf ( stderr, "Cannot identify '%s': %d, %s\n",
                  DEVICE_NAME, errno, strerror (errno));  
       return;
    }
    
    if (!S_ISCHR (st.st_mode))
    {
        fprintf (stderr, "%s is no device\n", DEVICE_NAME);
        return;
    }
    
    fd = open (DEVICE_NAME, O_RDWR /* required */ | O_NONBLOCK, 0);
    if (-1 == fd)
    {
        fprintf (stderr, "Cannot open '%s': %d, %s\n",
        DEVICE_NAME, errno, strerror (errno));
        exit(-1);
    }
    
}
static void usage (FILE * fp,int argc,char ** argv)
{
    fprintf (fp,
    "Usage: %s [options]\n\n"
    "Options:\n"
    "-d | --device name Video device name [/dev/video]\n"
    "-h | --help Print this message\n"
    "-m | --mmap Use memory mapped buffers\n"
    "-r | --read Use read() calls\n"
    "-u | --userp Use application allocated buffers\n"
    "",
    argv[0]);
}

static const char short_options [] = "d:hmru";
static const struct option
long_options [] = {
{ "device", required_argument, NULL, 'd' },
{ "help", no_argument, NULL, 'h' },
{ "mmap", no_argument, NULL, 'm' },
{ "read", no_argument, NULL, 'r' },
{ "userp", no_argument, NULL, 'u' },
{ 0, 0, 0, 0 }
};

void init_camera(void)//初始化摄像头
{
    open_device();
    init_device();
    start_capturing();
}

void close_camera(void)//关闭摄像头
{
    stop_capturing();
    uninit_device ();
    close_device ();
}
/*
int get_avi(const char* filename)
{
    dev_name = "/dev/video0";
    avi_fp = fopen(filename,"wb");
    avi_start(avi_fp);

    open_device();
    init_device();
    start_capturing();
	

//    mainloop ();

    return 0;
}

void end_avi(void)
{
    stop_capturing();
	avi_end(avi_fp, CAP_WIDH, CAP_HIGHT,3);
    fclose(avi_fp);
    uninit_device ();
    close_device ();
}


int main(int argc, char** argv)
{
	printf("start\n");
	init_camera_module();
	init_camera();
	mainloop();/
	close_camera();
	printf("close\n");

    get_avi("li");
	mainloop();
	end_avi();
    return 0;
   
}
*/