/*
 * testfat.c - a test program for fat filesystem library
 * v27Oct2004_0011
 * C Hanish Menon <hanishkvc>, 14july2004
 * 
 */

#define TESTFAT_PRGVER "v03Nov2004_1630"

#include <sched.h>
#include <sys/time.h>

#include <inall.h>
#include <errs.h>
#include <bdfile.h>
#include <bdhdd.h>
#include <fatfs.h>
#include <partk.h>

#define TESTFAT_BDBM_SECS 80000

bdkT bdkBDFile;
bdkT bdkHdd;

struct TFat fat1;
struct TFatBuffers fat1Buffers;
struct TFatFsUserContext gUC;
struct TFileInfo fInfo;
#define DATABUF_SIZE (FATFSCLUS_MAXSIZE*32)
uint8 dataBuf[DATABUF_SIZE], sBuf1[8*1024], sBuf2[8*1024], cCur;
struct timeval tv1, tv2;
int32 swTimeInUSECS;
void *pArg[8];
char pBuf[1024];

void testfat_starttime()
{
  gettimeofday(&tv1, NULL);
}

void testfat_stoptimedisp(char *sPrompt)
{
  gettimeofday(&tv2, NULL);
  fprintf(stderr,"*** Time spent [%s] ***\n", sPrompt);
#ifdef PRG_MODE_DM270
  fprintf(stderr,"DM270:03Nov2004-NNOOOOTTTTTTEEEEEE-CPU Time fast by ~2.5\n");
#endif
  fprintf(stderr,"tv1 [%ld sec: %ld usec] tv2 [%ld sec: %ld usec]\n", 
    tv1.tv_sec, tv1.tv_usec, tv2.tv_sec, tv2.tv_usec);
  if((tv2.tv_sec-tv1.tv_sec) == 0)
  {
    swTimeInUSECS =  tv2.tv_usec - tv1.tv_usec;
  }
  else
  {
    swTimeInUSECS = 1000000-tv1.tv_usec;
    swTimeInUSECS += tv2.tv_usec;
    swTimeInUSECS += (tv2.tv_sec-tv1.tv_sec-1)*1000000;
  }
  fprintf(stderr,"i.e USECS [%ld]\n", swTimeInUSECS);
}

int testfat_rootdirlisting(struct TFatFsUserContext *uc)
{
  uint32 prevPos;
  printf("************* rootdir listing ******************\n");
  prevPos = 0;
  while(fatfs_getfileinfo_fromdir("", uc->fat->RDBuf, uc->fat->rdSize, 
    &fInfo, &prevPos, 0) == 0)
  {
    printf("testfat: File[%s] attr[0x%x] firstClus:Size[%ld:%ld]\n",
      fInfo.name, fInfo.attr, fInfo.firstClus, fInfo.fileSize);
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
#if 1	  
    pArg[0]=fInfo.name; pArg[1]=fInfo.lfn;pArg[2]=(void*)(int)fInfo.attr;
    pArg[3]=(void*)fInfo.firstClus;pArg[4]=(void*)fInfo.fileSize;pArg[5]=NULL;
    pa_vprintfEx("testfat:File[%s:%hs] attr[0x%hhx] firstClus:Size[%ld:%ld]\n",
      pArg,pBuf,1024);
#else    
    printf("testfat: File[%s:%s] attr[0x%x] firstClus:Size[%ld:%ld]\n",
      fInfo.name, fInfo.attr, fInfo.firstClus, fInfo.fileSize);
#endif    
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
      fInfo.name,(char*)fInfo.lfn, fInfo.attr, fInfo.firstClus, fInfo.fileSize);
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

int testfat_checkreadspeed(struct TFatFsUserContext *uc, char *sFile, uint32 *fileSize)
{
  uint32 prevPos, prevClus, dataClusRead;
  int res, ret, bytesRead;

  prevPos = 0;
  if(fatuc_getfileinfo(uc, sFile, &fInfo, &prevPos) != 0)
  {
    printf("testfat:ERROR: opening src [%s] file\n", sFile);
    return -1;
  }
  *fileSize = fInfo.fileSize;
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
    if(res == 0)
    {
      ret = 0;
      break;
    }
  }
  return ret;
}

