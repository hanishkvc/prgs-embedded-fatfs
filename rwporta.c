/*
 * rwporta.h - cross platform portability layer for read/write
 * v11Mar2005_1222
 * C Hanish Menon, 2003
 * 
 * v0.4 - 03Sep2003
 * - Made all buffer based read and write logic update the 
 *   buffer pointer passed to it.
 * v0.4.1 - 17Nov2003-1043
 * - Added some missing definitions in the header file
 * - Also defined file_read/write_fourcc only if not already defined
 * v14july2004
 * - added support for extracting string from buffer
 * v17Feb2005
 * - added support for writing buffer to a buffer
 * - also the earlier logic to read a string from a buffer was more
 *   a logic to read a buffer from a buffer into a string so renamed
 *   accordingly. And made the earlier buffer_read_string a alias to
 *   it which is ok for now but inefficient as it would ignore NULL
 *   termination.
 * - added buffer_write_uint8_[le|be][_noup]
 * v11Mar2005
 * - added buffer_write_uint[16|32]_le_noup
 *
 */
#include "rwporta.h"

uint32 file_read_uint32_le(FILE *fp)
{
	uint32 integer;
	uint8 temp;

	integer = 0;
	temp = fgetc(fp);
	integer |= temp;
	temp = fgetc(fp);
	integer |= temp << 8;
	temp = fgetc(fp);
	integer |= temp << 16;
	temp = fgetc(fp);
	integer |= temp << 24;

	return integer;
}

uint32 file_write_uint32_le(FILE *fp, uint32 data)
{
	uint8 temp;

	temp = data & 0xff;
	fputc(temp, fp);
	temp = (data & 0xff00) >> 8;
	fputc(temp, fp);
	temp = (data & 0xff0000) >> 16;
	fputc(temp, fp);
	temp = (data & 0xff000000) >> 24;
	fputc(temp, fp);

	return data;
}

uint32 file_read_uint32_be(FILE *fp)
{
	uint32 integer;
	uint8 temp;

	integer = 0;
	temp = fgetc(fp); /* MSB */
	integer |= temp << 24;
	temp = fgetc(fp);
	integer |= temp << 16;
	temp = fgetc(fp);
	integer |= temp << 8;
	temp = fgetc(fp);
	integer |= temp;

	return integer;
}

uint32 file_write_uint32_be(FILE *fp, uint32 data)
{
	uint8 temp;

	temp = (data & 0xff000000) >> 24;
	fputc(temp, fp);
	temp = (data & 0xff0000) >> 16;
	fputc(temp, fp);
	temp = (data & 0xff00) >> 8;
	fputc(temp, fp);
	temp = data & 0xff;
	fputc(temp, fp);

	return data;
}

uint64 file_read_uint64_be(FILE *fp)
{
	uint64 integer, temp64;
	uint8 temp;

	integer = 0;
	temp = fgetc(fp); /* MSB */
	temp64 = temp;
	integer |= temp64 << 56;
	temp = fgetc(fp);
	temp64 = temp;
	integer |= temp64 << 48;
	temp = fgetc(fp);
	temp64 = temp;
	integer |= temp64 << 40;
	temp = fgetc(fp);
	temp64 = temp;
	integer |= temp64 << 32;
	temp = fgetc(fp);
	integer |= temp << 24;
	temp = fgetc(fp);
	integer |= temp << 16;
	temp = fgetc(fp);
	integer |= temp << 8;
	temp = fgetc(fp);
	integer |= temp;

	return integer;
}

uint64 file_write_uint64_be(FILE *fp, uint64 data)
{
	exit(-1);
}

uint16 file_read_uint16_be(FILE *fp)
{
	uint16 integer;
	uint8 temp;

	integer = 0;
	temp = fgetc(fp); /* MSB */
	integer |= temp << 8;
	temp = fgetc(fp);
	integer |= temp;

	return integer;
}

uint16 file_write_uint16_be(FILE *fp, uint16 data)
{
	uint8 temp;

	temp = (data & 0xff00) >> 8;
	fputc(temp, fp);
	temp = data & 0xff;
	fputc(temp, fp);

	return data;
}

uint16 file_write_uint16_le(FILE *fp, uint16 data)
{
	uint8 temp;

	temp = data & 0xff;
	fputc(temp, fp);
	temp = (data & 0xff00) >> 8;
	fputc(temp, fp);

	return data;
}

uint16 file_read_uint16_le(FILE *fp)
{
	uint16 integer;
	uint8 temp;

	integer = 0;
	temp = fgetc(fp);
	integer |= temp;
	temp = fgetc(fp); /* MSB */
	integer |= temp << 8;

	return integer;
}

