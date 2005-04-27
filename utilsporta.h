/*
 * utilsporta.h - portability utility functions
 * v14Oct2004-1344
 * C Hanish Menon <hanishkvc>, 28Aug2004
 */

#ifndef _UTILS_PORTA_H_
#define _UTILS_PORTA_H_

#include <paall.h>

#define char16 unsigned short int
#define uint8 unsigned char
#define int32 long int
#define uint32 unsigned long int
#define PA_NULL 0

int pa_printstr(const char *str);
int pa_printstrErr(const char *str);

int pa_printint(const int data);
int pa_printints(const int data, char *str);
int pa_printuint(const unsigned int data);
int pa_printuints(const unsigned int data, char *str);
int pa_printhex(const unsigned int data);
int pa_printhexs(const unsigned int data, char *str);

void pa_divmod(int num, int den, int *div, int *rem);
int pa_mod(int num, int den);
int pa_div(int num, int den);

void pa_swapstring(char *dest, int destLen);
void pa_memset(void *vdest, int ival, uint32 len);
void pa_memcpy(void *dest, void *src, uint32 len);
int pa_strnlen(const char *src, int maxlen);
int pa_strnlen_c16(const char16 *src, int maxlen);
int pa_strncpyEx(char *dest, const char *src, uint32 len, int *iConsumed);
int pa_strncpy(char *dest, char *src, uint32 len);
int pa_strncpyEx_c16(char16 *dest, char16 *src, uint32 len, int *iConsumed);
int pa_strncmp(char *dest, char *src, uint32 len);
int pa_strncmp_c16(char16 *dest, char16 *src, uint32 len);
void pa_toupper(char *str);

uint32 pa_strtoul(const char *nptr, char **endptr, int base);
int32 pa_strtol(const char *nptr, char **endptr, int base);
void pa_uint8TOhexstr(char *cDest, uint8 iInt);
void pa_uint32TOhexstr(char *cDest, uint32 iInt);
int pa_uint32TOstr(char *cDest, int destLen, uint32 iInt, int *iConsumed);
int pa_int32TOstr(char *cDest, int destLen, int32 iInt, int *iConsumed);
int pa_bufferTOhexstr(char *dest, int destLen, char *src, int srcLen);

int pa_strc16Tostr_nolen(char *dest, char16 *src);
int pa_strc16Tostr_len(char *dest, char16 *src, int destLen);
int pa_strTostrc16_len(char16 *dest, char *src, int destLen);

int pa_vsnprintf(char *dest, int destLen, char *format, void *args[]);
int pa_vprintfEx(char *format, void *args[], char *dest, int destLen);

#ifndef PA_PERROR
int pa_perror(char *str);
#endif

int pa_isspace(char iCur);
int pa_isdigit(char iCur);

void pa_getdatetime(int32* yearSince1900, uint8* month, uint8* day, 
       uint8* hr, uint8* min, uint8* sec);

#endif

