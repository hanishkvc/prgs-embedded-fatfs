/*
 * rwporta.h - cross platform portability layer for read/write
 * v04Oct2004_1522
 * C Hanish Menon, 2003
 *
 */
#ifndef _RWPORTA_H_
#define _RWPORTA_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Defines for portability */
#define uint64 unsigned long long int
#define uint32 unsigned long int
#define uint16 unsigned short int
#define uint8  unsigned char 
#define int64  long long int
#define int32  long int
#define int16  short int
#define int8   char

#ifndef file_read_fourcc
#define file_read_fourcc file_read_uint32_le
#endif
#ifndef file_write_fourcc
#define file_write_fourcc file_write_buffer
#endif

uint32 file_read_uint32_le(FILE *fp);
uint32 file_write_uint32_le(FILE *fp, uint32 data);
uint32 file_read_uint32_be(FILE *fp);
uint32 file_write_uint32_be(FILE *fp, uint32 data);

uint64 file_read_uint64_be(FILE *fp);
uint64 file_write_uint64_be(FILE *fp, uint64 data);

uint16 file_read_uint16_be(FILE *fp);
uint16 file_write_uint16_be(FILE *fp, uint16 data);
uint16 file_read_uint16_le(FILE *fp);
uint16 file_write_uint16_le(FILE *fp, uint16 data);

#define file_read_uint8_le file_read_uint8
#define file_read_uint8(fp) fgetc(fp)

int32 file_read_buffer(FILE *fp, uint8 *buffer, uint32 size);
int32 file_write_buffer(FILE *fp, uint8 *buffer, uint32 size);

/* NOTE: the curPos passed gets updated automatically */

uint32 buffer_read_uint32_be(uint8 **curPos);
uint32 buffer_write_uint32_be(uint8 **curPos, uint32 data);

uint16 buffer_read_uint16_be(uint8 **curPos);
uint16 buffer_write_uint16_be(uint8 **curPos, uint16 data);

uint32 buffer_read_uint32_le(uint8 **curPos);
uint32 buffer_write_uint32_le(uint8 **curPos, uint32 data);

uint16 buffer_read_uint16_le(uint8 **curPos);
uint16 buffer_write_uint16_le(uint8 **curPos, uint16 data);

#define buffer_read_uint8_be buffer_read_uint8_le
#define buffer_write_uint8_be buffer_write_uint8_le
uint8 buffer_read_uint8_le(uint8 **curPos); 
uint8 buffer_write_uint8_le(uint8 **curPos, uint8 data);
uint8 buffer_read_uint8_le_noup(uint8 *curPos); 
uint8 buffer_write_uint8_le_noup(uint8 *curPos, uint8 data); 

uint32 buffer_read_bits_lsbbased(uint8 **curPos, uint32 *curBitPos, int length);
uint32 buffer_read_bits_msbbased(uint8 **curPos, uint32 *curBitPos, int length);

char *buffer_read_buffertostring(uint8 **curPos, int length, uint8 *buf, int bufLen);
#define buffer_read_string buffer_read_buffertostring
char *buffer_read_buffertostring_noup(uint8 *curPos, int length, uint8 *buf, int bufLen);
#define buffer_read_string_noup buffer_read_buffertostring_noup
char *buffer_write_buffer(uint8 **curPos, uint8 *buf, int bufLen);
char *buffer_write_buffer_noup(uint8 *curPos, uint8 *buf, int bufLen);

#endif