int32 file_read_buffer(FILE *fp, uint8 *buffer, uint32 size)
{
	int32 temp;

	temp = fread(buffer, 1, size, fp);
	if(temp != size)
	{
		fprintf(stderr, "Error: Couldn't read specified amount of file data\n");
		exit(-1);
	}
	return temp;
}

int32 file_write_buffer(FILE *fp, uint8 *buffer, uint32 size)
{
	int32 temp;

	temp = fwrite(buffer, 1, size, fp);
	if(temp != size)
	{
		fprintf(stderr, "Error: Couldn't write specified amount of file data\n");
		exit(-1);
	}
	return temp;
}



uint32 buffer_read_uint32_be(uint8 **curPos)
{
	uint32 integer;
	uint8 temp;

	//integer = 0;
	temp = **curPos;
	integer = temp << 24;
	(*curPos)++;
	temp = **curPos;
	integer |= temp << 16;
	(*curPos)++;
	temp = **curPos;
	integer |= temp << 8;
	(*curPos)++;
	temp = **curPos;
	integer |= temp;
	(*curPos)++;
	return integer;	
}

uint32 buffer_write_uint32_be(uint8 **curPos, uint32 data)
{
	uint8 temp;

	temp = (data & 0xff000000) >> 24;
	**curPos = temp; (*curPos)++;
	temp = (data & 0xff0000) >> 16;
	**curPos = temp; (*curPos)++;
	temp = (data & 0xff00) >> 8;
	**curPos = temp; (*curPos)++;
	temp = data & 0xff;
	**curPos = temp; (*curPos)++;

	return data;
}

uint32 buffer_read_uint32_le(uint8 **curPos)
{
	uint32 integer;
	uint8 temp;

	temp = **curPos;
	integer = temp;
	(*curPos)++;
	temp = **curPos;
	integer |= temp << 8;
	(*curPos)++;
	temp = **curPos;
	integer |= temp << 16;
	(*curPos)++;
	temp = **curPos;
	integer |= temp << 24;
	(*curPos)++;

	return integer;	
}

uint32 buffer_write_uint32_le(uint8 **curPos, uint32 data)
{
	uint8 temp;

	temp = data & 0xff;
	**curPos = temp; (*curPos)++;
	temp = (data & 0xff00) >> 8;
	**curPos = temp; (*curPos)++;
	temp = (data & 0xff0000) >> 16;
	**curPos = temp; (*curPos)++;
	temp = (data & 0xff000000) >> 24;
	**curPos = temp; (*curPos)++;

	return data;
}

uint32 buffer_write_uint32_le_noup(uint8 *curPos, uint32 data)
{
  uint8 *tCurPos = curPos;
  return buffer_write_uint32_le(&tCurPos, data);
}


uint16 buffer_write_uint16_be(uint8 **curPos, uint16 data)
{
	uint8 temp;

	temp = (data & 0xff00) >> 8;
	**curPos = temp; (*curPos)++;
	temp = data & 0xff;
	**curPos = temp; (*curPos)++;

	return data;
}

uint16 buffer_read_uint16_be(uint8 **curPos)
{
	uint16 integer;
	uint8 temp;

	temp = **curPos;
	integer = temp << 8;
	(*curPos)++;
	temp = **curPos;
	integer |= temp;
	(*curPos)++;
	return integer;	
}

uint16 buffer_write_uint16_le(uint8 **curPos, uint16 data)
{
	uint8 temp;

	temp = data & 0xff;
	**curPos = temp; (*curPos)++;
	temp = (data & 0xff00) >> 8;
	**curPos = temp; (*curPos)++;

	return data;
}

uint16 buffer_write_uint16_le_noup(uint8 *curPos, uint16 data)
{
  uint8 *tCurPos = curPos;
  return buffer_write_uint16_le(&tCurPos, data);
}

uint16 buffer_read_uint16_le(uint8 **curPos)
{
	uint16 integer;
	uint8 temp;

	temp = **curPos;
	integer = temp;
	(*curPos)++;
	temp = **curPos;
	integer |= temp << 8;
	(*curPos)++;
	return integer;	
}

uint8 buffer_read_uint8_le(uint8 **curPos) 
{
	uint8 temp;
  
	temp = **curPos;
	(*curPos)++;
	return temp;
}

uint8 buffer_write_uint8_le(uint8 **curPos, uint8 data)
{

  **curPos = data; (*curPos)++;
  return data;
}

