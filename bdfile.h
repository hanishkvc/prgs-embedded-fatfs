/*
 * bdfile.h - library for working with a linux loop based filesystem file
 * v12Oct2004_1319
 * C Hanish Menon, 2004
 * 
 */

#ifndef _BDFILE_H_
#define _BDFILE_H_

#include <bdk.h>
#define BDFILE "bdf.bd"

#define BDFILE_FILE_LSEEK64 1

bdkT bdkBDFile;

int bdfile_setup();

#endif

