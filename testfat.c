/*
 * testfat.c - a test program for fat filesystem library
 * v10Oct2004_0019
 * C Hanish Menon <hanishkvc>, 14july2004
 * 
 */

#define TESTFAT_PRGVER "v10Oct2004_0025"

#include <sys/time.h>

#include <inall.h>
#include <errs.h>
#include <bdfile.h>
#include <bdhdd.h>
#include <fatfs.h>
#include <partk.h>

struct TFat fat1;
struct TFatBuffers fat1Buffers;
struct TFatFsUserContext gUC;
struct TFileInfo fInfo;
#define DATABUF_SIZE (FATFSCLUS_MAXSIZE*10)
uint8 dataBuf[DATABUF_SIZE], sBuf1[8*1024], sBuf2[8*1024], cCur;
struct timeval tv1, tv2;
int32 tempT;

void testfat_starttime()
{
  gettimeofday(&tv1, NULL);
}

void testfat_stoptimedisp(char *sPrompt)
{
  gettimeofday(&tv2, NULL);
  fprintf(stderr,"*** Time spent [%s] ***\n", sPrompt);
  fprintf(stderr,"tv1 [%ld sec: %ld usec] tv2 [%ld sec: %ld usec]\n", 
    tv1.tv_sec, tv1.tv_usec, tv2.tv_sec, tv2.tv_usec);
  if((tv2.tv_sec-tv1.tv_sec) == 0)
    fprintf(stderr,"i.e USECS [%ld]\n", tv2.tv_usec - tv1.tv_usec);
  else
  {
    tempT = 1000000-tv1.tv_usec;
    tempT += tv2.tv_usec;
    tempT += (tv2.tv_sec-tv1.tv_sec-1)*1000000;
    fprintf(stderr,"i.e USECS [%ld]\n", tempT);
  }
}

int testfat_rootdirlisting(struct TFatFsUserContext *uc)
{
  uint32 prevPos;
  printf("************* rootdir listing ******************\n");
  prevPos = 0;
  while(fatfs_getfileinfo_fromdir("", uc->fat->RDBuf, uc->fat->rdSize, 
    &fInfo, &prevPos) == 0)
  {
    printf("testfat: File[%s] attr[0x%x] firstClus:Size[%ld:%ld]\n",
      fInfo.lfn, fInfo.attr, fInfo.firstClus, fInfo.fileSize);
  }
  return 0;
}

int testfat_dirlisting(struct TFatFsUserContext *uc, char *dir)
{
  uint32 prevPos;
  printf("************* dir[%s][%s] listing ******************\n", 
    uc->sCurDir,dir);
#if 0
  if(fatuc_chdir(&uc, dir) != 0)
    return -1;
  prevPos = 0;
  while(fatuc_getfileinfo(&uc, "", &fInfo, &prevPos) == 0)
  {
    printf("testfat: File[%s:%s] attr[0x%x] firstClus:Size[%ld:%ld]\n",
      fInfo.name, fInfo.lfn, fInfo.attr, fInfo.firstClus, fInfo.fileSize);
  }
#else
  prevPos = 0;
  if(strcmp(dir,".") == 0)
    dir[0] = 0;
  while(fatuc_getfileinfo(uc, dir, &fInfo, &prevPos) == 0)
  {
    printf("testfat: File[%s:%s] attr[0x%x] firstClus:Size[%ld:%ld]\n",
      fInfo.name, fInfo.lfn, fInfo.attr, fInfo.firstClus, fInfo.fileSize);
  }
#endif
  return 0;
}

#define CLUSLIST_SIZE 1

int testfat_checkfile(struct TFatFsUserContext *uc, char *cFile)
{
  int clSize, iCur, res;
  uint32 prevClus, prevPos;
  struct TClusList cl[CLUSLIST_SIZE];

  printf("************* Check file logic ******************\n");
  prevPos = 0;
  res = fatuc_getfileinfo(uc, cFile, &fInfo, &prevPos);
  if(res == 0)
  {
    printf("testfat: File[%s][%s] attr[0x%x] firstClus[%ld] fileSize[%ld]\n",
      fInfo.name, fInfo.lfn, fInfo.attr, fInfo.firstClus, fInfo.fileSize);
    prevClus = 0;
    do
    {
      clSize = CLUSLIST_SIZE; 
      res = fatfs_getopticluslist_usefileinfo(uc->fat, &fInfo, cl, &clSize, 
        &prevClus);
      if((res!=0) && (res!=-ERROR_TRYAGAIN))
      {
        fprintf(stderr,"ERR:testfat:checkfile:FATchain not valid for file\n");
	return -1;
      }
      for(iCur=0; iCur < clSize; iCur++)
      {
        printf("[%s] Optimised ClusList baseClus[%ld] adjClusCnt[%ld] prevClus[%ld]\n",
          fInfo.name, cl[iCur].baseClus, cl[iCur].adjClusCnt, prevClus);
      }
    }while(res != 0);
  }
  else
  {
    printf("testfat: file [%s] not found \n", cFile);
    return -1;
  }
  return 0;
}

