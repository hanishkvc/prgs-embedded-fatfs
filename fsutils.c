/*
 * fsutils.c - library for fs utility functions
 * v09Oct2004_2307
 * C Hanish Menon <hanishkvc>, 14july2004
 */
#include <partk.h>
#include <fatfs.h>

int fsutils_mount(bdkT *bd, struct TFat *fat, struct TFatBuffers *fatBuffers,
      int partNo)
{
  pikT pInfo;
  int iRet,baseSec,totSecs;
  
  if((iRet=bd->init(bd,(char*)fatBuffers->FBBuf)) != 0)
    return iRet;
  if(partk_get(&pInfo,bd,(char*)fatBuffers->FBBuf) != 0)
  {
    printf("DEBUG:mount: no MBR\n");
    baseSec = 0;
    totSecs = bd->totSecs;
  }
  else
  {
    printf("DEBUG:mount: yes MBR\n");
    baseSec = pInfo.fLSec[partNo];
    totSecs = pInfo.nLSec[partNo];
  }
  
  if((iRet=fatfs_init(fat, fatBuffers, bd, baseSec, totSecs)) != 0)
  {
    bd->cleanup(bd);
    return iRet;
  }
  return 0;
}

int fsutils_umount(bdkT *bd, struct TFat *fat)
{
  int iRet;
  
  if((iRet=fatfs_cleanup(fat)) != 0) return iRet;
  if((iRet=bd->cleanup(bd)) != 0) return iRet;
  return 0;
}

