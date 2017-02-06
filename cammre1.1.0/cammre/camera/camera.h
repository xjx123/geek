/**************************************************************
       文件：camera.h
	作者：李峡
	日期：2012-7-27
	描述：摄像头拍照并压缩、拍视频并压缩的接口函数
	其他内容的说明：
	函数列表：主要函数及其功能
	1.
	历史修改记录：
	<作者>        <日期>        <描述>
         李峡          2012-7-28       建立文件

***************************************************************/
#ifndef  __CAMERA_H__
#define  __CAMERA_H__
#include "avi.h"

enum task_status{ cameraFree, 
                  photographing, videotaping, 
                  photoFinish, videoFinish };

struct Camera_module
{
    enum task_status taskStatus;
	
	char* photoBuf;
	char* videoBuf;
};

extern struct Camera_module Camera_struct;

extern void init_camera_module(void);
extern void init_camera(void);
extern void close_camera(void);
extern void mainloop(void);

extern int get_avi(const char* filename);
extern void end_avi(void);
extern void avi_add(FILE * fp,u8 * buf,int size);
extern void avi_start(FILE * fp);

#endif