int testfat_fileextract(struct TFatFsUserContext *uc, char *sFile, char *dFile)
{
  FILE *fDest;
  uint32 prevPos, prevClus, dataClusRead;
  int res, ret, resFW, bytesRead;

  fDest = fopen(dFile, "w");
  if(fDest == NULL)
  {
    perror("testfat:ERROR: opening dest file");
    return -1;
  }
  prevPos = 0;
  if(fatuc_getfileinfo(uc, sFile, &fInfo, &prevPos) != 0)
  {
    printf("testfat:ERROR: opening src [%s] file\n", sFile);
    return -1;
  }
  prevClus = 0;
  ret = -1;
  fatfs_checkbuf_forloadfileclus(uc->fat, DATABUF_SIZE);
  while(1)
  {
    res=fatfs_loadfileclus_usefileinfo(uc->fat, &fInfo, dataBuf, DATABUF_SIZE, 
      &dataClusRead, &prevClus);
    if((res != 0)&&(res != -ERROR_TRYAGAIN))
      break;
#if DEBUG_TESTFAT > 15    
    else
      printf("testfat:INFO: loadfileclus clusRead[%ld] prevClus[%ld]\n", 
        dataClusRead, prevClus);
#endif    
    bytesRead = dataClusRead*uc->fat->bs.secPerClus*uc->fat->bs.bytPerSec;
    resFW=fwrite(dataBuf,1, bytesRead, fDest);
    if(resFW != bytesRead)
    {
      perror("testfat:ERROR: less bytes written\n");
      break;
    }
    if(res == 0)
    {
      ret = 0;
      break;
    }
  }
  fclose(fDest);
  return ret;
}

int main(int argc, char **argv)
{
  int bExit;
  bdkT *bdk;
  
  printf("[%s]Usage: %s <h_d|f_ile> <y|n = interactive mode>\n", 
    TESTFAT_PRGVER, argv[0]);

  /*** initialization ***/
  testfat_starttime();
  bdfile_setup();
  bdhdd_setup();
  if(argc > 1)
  {
    if(argv[1][0] == 'h')
      bdk = &bdkHdd;
    else
      bdk = &bdkBDFile;
    fsutils_mount(bdk, &fat1, &fat1Buffers, 0);
  }
  else
  {
    printf("Not enough args, quiting\n");
    exit(10);
  }
  fatuc_init(&gUC, &fat1);
  testfat_stoptimedisp("Init");

  if((argc > 2) && (argv[2][0] == 'n'))
  {
    testfat_starttime();
    testfat_checkfile(&gUC, argv[2]);
    testfat_stoptimedisp("checkFile");
    goto cleanup;
  }
  /*** interactive commands ***/
  do{
    bExit = 0;
    printf("[%s]========curDir [%s]========\n", TESTFAT_PRGVER,gUC.sCurDir);
    printf("(l) dirListing (e) fileExtract (E) Exit\n");
    printf("(c) chDir (f) checkFile\n");
    cCur = fgetc(stdin); fgetc(stdin);
    switch(cCur)
    {
    case 'l':
      printf("enter directory to list:");
      scanf("%s",sBuf1);
      fgetc(stdin);
      testfat_starttime();
      testfat_dirlisting(&gUC, sBuf1);
      testfat_stoptimedisp("dirListing");
      break;
    case 'e':
      printf("enter src and dest files:");
      scanf("%s %s", sBuf1, sBuf2);
      fgetc(stdin);
      testfat_starttime();
      testfat_fileextract(&gUC, sBuf1, sBuf2);
      testfat_stoptimedisp("fileExtract");
      break;
    case 'c':
      printf("enter dir to chdir to:");
      scanf("%s", sBuf1);
      fgetc(stdin);
      testfat_starttime();
      fatuc_chdir(&gUC, sBuf1);
      testfat_stoptimedisp("chDir");
      break;
    case 'f':
      printf("enter file to check:");
      scanf("%s",sBuf1);
      fgetc(stdin);
      testfat_starttime();
      testfat_checkfile(&gUC, sBuf1);
      testfat_stoptimedisp("checkFile");
      break;
    case 'E':
      bExit = 1;
      break;
    }
  }while(!bExit);

cleanup:  /*** cleanup ***/
  fsutils_umount(&bdkBDFile,&fat1);
  return 0; 
}

