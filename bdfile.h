/*
 * bdfile.h - library for working with a linux loop based filesystem file
 * v30Sep2004_2230
 * C Hanish Menon, 2004
 * 
 */

#ifndef _BDFILE_H_
#define _BDFILE_H_

#include <bdk.h>
#define BDFILE "bdf.bd"

bdkT bdkBDFile;

int bdfile_setup();

#endif

