/*
 * bdfile.c - library for working with a linux loop based filesystem file
 * v14july2004
 * C Hanish Menon, 2004
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
  int res;

  res = lseek(gFID, sec*BDSECSIZE, SEEK_SET);
  if(res == -1)
  {
    perror("BDFILE: lseeking file");
    exit(10);
  }
  res = read(gFID, buf, count*BDSECSIZE);
  if(res != count*BDSECSIZE)
  {
    perror("BDFILE: reading file");
    exit(10);
  }
  return 0;
}


int bd_cleanup()
{
  close(gFID);
  return 0;
}

