/*
 * testfat.c - a test program for fat filesystem library
 * v15july2004-2200
 * C Hanish Menon, 14july2004
 * 
 */

#include <bdfile.h>
#include <fatfs.h>

struct TFileInfo fInfo;
struct TClusList cl[2];

int main(int argc, char **argv)
{
  int clSize, iCur, res;
  uint32 prevClus;

  bd_init();
  if(fatfs_init(0)!= 0)
  {
    bd_cleanup();
    return -1;
  }
  fatfs_fileinfo_indir("", gFat.RDBuf, gFat.rdSize, &fInfo);
  if(argc > 1)
  {
    printf("**********************************\n");
    fatfs_fileinfo_indir(argv[1], gFat.RDBuf, gFat.rdSize, &fInfo);
    prevClus = 0;
    do
    {
      if(prevClus != 0)
        printf("info:testfat: file as more clusters\n");
      clSize = 2; 
      res = fatfs16_getfile_opticlusterlist(fInfo, cl, &clSize, &prevClus);
      for(iCur=0; iCur < clSize; iCur++)
      {
        printf("[%s] Optimised ClusList baseClus[%ld] adjClusCnt[%ld] prevClus[%ld]\n",
          fInfo.name, cl[iCur].baseClus, cl[iCur].adjClusCnt, prevClus);
      }
    }while(res != 0);
  }

  fatfs_cleanup();
  bd_cleanup();
  return 0; 
}

