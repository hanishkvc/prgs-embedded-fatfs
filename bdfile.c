/*
 * bdfile.c - library for working with a linux loop based filesystem file
 * v06Nov2004_1707
 * C Hanish Menon <hanishkvc>, 14july2004
 * 
 */

#include <bdfile.h>
#ifdef BDFILE_FILE_LSEEK64
#define _GNU_SOURCE
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <errs.h>
#include <utilsporta.h>

int bdf_init(bdkT *bd, char *secBuf, int grpId, int devId, int reset)
{
#ifdef BDFILE_FILE_LSEEK64
  fprintf(stderr,"INFO:BDFILE: using LSEEK64\n");
#else
  fprintf(stderr,"INFO:BDFILE: using LSEEK\n");
#endif
  bd->u1 = (void*)open(BDFILE, O_RDWR);
  if((int)bd->u1 == -1)
  {
    perror("BDFILE: Opening file");
    exit(10);
  }
  bd->secSize = BDK_SECSIZE_512;
  bd->totSecs = 0xFFFFFFFF;
  return 0;
}

int bdf_get_sectors(bdkT *bd, long sec, long count, char*buf)
{
  int res, toRead;

#if (DEBUG_PRINT_BDFILE > 15)
  printf("INFO:bdfile:get sec[%ld] count[%ld]\n", sec, count);
#endif

#ifdef BDFILE_FILE_LSEEK64
  {
  long long fOffset;
  fOffset = (long long)sec*bd->secSize;
  res = lseek64((int)bd->u1, fOffset, SEEK_SET);
  }
#else
  res = lseek((int)bd->u1, sec*bd->secSize, SEEK_SET);
#endif
  if(res == -1)
  {
    perror("BDFILE: lseeking file");
    return -ERROR_UNKNOWN;
  }
  toRead = count*bd->secSize;

  while(1)
  {
    res = read((int)bd->u1, buf, toRead);
    if(res == -1)
    {
      perror("BDFILE: reading file");
      return -ERROR_UNKNOWN;
    }
    else if(res != toRead)
    {
      printf("??INFO??:bdfile: read of [%d] got [%d]\n", toRead, res);
      buf += res;
      toRead -= res;
      sleep(1);
      printf("INFO:bdfile: remaining to read [%d]\n", toRead);
    }
    else
      break;
  }
  return 0;
}

int bdf_put_sectors(bdkT *bd, long sec, long count, char*buf)
{
  int res, toWrite;

#if (DEBUG_PRINT_BDFILE > 15)
  printf("INFO:bdfile:put sec[%ld] count[%ld]\n", sec, count);
#endif

#ifdef BDFILE_FILE_LSEEK64
  {
  long long fOffset;
  fOffset = (long long)sec*bd->secSize;
  res = lseek64((int)bd->u1, fOffset, SEEK_SET);
  }
#else
  res = lseek((int)bd->u1, sec*bd->secSize, SEEK_SET);
#endif
  if(res == -1)
  {
    perror("BDFILE: lseeking file");
    return -ERROR_UNKNOWN;
  }
  toWrite = count*bd->secSize;

  while(1)
  {
    res = write((int)bd->u1, buf, toWrite);
    if(res == -1)
    {
      perror("BDFILE: writing file");
      return -ERROR_UNKNOWN;
    }
    else if(res != toWrite)
    {
      printf("??INFO??:bdfile: write of [%d] got [%d]\n", toWrite, res);
      buf += res;
      toWrite -= res;
      sleep(1);
      printf("INFO:bdfile: remaining to write [%d]\n", toWrite);
    }
    else
      break;
  }
  return 0;
}


int bdf_cleanup(bdkT *bd)
{
  close((int)bd->u1);
  return 0;
}

int bdf_reset(bdkT *bd)
{
  fprintf(stderr,"INFO:bdfile: reset called\n");
  return 0;
}

int bdfile_setup(bdkT *bdk)
{
  bdk->init = bdf_init;
  bdk->cleanup = bdf_cleanup;
  bdk->reset = bdf_reset;
  bdk->get_sectors = bdf_get_sectors;
  bdk->put_sectors = bdf_put_sectors;
  bdk->get_sectors_benchmark = NULL;
  pa_strncpy(bdk->name,"bdfile",BDK_DEVNAMELEN);
  return 0;
}

