/**************************************************************
       文件：SD.c
	作者：李达、李峡
	日期：2012-8-1
	描述：
	函数列表：主要函数及其功能
	1.
	历史修改记录：
	<作者>        <日期>        <描述>
         李达          2012-8-1       建立文件
         李峡          2012-8-3       修改文件
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