/*
 * testfat.c - a test program for fat filesystem library
 * v15july2004-2200
 * C Hanish Menon, 14july2004
 * 
 */

#include <sys/time.h>

#include <inall.h>
#include <bdfile.h>
#include <fatfs.h>
#include <part.h>

struct TFileInfo fInfo;
struct TClusList cl[2];
struct TPartInfo pInfo;

int main(int argc, char **argv)
{
  int clSize, iCur, res, baseSec;
  uint32 prevClus, prevPos, tempT;
  struct timeval tv1, tv2;

  gettimeofday(&tv1, NULL);
  printf("Usage: %s <y|n = partition> <filetocheck> <y|n = rootdirlist>\n", 
    argv[0]);

  /*** initialization ***/
  bd_init();
  if((argc > 1) && (argv[1][0] == 'y'))
  {
    printf("************* partition loading ******************\n");
    if(part_get(&pInfo) != 0)
      exit(1);
    baseSec = pInfo.fLSec[0];
  }
  else
    baseSec = 0;
  
  if(fatfs_init(baseSec)!= 0)
  {
    bd_cleanup();
    return -1;
  }

  /*** rootdir listing ***/
  if((argc > 3) && (argv[3][0] == 'y'))
  {
    printf("************* rootdir listing ******************\n");
    prevPos = 0;
    while(fatfs_fileinfo_indir("", gFat.RDBuf, gFat.rdSize, 
      &fInfo, &prevPos) == 0)
    {
      printf("testfat: File[%s][%s] attr[0x%x] firstClus[%ld] fileSize[%ld]\n",
        fInfo.name, fInfo.lfn, fInfo.attr, fInfo.firstClus, fInfo.fileSize);
    }
  }

  /*** check file logic ***/
  if(argc > 2)
  {
    printf("************* Check file logic ******************\n");
    prevPos = 0;
    res = fatfs_fileinfo_indir(argv[2], gFat.RDBuf, gFat.rdSize, &fInfo, &prevPos);
    if(res == 0)
    {
      prevClus = 0;
      do
      {
        if(prevClus != 0)
          printf("info:testfat: file as more clusters\n");
        clSize = 1; 
        res = fatfs16_getfile_opticlusterlist(fInfo, cl, &clSize, &prevClus);
        for(iCur=0; iCur < clSize; iCur++)
        {
          printf("[%s] Optimised ClusList baseClus[%ld] adjClusCnt[%ld] prevClus[%ld]\n",
            fInfo.name, cl[iCur].baseClus, cl[iCur].adjClusCnt, prevClus);
        }
      }while(res != 0);
    }
    else
      printf("testfat: file [%s] not found \n", argv[2]);
  }

  fatfs_cleanup();
  bd_cleanup();
  gettimeofday(&tv2, NULL);
  fprintf(stderr,"tv1 [%ld sec: %ld usec] tv2 [%ld sec: %ld usec]\n", 
    tv1.tv_sec, tv1.tv_usec, tv2.tv_sec, tv2.tv_usec);
  if((tv2.tv_sec-tv1.tv_sec) == 0)
    fprintf(stderr,"Timespent is %ld usecs\n", tv2.tv_usec - tv1.tv_usec);
  else
  {
    tempT = 1000000-tv1.tv_usec;
    tempT += tv2.tv_usec;
    tempT += (tv2.tv_sec-tv1.tv_sec-1)*1000000;
    fprintf(stderr,"Timespent is %ld usecs\n", tempT);
  }
  return 0; 
}

