/*
 * partk.h - library for working with partition table
 * v14Oct2004_0935
 * C Hanish Menon <hanishkvc>, 16july2004
 * 
 */

#ifndef _PARTK_H_
#define _PARTK_H_

#include <inall.h>
#include <rwporta.h>
#include <errs.h>
#include <bdk.h>

#ifndef DEBUG_PRINT_PARTK
#define DEBUG_PRINT_PARTK 11
#endif

#undef PARTK_VERIFY_MBRSTARTBYTE  

#define PARTK_NUMPARTS 4
#define PARTK1_OFFSET 0x1be
#define PARTK2_OFFSET 0x1ce
#define PARTK3_OFFSET 0x1de
#define PARTK4_OFFSET 0x1ee
#define PARTKEXECMARK_OFFSET 0x1fe

#define PARTK_MBR_STARTBYTE_T0 0xfc
#define PARTK_BS_STARTBYTE_T0 0xeb
#define PARTK_BS_STARTBYTE_T1 0xe9

/* In partition table Cyl and Sector are embedded in a single 16bit number
   like this => (MSB-LSB) 8bitCylLo,2bitCylHi,6bitSec */

typedef struct partInfoKT
{
  uint8 active[PARTK_NUMPARTS];
  uint8 sHead[PARTK_NUMPARTS];
  uint16 sCyl[PARTK_NUMPARTS]; 
  uint8 sSec[PARTK_NUMPARTS];
  uint8 type[PARTK_NUMPARTS];
  uint8 eHead[PARTK_NUMPARTS];
  uint16 eCyl[PARTK_NUMPARTS];
  uint8 eSec[PARTK_NUMPARTS];
  uint32 fLSec[PARTK_NUMPARTS]; /* logical linear sector numbering */
  uint32 nLSec[PARTK_NUMPARTS];
} pikT;

int partk_get(pikT *pi, bdkT *bd, char *pBuf, int forceMbr);

#endif

