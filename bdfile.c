/*
 * bdfile.c - library for working with a linux loop based filesystem file
 * v12Oct2004_1519
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

#include <inall.h>
#include <errs.h>
#include <utilsporta.h>

int bdf_init(bdkT *bd, char *secBuf, int grpId, int devId)
{
#ifdef BDFILE_FILE_LSEEK64
  fprintf(stderr,"INFO:BDFILE: using LSEEK64\n");
#else
  fprintf(stderr,"INFO:BDFILE: using LSEEK\n");
#endif
  bd->u1 = (void*)open(BDFILE, O_RDONLY);
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
  printf("INFO:bdfile: sec[%ld] count[%ld]\n", sec, count);
#endif

/*
#ifdef BDFILE_FILE__LLSEEK
  {
  long long fOffset;
  unsigned long fOffsetH, fOffsetL;
  fOffset = (long long)sec*bd->secSize;
  fOffsetH = (fOffset >> 32) & 0xFFFFFFFF;
  fOffsetL = fOffset & 0xFFFFFFFF;
  res = _llseek((int)bd->u1,fOffsetH,fOffsetL,&fOffset,SEEK_SET);
  }
#endif
*/
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

int bdfile_setup()
{
  bdkBDFile.init = bdf_init;
  bdkBDFile.cleanup = bdf_cleanup;
  bdkBDFile.reset = bdf_reset;
  bdkBDFile.get_sectors = bdf_get_sectors;
  bdkBDFile.get_sectors_benchmark = NULL;
  pa_strncpy(bdkBDFile.name,"bdfile",BDK_DEVNAMELEN);
  return 0;
}

