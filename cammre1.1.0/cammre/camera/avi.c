
/**************************************************************
       文件：avi.c
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "avi.h"
//#include "../internet/internet.h"   //删除 8-3 14:41 李峡
#include "../camera/camera.h"
#include "../sd/sd.h"

// header flags

const u32 AVIF_HASINDEX = 0x00000010;    /* index at end of file */
const u32 AVIF_MUSTUSEINDEX=0x00000020;
const u32 AVIF_ISINTERLEAVED=0x00000100;
const u32 AVIF_TRUSTCKTYPE=0x00000800;
const u32 AVIF_WASCAPTUREFILE=0x00010000;
const u32 AVIF_COPYRIGHTED=0x00020000;


int nframes;
int totalsize;
//unsigned int* sizes;


typedef struct g_map_
{
    int      *map_ptr;
    int      size;
}gmap_t;

gmap_t gmap;

#define MAP_AVI	(gmap.map_ptr)


void resize_gmap()
{
    realloc(gmap.map_ptr, gmap.size * sizeof(int) * 2);
    gmap.size = gmap.size * 2;
}

void _bzero(void *s, int n)
{
	memset(s,0,n);
}


void fprint_quartet(/*int fd*/FILE * fp, unsigned int i)
{
    char data[4];
	int rt = 0;

    data[0] = (char) i%0x100;
    i /= 0x100;
    data[1] = (char) i%0x100;
    i /= 0x100;
    data[2] = (char) i%0x100;
    i /= 0x100;
    data[3] = (char) i%0x100;

    /*write( fd, &data, 4 );*/
	rt = fwrite(&data, 4, 1, fp);
    if(rt != 1)
    printf(" fprintf_quartet failed!  \n ");
}

// start writing an AVI file

 void avi_start(FILE * fp/*, int frames*/)/*__stdcall*/ 
{
	int ofs;
	//FILE * fp = NULL;
	//fp = fopen(filename, "wb");
	///*if(!fp) return fp;*/

    ofs = sizeof(struct riff_head)+
              sizeof(struct list_head)+
              sizeof(struct avi_head)+
              sizeof(struct list_head)+
              sizeof(struct stream_head)+
              sizeof(struct frame_head)+
              sizeof(struct list_head)+
              sizeof(struct dmlh_head)+
              sizeof(struct list_head);

    //printf( "avi_start: frames = %d\n", frames );

   /* lseek(fd, ofs, SEEK_SET);*/
    fseek(fp,ofs, SEEK_SET);

    nframes = 0;
    totalsize = 0;

	gmap.size = 100;
    gmap.map_ptr = malloc(gmap.size * sizeof(int));

    //sizes = (unsigned int*) calloc( sizeof(unsigned int), frames );   // hold size of each frame
	
    //return fp;
    return;
}

// add a jpeg frame to an AVI file
void  avi_add(/*int fd*/FILE * fp,  u8 *buf, int size)/*__stdcall*/
{
    struct db_head db = {"00db", 0};

 //   printf( "avi_add: nframes = %d, totalsize = %d, size = %d\n", nframes, totalsize, size );


    while( size%4 ) size++; // align 0 modulo 4*/
    db.size = size;

    /* write( fd, &db, sizeof(db) );*/
//    fwrite( &db, sizeof(db), 1, fp);    

    /*write( fd, buf, size );*/
//    fwrite( buf, size, 1, fp);

	
    //sizes[nframes] = size;

	if(nframes >= gmap.size)
	{
		resize_gmap();
	}

	MAP_AVI[nframes] = size;
	nframes++;

    totalsize += size;  // total frame size
//    free(buf);
}

