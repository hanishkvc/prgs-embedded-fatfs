/*
 * bdk.h - generic blockdev logic
 * v04Oct2004_1826
 * C Hanish Menon <hanishkvc>, 14july2004
 * 
 */
#ifndef _BDK_H_
#define _BDK_H_

#define BDK_NUMDEVICES 16
#define BDK_SECSIZE 512
#define BDK_DEVNAMELEN 16

typedef struct blockDevKT
{
  int (*init)(struct blockDevKT*);
  int (*get_sectors)(struct blockDevKT*, long, long, char*); 
  int (*cleanup)(struct blockDevKT*);
  char name[BDK_DEVNAMELEN];
  long secSize;
  unsigned long totSecs;
  void *u1, *u2, *u3, *u4;
} bdkT;

bdkT *blockDevKs[BDK_NUMDEVICES];

#endif
