/*
 * bdk.h - generic blockdev logic
 * v12Oct2004_1237
 * C Hanish Menon <hanishkvc>, 14july2004
 * 
 */
#ifndef _BDK_H_
#define _BDK_H_

#define BDK_NUMDEVICES 16
#define BDK_MAXSECSIZE 4096
#define BDK_SECSIZE_512 512
#define BDK_DEVNAMELEN 16

typedef struct blockDevKT
{
  int grpId, devId;
  int (*init)(struct blockDevKT*, char*, int, int);
  int (*get_sectors)(struct blockDevKT*, long, long, char*); 
  int (*cleanup)(struct blockDevKT*);
  int (*reset)(struct blockDevKT*);
  char name[BDK_DEVNAMELEN];
  long secSize;
  unsigned long totSecs;
  /* for bdhdd */
  int CMDBR, CNTBR;
  /* general */
  void *u1, *u2, *u3, *u4;
} bdkT;

bdkT *blockDevKs[BDK_NUMDEVICES];

#endif