uint8 buffer_read_uint8_le_noup(uint8 *curPos)
{
  uint8 *tCurPos = curPos;
  return buffer_read_uint8_le(&tCurPos);
}

uint8 buffer_write_uint8_le_noup(uint8 *curPos, uint8 data)
{
  uint8 *tCurPos = curPos;
  return buffer_write_uint8_le(&tCurPos, data);
}


/* 8 because curPos is a (uint8*) */
#define MINIMUMBITSREAD 8
/* This is because maxbits supported is 32 (uint32 ret value) */
#define MAXNUMBITSSUPPORTEDS_MSBSET 0x80000000
#define MAXNUMBITSSUPPORTED 32

/* In this logic Bytes LSbit contains the bitsymbols LSbit */
/* curBitPos should be <= 7 */
uint32 buffer_read_bits_lsbbased(uint8 **curPos, uint32 *curBitPos, int length)
{
  uint8 curTData;
  uint32 curData;
  int bitsFilled;
  uint32 mask;
  int32 iMask;

  curTData = **curPos;
  curTData >>= *curBitPos;
  curData = curTData;  
  bitsFilled = MINIMUMBITSREAD-*curBitPos; 

  while(bitsFilled < length)
  {
    (*curPos)++;
    curTData = **curPos;
    curData |= curTData << bitsFilled;
    bitsFilled += MINIMUMBITSREAD;
  }
  
  if(bitsFilled != length) /* Can be only bitsFilled > length */
    *curBitPos = MINIMUMBITSREAD - (bitsFilled - length);
  else /* bitsFilled == length */
  {
    *curBitPos = 0;
    (*curPos)++;
  }
  mask = MAXNUMBITSSUPPORTEDS_MSBSET;
  iMask = (int32)mask;
  iMask >>= ((MAXNUMBITSSUPPORTED-length)-1);
  mask = (uint32)iMask;
  mask = ~mask;
  curData = curData & mask;
  return curData;
}

/* In this logic Bytes MSbit contains the bitsymbols MSbit */
uint32 buffer_read_bits_msbbased(uint8 **curPos, uint32 *curBitPos, int length)
{
  uint8 curTData;
  uint32 curData;
  int bitsFilled;
  uint32 mask;
  int32 iMask;

  curTData = **curPos;
  mask = 0xFFFFFF80; iMask = mask;
  iMask >>= *curBitPos - 1; mask = iMask; mask = ~mask;
  curTData &= mask;
  curData = curTData;  
  bitsFilled = MINIMUMBITSREAD-*curBitPos; 

  while(bitsFilled < length)
  {
    (*curPos)++;
    curTData = **curPos;
    curData <<= MINIMUMBITSREAD;
    curData |= curTData;
    bitsFilled += MINIMUMBITSREAD;
  }
  
  if(bitsFilled != length) /* Can be only bitsFilled > length */
  {
    curData >>= (bitsFilled - length);
    *curBitPos = MINIMUMBITSREAD - (bitsFilled - length);
  }
  else /* bitsFilled == length */
  {
    *curBitPos = 0;
    (*curPos)++;
  }
  /* This logic is not required strictly speaking, but have left
   * it here to keep symmetry with _lsbbased version
   */
  mask = MAXNUMBITSSUPPORTEDS_MSBSET;
  iMask = (int32)mask;
  iMask >>= ((MAXNUMBITSSUPPORTED-length)-1);
  mask = (uint32)iMask;
  mask = ~mask;
  curData = curData & mask;
  return curData;
}

char *buffer_read_buffertostring(uint8 **curPos, int length, uint8 *str, int strLen)
{
  int iCur, len;

  if(strLen <= length) 
    len = strLen-1; 
  else 
    len = length;
  for(iCur=0; iCur<len; iCur++)
  {
    str[iCur] = **curPos;
    (*curPos)++;
  }
  str[len]=(uint8)NULL;
  return str;
}

char *buffer_read_buffertostring_noup(uint8 *curPos, int length, uint8 *str, int strLen)
{
  uint8 *tCurPos = curPos;
  return buffer_read_buffertostring(&tCurPos, length, str, strLen);
}

char *buffer_write_buffer(uint8 **curPos, uint8 *buf, int bufLen)
{
  int iCur;

  for(iCur=0; iCur<bufLen; iCur++)
  {
    **curPos = buf[iCur];
    (*curPos)++;
  }
  return buf;
}

char *buffer_write_buffer_noup(uint8 *curPos, uint8 *buf, int bufLen)
{
  uint8 *tCurPos = curPos;
  return buffer_write_buffer(&tCurPos, buf, bufLen);
}