// finish writing the AVI file - filling in the header
  void  avi_end(/*int fd*/FILE * fp, int width, int height, int fps)/*__stdcall*/
{
    struct idx1_head idx = {"idx1", 16*nframes };
    struct db_head db = {"00db", 0};
    struct riff_head rh = { "RIFF", 0, "AVI "};
    struct list_head lh1 = {"LIST", 0, "hdrl"};
    struct avi_head ah;
    struct list_head lh2 = {"LIST", 0, "strl"};
    struct stream_head sh;
    struct frame_head fh;
    struct list_head lh3 = {"LIST", 0, "odml" };
    struct dmlh_head dh = {"dmlh", 4, nframes };
    struct list_head lh4 = {"LIST", 0, "movi"};
    int i;
    unsigned int offset = 4;

    printf( "avi_end: nframes = %d, fps = %d\n", nframes, fps );

    // write index

    /*write(fd, &idx, sizeof(idx));*/
    
	fwrite(&idx, sizeof(idx), 1, fp);

    for ( i = 0; i < nframes; i++ )
    {
        //write(fd, &db, 4 ); // only need the 00db
		fwrite(&db, 4, 1, fp);
        fprint_quartet( fp/*fd*/, 18 );       // ???
        fprint_quartet( fp/*fd*/, offset );
        fprint_quartet( fp/*fd*/, MAP_AVI[i]);
        offset += MAP_AVI[i] + 8; //+8 (for the additional header)
    }

    //free( sizes );

    _bzero( &ah, sizeof(ah) );
    strcpy(ah.avih, "avih");
    ah.time = 1000000 / fps;
    ah.maxbytespersec = 1000000.0*(totalsize/nframes)/ah.time;
    ah.nframes = nframes;
    ah.numstreams = 1;
    ah.flags = AVIF_HASINDEX;
    ah.width = width;
    ah.height = height;

    _bzero(&sh, sizeof(sh));
    strcpy(sh.strh, "strh");
    strcpy(sh.vids, "vids");
    strcpy(sh.codec, "MJPG");
    sh.scale = ah.time;
    sh.rate = 1000000;
    sh.length = nframes;

    _bzero(&fh, sizeof(fh));
    strcpy(fh.strf, "strf");
    fh.width = width;
    fh.height = height;
    fh.planes = 1;
    fh.bitcount = 24;
    strcpy(fh.codec,"MJPG");
    fh.unpackedsize = 3*width*height;

    rh.size = sizeof(lh1)+sizeof(ah)+sizeof(lh2)+sizeof(sh)+
        sizeof(fh)+sizeof(lh3)+sizeof(dh)+sizeof(lh4)+
        nframes*sizeof(struct db_head)+
        totalsize + sizeof(struct idx1_head)+ (16*nframes) +4; // FIXME:16 bytes per nframe // the '4' - what for???

    lh1.size = 4+sizeof(ah)+sizeof(lh2)+sizeof(sh)+sizeof(fh)+sizeof(lh3)+sizeof(dh);
    ah.size = sizeof(ah)-8;
    lh2.size = 4+sizeof(sh)+sizeof(fh)+sizeof(lh3)+sizeof(dh);     //4+sizeof(sh)+sizeof(fh);
    sh.size = sizeof(sh)-8;
    fh.size = sizeof(fh)-8;
    fh.size2 = fh.size;
    lh3.size = 4+sizeof(dh);
    lh4.size = 4+ nframes*sizeof(struct db_head)+ totalsize;

    /*lseek(fd, 0, SEEK_SET);*/
    fseek(fp,0, SEEK_SET);

  /*  write(fd, &rh, sizeof(rh));*/
    fwrite( &rh, sizeof(rh), 1, fp);
   /* write(fd, &lh1, sizeof(lh1));*/
	fwrite( &lh1, sizeof(lh1), 1, fp);
    /*write(fd, &ah, sizeof(ah));*/
    fwrite( &ah, sizeof(ah), 1, fp);
   /* write(fd, &lh2, sizeof(lh2));*/
    fwrite( &lh2, sizeof(lh2), 1, fp);
    /*write(fd, &sh, sizeof(sh));*/
    fwrite( &sh, sizeof(sh), 1, fp);
    /*write(fd, &fh, sizeof(fh));*/
    fwrite( &fh, sizeof(fh), 1, fp);
   /* write(fd, &lh3, sizeof(lh3));*/
    fwrite(&lh3, sizeof(lh3), 1, fp);
   /* write(fd, &dh, sizeof(dh));*/
    fwrite(&dh, sizeof(dh), 1, fp);
   /* write(fd, &lh4, sizeof(lh4));*/
   fwrite(&lh4, sizeof(lh4), 1, fp);

  /* if(fp) fclose(fp);*/

}

