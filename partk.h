/*
 * part.h - library for working with partition table
 * v16july2004-1000
 * C Hanish Menon <hanishkvc>, 16july2004
 * 
 */

#ifndef _PART_H_
#define _PART_H_

#include <rwporta.h>

#ifndef DEBUG_PRINT_PART
#define DEBUG_PRINT_PART 11
#endif


#define PART1_OFFSET 0x1be
#define PART2_OFFSET 0x1ce
#define PART3_OFFSET 0x1de
#define PART4_OFFSET 0x1ee
#define PARTEXECMARK_OFFSET 0x1fe

/* In partition table Cyl and Sector are embedded in a single 16bit number
   like this => (MSB-LSB) 8bitCylLo,2bitCylHi,6bitSec */

struct TPartInfo
{
  uint8 active[4];
  uint8 sHead[4];
  uint16 sCyl[4]; 
  uint8 sSec[4];
  uint8 type[4];
  uint8 eHead[4];
  uint16 eCyl[4];
  uint8 eSec[4];
  uint32 fLSec[4]; /* logical linear sector numbering */
  uint32 nLSec[4];
};

int part_get(struct TPartInfo *pInfo);

#endif

