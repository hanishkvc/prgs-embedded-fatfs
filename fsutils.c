/*
 * fsutils.c - library for fs utility functions
 * v14Oct2004_0935
 * C Hanish Menon <hanishkvc>, 14july2004
 */
#include <partk.h>
#include <fatfs.h>

int fsutils_mount(bdkT *bd, int bdGrpId, int bdDevId, int partNo,
      struct TFat *fat, struct TFatBuffers *fatBuffers,
      int forceMbr)
{
  pikT pInfo;
  int iRet,baseSec,totSecs;
  
  if((iRet=bd->init(bd,(char*)fatBuffers->FBBuf,bdGrpId,bdDevId)) != 0)
    return iRet;
  if((iRet=partk_get(&pInfo,bd,(char*)fatBuffers->FBBuf,forceMbr)) != 0)
  {
    if(iRet != -ERROR_PARTK_NOMBR) return iRet;
    printf("INFO:mount: no MBR\n");
    baseSec = 0;
    totSecs = bd->totSecs;
  }
  else
  {
    printf("INFO:mount: yes MBR\n");
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

