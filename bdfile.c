/*
 * bdfile.c - library for working with a linux loop based filesystem file
 * v16july2004
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

static int gFID;

int bd_init()
{
  gFID = open(BDFILE, O_RDONLY);
  if(gFID == -1)
  {
    perror("BDFILE: Opening file");
    exit(10);
  }
  return 0;
}

int bd_get_sectors(long sec, long count, char*buf)
{
  int res, toRead;

#if (DEBUG_PRINT_BDFILE > 10)
  printf("INFO:bdfile: sec[%ld] count[%ld]\n", sec, count);
#endif

  res = lseek(gFID, sec*BDSECSIZE, SEEK_SET);
  if(res == -1)
  {
    perror("BDFILE: lseeking file");
    return -ERROR_UNKNOWN;
  }
  toRead = count*BDSECSIZE;

  while(1)
  {
    res = read(gFID, buf, toRead);
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

int bd_cleanup()
{
  close(gFID);
  return 0;
}

