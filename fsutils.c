/*
 * fsutils.c - library for fs utility functions
 * v04Oct2004_1819
 * C Hanish Menon <hanishkvc>, 14july2004
 */
#include <partk.h>
#include <fatfs.h>

int fsutils_mount(bdkT *bd, struct TFat *fat, struct TFatBuffers *fatBuffers,
      int partNo)
{
  pikT pInfo;
  int iRet,baseSec;
  
  if((iRet=bd->init(bd)) != 0)
    return iRet;
  if(partk_get(&pInfo,bd,(char*)fatBuffers->FBBuf) != 0)
  {
    printf("DEBUG:mount: no MBR\n");
    baseSec = 0;
  }
  else
  {
    printf("DEBUG:mount: yes MBR\n");
    baseSec = pInfo.fLSec[partNo];
  }
  
  if((iRet=fatfs_init(fat, fatBuffers, bd, baseSec)) != 0)
  {
    bd->cleanup(bd);
    return iRet;
  }
  return 0;
}

int fsutils_umount(bdkT *bd, struct TFat *fat)
{
  fatfs_cleanup(fat);
  bd->cleanup(bd);
  return 0;
}

