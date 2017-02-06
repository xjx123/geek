/**************************************************************
       文件：avi.h
	作者：李峡
	日期：2012-7-30
	描述：
	其他内容的说明：
	函数列表：主要函数及其功能
	1.
	历史修改记录：
	<作者>        <日期>        <描述>
         李峡          2012-7-30       建立文件
         李达		  2012-8-1         修改文件
***************************************************************/


#ifndef AVI_H
#define AVI_H

//#include <unistd.h>

#include <string.h>
#include <stdio.h>
#include <sys/types.h>



typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned u32;



// function prototypes

void resize_gmap();

void  _wbzero(void *, int );

extern void  avi_start(FILE * fp);
extern void  avi_add(FILE * fp, u8 *buf, int size);
extern void  avi_end(FILE * fp, int width, int height, int fps);
extern void  fprint_quartet(FILE * fp, unsigned int i);

// the following structures are ordered as they appear in a typical AVI

struct riff_head 
{
    char riff[4];               // chunk type = "RIFF"
    u32 size;                   // chunk size
    char avistr[4];             // avi magic = "AVI "
};

// the avih chunk contains a number of list chunks

struct avi_head
 {
    char avih[4];               // chunk type = "avih"
    u32 size;                   // chunk size
    u32 time;                   // microsec per frame == 1e6 / fps
    u32 maxbytespersec;         // = 1e6*(total size/frames)/per_usec)
    u32 pad;                    // pad = 0
    u32 flags;                  // e.g. AVIF_HASINDEX
    u32 nframes;                // total number of frames
    u32 initialframes;          // = 0
    u32 numstreams;             // = 1 for now (later = 2 because of audio)
    u32 suggested_bufsize;      // = 0 (no suggestion)
    u32 width;                  // width
    u32 height;                 // height
    u32 reserved[4];            // reserved for future use = 0
};


// the LIST chunk contains a number (==#numstreams) of stream chunks

struct list_head 
{
    char list[4];               // chunk type = "LIST"
    u32 size;
    char type[4];
};


struct dmlh_head 
{
    char dmlh[4];               // chunk type dmlh
    u32 size;                   // 4
    u32 nframes;                // number of frames
};


struct stream_head 
{
    char strh[4];               // chunk type = "strh"
    u32 size;                   // chunk size
    char vids[4];               // stream type = "vids"
    char codec[4];              // codec name (for us, = "MJPG")
    u32 flags;                  // contains AVIT_F* flags
    u16 priority;               // = 0
    u16 language;               // = 0
    u32 initialframes;          // = 0
    u32 scale;                  // = usec per frame
    u32 rate;                   // 1e6
    u32 start;                  // = 0
    u32 length;                 // number of frames
    u32 suggested_bufsize;      // = 0
    u32 quality;                // = 0 ?
    u32 samplesize;             // = 0 ?
};


struct db_head
 {
    char db[4];                 // "00db"
    u32 size;
};

// a frame chunk contains one JPEG image

struct frame_head 
{
    char strf[4];               // chunk type = "strf"
    u32 size;                   // sizeof chunk (big endian)    ?
    u32 size2;                  // sizeof chunk (little endian) ?
    u32 width;
    u32 height;
    u16 planes;                 // 1 bitplane
    u16 bitcount;               // 24 bpl
    char codec[4];              // MJPG (for us)
    u32 unpackedsize;           // = 3*w*h
    u32 r1;                     // reserved
    u32 r2;                     // reserved
    u32 clr_used;               // reserved
    u32 clr_important;          // reserved
};

struct idx1_head 
{
    char idx1[4];               // chunk type = "idx1"
    u32 size;                   // chunk size
};

#ifdef __cplusplus
}
#endif

#endif