int main(int argc, char **argv)
{
  int bExit, grpId, devId, partNo, forceMbr;
  bdkT *bdk;
  char *pChar;
  uint32 fileSize;
  struct sched_param schedP;
  
  printf("[%s]Usage: %s <hd|file> <bdGrp,bdDev,partNo> <resetBD|noResetBD> <forceMBR|noForceMBR> <y|n interactive> <ni-file>\n", 
    TESTFAT_PRGVER, argv[0]);
  printf("INFO:testfat:Databuf size is [%d]\n",DATABUF_SIZE);

  /*** initialization ***/
  testfat_starttime();
  bdfile_setup(&bdkBDFile);
  bdhdd_setup(&bdkHdd);
  if(argc < 6)
  {
    printf("Not enough args, quiting\n");
    exit(10);
  }
  schedP.sched_priority = 50;
  if(sched_setscheduler(0,SCHED_RR,&schedP) != 0)
  {
    fprintf(stderr,"ERR:testfat: Unable to use realtime scheduling\n");	  
    exit(20);
  }
  else
    fprintf(stderr,"INFO:testfat: REALTIME Scheduling enabled\n");
  if(argv[1][0] == 'h')
    bdk = &bdkHdd;
  else
    bdk = &bdkBDFile;
  grpId = strtoul(argv[2],&pChar,0);
  devId = strtoul(&pChar[1],&pChar,0);
  partNo = strtoul(&pChar[1],NULL,0);
  
  if(argv[3][0] == 'r')
    bdk->reset(bdk);
  if(argv[4][0] == 'f')
    forceMbr = 1;
  else
    forceMbr = 0;

  if(fsutils_mount(bdk,grpId,devId,partNo,&fat1,&fat1Buffers,forceMbr) != 0)
  {
    fprintf(stderr,"ERR: mount failed\n");
    exit(20);
  }
  fatuc_init(&gUC, &fat1);
  testfat_stoptimedisp("Init");

  if((argc>6) && (argv[5][0] == 'n'))
  {
    testfat_starttime();
    testfat_checkfile(&gUC, argv[6]);
    testfat_stoptimedisp("checkFile");
    goto cleanup;
  }
  /*** interactive commands ***/
  do{
    bExit = 0;
    printf("[%s]========curDir [%s]========\n", TESTFAT_PRGVER,gUC.sCurDir);
    printf("(l) dirListing (e) fileExtract (E) Exit (b) BlockDev speed\n");
    printf("(c) chDir (f) checkFile (R) Reset (s) readspeed\n");
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
    case 's':
      printf("enter src file for read speed test:");
      scanf("%s",sBuf1);
      fgetc(stdin);
      testfat_starttime();
      testfat_checkreadspeed(&gUC, sBuf1, &fileSize);
      testfat_stoptimedisp("readspeed");
      fprintf(stderr,"fileSize[%ld] time[%ld]usecs readSpeed/msec[%ld]\n",
        fileSize,swTimeInUSECS,(fileSize/(swTimeInUSECS/1000)));
      break;
    case 'b':
      printf("BlockDev raw speed test for [%d] sectors\n",TESTFAT_BDBM_SECS);
      testfat_starttime();
      if(bdk->get_sectors_benchmark == NULL)
      {
        fprintf(stderr,"ERR:testfat: NO BlockDev benchmark function\n");
        testfat_stoptimedisp("FAILED BlockDev raw");
	break;
      }
      if(bdk->get_sectors_benchmark(bdk,0,TESTFAT_BDBM_SECS,dataBuf)!=0)
        fprintf(stderr,"ERR:testfat: get_sectors_benchmark failed\n");
      testfat_stoptimedisp("BlockDev raw speed");
      break;
    case 'R':
      printf("Reseting blockdev [%s]\n",bdk->name);
      bdk->reset(bdk);
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

