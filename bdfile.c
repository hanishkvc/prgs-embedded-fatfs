/*
 * bdfile.c - library for working with a linux loop based filesystem file
 * v30Sep2004_2230
 * C Hanish Menon <hanishkvc>, 14july2004
 * 
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <inall.h>
#include <bdfile.h>
#include <errs.h>
#include <utilsporta.h>

int bdf_init(bdkT *bd)
{
  bd->u1 = (void*)open(BDFILE, O_RDONLY);
  if((int)bd->u1 == -1)
  {
    perror("BDFILE: Opening file");
    exit(10);
  }
  return 0;
}

int bdf_get_sectors(bdkT *bd, long sec, long count, char*buf)
{
  int res, toRead;

#if (DEBUG_PRINT_BDFILE > 10)
  printf("INFO:bdfile: sec[%ld] count[%ld]\n", sec, count);
#endif

  res = lseek((int)bd->u1, sec*bd->secSize, SEEK_SET);
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

int bdfile_setup()
{
  bdkBDFile.init = bdf_init;
  bdkBDFile.cleanup = bdf_cleanup;
  bdkBDFile.get_sectors = bdf_get_sectors;
  bdkBDFile.secSize = BDK_SECSIZE;
  pa_strncpy(bdkBDFile.name,"bdfile",BDK_DEVNAMELEN);
  return 0;
}

