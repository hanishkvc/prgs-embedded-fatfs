/*
 * partk.h - library for working with partition table
 * v30Sep2004-2145
 * C Hanish Menon <hanishkvc>, 16july2004
 * 
 */

#ifndef _PARTK_H_
#define _PARTK_H_

#include <rwporta.h>
#include <bdk.h>

#ifndef DEBUG_PRINT_PARTK
#define DEBUG_PRINT_PARTK 11
#endif

#define PARTK_NUMPARTS 4
#define PARTK1_OFFSET 0x1be
#define PARTK2_OFFSET 0x1ce
#define PARTK3_OFFSET 0x1de
#define PARTK4_OFFSET 0x1ee
#define PARTKEXECMARK_OFFSET 0x1fe

/* In partition table Cyl and Sector are embedded in a single 16bit number
   like this => (MSB-LSB) 8bitCylLo,2bitCylHi,6bitSec */

typedef struct partInfoKT
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
} pikT;

int partk_get(pikT *pi, bdkT *bd);

#endif

