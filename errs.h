/*
 * errs.h - header providing common error defines
 * v07Oct2004_1958
 * C Hanish Menon <hanishkvc>, 14july2004
 * 
 */

#ifndef _ERRS_H_
#define _ERRS_H_

#define ERROR_NOTSUPPORTED 100
#define ERROR_INVALID 101
#define ERROR_INSUFFICIENTRESOURCE 102
#define ERROR_TRYAGAIN 103
#define ERROR_FAILED 104

#define ERROR_NOMORE 201
#define ERROR_NOTFOUND 202

#define ERROR_UNKNOWN 999

#define ERROR_FATFS_NOTCLEANSHUT 1001
#define ERROR_FATFS_HARDERR 1002

#define ERROR_FATFS_EOF 0xFFFFFFF8
#define ERROR_FATFS_BADCLUSTER 0xFFFFFFF7
#define ERROR_FATFS_FREECLUSTER 0xFFFFFFF0

#define ERROR_MAXUINT32 0xFFFFFFFF

#endif

