/*
 * bdfile.h - library for working with a linux loop based filesystem file
 * v02Nov2004_0007
 * C Hanish Menon, 2004
 * 
 */

#ifndef _BDFILE_H_
#define _BDFILE_H_

#include <inall.h>
#include <bdk.h>
#define BDFILE "bdf.bd"

#ifdef PRG_MODE_DM270
#undef BDFILE_FILE_LSEEK64
#else
#define BDFILE_FILE_LSEEK64 1
#endif

int bdfile_setup(bdkT *bdk);

#endif

