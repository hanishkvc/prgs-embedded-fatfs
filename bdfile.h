/*
 * bdfile.h - library for working with a linux loop based filesystem file
 * v14july2004
 * C Hanish Menon, 2004
 * 
 */

#ifndef _BDFILE_H_
#define _BDFILE_H_

#define BDFILE "bd.bd"
#define BDSECSIZE 512

int bd_init();
int bd_get_sectors(long sec, long count, char*buf);
int bd_cleanup();

#endif

