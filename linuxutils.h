/*
 * linuxutils.h - library for linux related support routines
 * v07Nov2004_1101
 * C Hanish Menon <hanishkvc>, 14july2004
 */
#ifndef _LINUXUTILS_H_
#define _LINUXUTILS_H_

#include <sys/time.h>

int lu_nanosleeppaka(int secs, int nanosecs);
void lu_starttime(struct timeval *tv1);
void lu_stoptimedisp(struct timeval *tv1, struct timeval *tv2, long int *timeInUSECS, char *sPrompt);

#endif

