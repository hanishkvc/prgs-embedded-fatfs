/*
 * utilsporta.h - portability utility functions
 * v14Oct2004-1344
 * C Hanish Menon <hanishkvc>, 28Aug2004
 */

#ifndef _UTILS_PORTA_H_
#define _UTILS_PORTA_H_

#define char16 unsigned short int
#define uint8 unsigned char
#define int32 long int
#define uint32 unsigned long int


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
void pa_memset(void *vdest, int ival, unsigned long int len);
void pa_memcpy(void *dest, void *src, unsigned long int len);
int pa_strnlen(const char *src, int maxlen);
int pa_strncpyEx(char *dest, char *src, unsigned long int len, int *iConsumed);
int pa_strncpy(char *dest, char *src, unsigned long int len);
int pa_strncmp(char *dest, char *src, unsigned long int len);
int pa_strncmp_c16(char16 *dest, char16 *src, uint32 len);

unsigned long int pa_strtoul(const char *nptr, char **endptr, int base);
long int pa_strtol(const char *nptr, char **endptr, int base);
void pa_uint8TOhexstr(char *cDest, unsigned char iInt);
void pa_uint32TOhexstr(char *cDest, unsigned long int iInt);
int pa_uint32TOstr(char *cDest, int destLen, unsigned long int iInt, int *iConsumed);
int pa_int32TOstr(char *cDest, int destLen, long int iInt, int *iConsumed);
int pa_bufferTOhexstr(char *dest, int destLen, char *src, int srcLen);

int pa_vsnprintf(char *dest, int destLen, char *format, void *args[]);
int pa_vprintfEx(char *format, void *args[], char *dest, int destLen);

int pa_perror(char *str);

int pa_isspace(char iCur);
int pa_isdigit(char iCur);

#endif

