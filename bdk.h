/*
 * bdk.h - generic blockdev logic
 * v17Mar2005_1307
 * C Hanish Menon <hanishkvc>, 14july2004
 * 
 */
#ifndef _BDK_H_
#define _BDK_H_

#define BDK_NUMDEVICES 16
#define BDK_MAXSECSIZE 4096
#define BDK_SECSIZE_512 512
#define BDK_DEVNAMELEN 16

#define BDK_FLAG_INITRESET 0x1
#define BDK_FLAG_BUSSLOW   0x2
#define BDK_FLAG_SECSINGLE 0x4

typedef struct blockDevKT
{
  int grpId, devId;
  int (*init)(struct blockDevKT*, char* secBuf, 
               int grpId, int devId, int bdkFlags);
  int (*get_sectors)(struct blockDevKT*, long sec, long count, char* buf); 
  int (*put_sectors)(struct blockDevKT*, long sec, long count, char* buf); 
  int (*cleanup)(struct blockDevKT*);
  int (*reset)(struct blockDevKT*);
  char name[BDK_DEVNAMELEN];
  long secSize, multiCnt;
  unsigned long totSecs;
  /* for bdhdd/bdh8b16 */
  int CMDBR, CNTBR;
  /* general */
  void *u1, *u2, *u3, *u4;
  /* benchmark */
  int (*get_sectors_benchmark)(struct blockDevKT*, 
                                long sec, long count, char* buf); 
} bdkT;

bdkT *blockDevKs[BDK_NUMDEVICES];

int bdfile_setup(bdkT *bdk);
int bdh8b16_setup(bdkT *bdk);
int bdhdd_setup(bdkT *bdk);

#endif

