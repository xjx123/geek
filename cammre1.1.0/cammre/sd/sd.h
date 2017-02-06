/**************************************************************
       �ļ���SD.c
	���ߣ�����Ͽ
	���ڣ�2012-8-1
	������
	�����б���Ҫ�������书��
	1.
	��ʷ�޸ļ�¼��
	<����>        <����>        <����>
         ���          2012-8-1       �����ļ�
         ��Ͽ          2012-8-3       �޸��ļ�
***************************************************************/
#ifndef  __SD_H__
#define  __SD_H__

enum SD_status{sdOpen, sdClose};
enum SD_work{sdOn, sdOff};

struct SD_module
{
    enum SD_work sdWork;
    enum SD_status sdStatus;
};
extern struct SD_module SD_struct;
extern void SD_init(void);
extern void SD_save(char* filename);
extern void SD_close(void);
extern void SD_listen(void);

#endif