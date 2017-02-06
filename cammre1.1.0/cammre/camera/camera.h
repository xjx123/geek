/**************************************************************
       �ļ���camera.h
	���ߣ���Ͽ
	���ڣ�2012-7-27
	����������ͷ���ղ�ѹ��������Ƶ��ѹ���Ľӿں���
	�������ݵ�˵����
	�����б���Ҫ�������书��
	1.
	��ʷ�޸ļ�¼��
	<����>        <����>        <����>
         ��Ͽ          2012-7-28       �����ļ�

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