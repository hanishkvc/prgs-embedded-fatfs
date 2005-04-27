/*
 * testfat.c - a test program for fat filesystem library
 * v27Apr2005_2030
 * C Hanish Menon <hanishkvc>, 14july2004
 * 
 */

#define TESTFAT_PRGVER "v27Apr2005_2030"
#undef NO_REALTIME_SCHED  

#include <sched.h>

#include <inall.h>
#include <errs.h>
#include <bdk.h>
#include <fatfs.h>
#include <partk.h>
#include <linuxutils.h>
#include <rand.h>

#define TESTFAT_BDBM_SECS 80000
#define TESTFAT_BDBMR_SECS 2048
#define TESTFAT_BDBMR_TIMES 20
#define TESTFAT_BDBMW_SECS 2048
#define TESTFAT_BDBMW_TIMES 20

bdkT bdkBDFile;
bdkT bdkHdd;
bdkT bdkH8b16;

struct TFat fat1;
struct TFatBuffers fat1Buffers;
struct TFatFsUserContext gUC;
struct TFileInfo fInfo;
#define DATABUF_MAXSIZE (FATFSCLUS_MAXSIZE*64)
uint32 dataBuf[DATABUF_MAXSIZE/4];
uint8 sBuf1[8*1024], sBuf2[8*1024], cCur, *gDataBuf;
uint8 t8LFN[256]; uint16 t16LFN[256];
struct timeval gTFtv1, gTFtv2;
int32 timeInUSECS;
void *pArg[8];
char pBuf[1024];
uint32 gDataBufSize;
int gFatUCFuncs=0;
char sCur[16];

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

#if 0
/* 27Apr2005: Better and simpler logic except for DEBUG_LEVEL issue */
/* Works if fatfs library is compiled with DEBUG_PRINT_FATFS > 15 */
int testfat_fsfreelist(struct TFatFsUserContext *uc)
{
  return fatfs_update_freeclusters(uc->fat);
}

#else

#define CLUSLIST_SIZE 8

int testfat_fsfreelist(struct TFatFsUserContext *uc)
{
  int clSize; 
  int iCur, res;
  uint32 fromClus, totalClus;
  struct TClusList cl[CLUSLIST_SIZE];

  printf("************* Filesystem Free list logic ******************\n");
  fromClus = 0;
  totalClus = 0;
  do
  {
    clSize = CLUSLIST_SIZE; 
    res = fatfs__getoptifreecluslist(uc->fat, cl, &clSize, 0, &fromClus);
    for(iCur=0; iCur < clSize; iCur++)
    {
      totalClus += cl[iCur].adjClusCnt+1;
      printf("Optimised FreeClusList baseClus[%ld] adjClusCnt[%ld] next fromClus[%ld]\n",
        cl[iCur].baseClus, cl[iCur].adjClusCnt, fromClus);
    }
    if((res!=0) && ((res!=-ERROR_TRYAGAIN) || (res!=-ERROR_NOMORE)))
    {
      fprintf(stderr,"ERR:testfat:fsfreelist:FATchain may not be valid\n");
      return -1;
    }
  }while(res != 0);
  printf("TotalFREEspace [%ld]\n",totalClus*uc->fat->clusSize);
  return 0;
}

#endif

int testfat_checkfile(struct TFatFsUserContext *uc, char *cFile)
{
  int clSize, iCur, res;
  uint32 fromClus, prevPos;
  struct TClusList cl[CLUSLIST_SIZE];

  printf("************* Check file logic ******************\n");
  prevPos = 0;
  res = fatuc_getfileinfo(uc, cFile, &fInfo, &prevPos);
  if(res == 0)
  {
    printf("testfat: File[%s][%s] attr[0x%x] firstClus[%ld] fileSize[%ld]\n",
      fInfo.name,(char*)fInfo.lfn, fInfo.attr, fInfo.firstClus, fInfo.fileSize);
    fromClus = 0;
    do
    {
      clSize = CLUSLIST_SIZE; 
      res = fatfs_getopticluslist_usefileinfo(uc->fat, &fInfo, cl, &clSize, 
        &fromClus);
      if((res!=0) && (res!=-ERROR_TRYAGAIN))
      {
        fprintf(stderr,"ERR:testfat:checkfile:FATchain not valid for file\n");
	return -1;
      }
      for(iCur=0; iCur < clSize; iCur++)
      {
        printf("[%s] Optimised ClusList baseClus[%ld] adjClusCnt[%ld] next fromClus[%ld]\n",
          fInfo.name, cl[iCur].baseClus, cl[iCur].adjClusCnt, fromClus);
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
  uint32 prevPos, fromClus, dataClusRead;
  int res, ret, resFW, bytesRead;

  if(strncmp(dFile,"STDOUT",6) != 0)
  {
    fDest = fopen(dFile, "w");
    if(fDest == NULL)
    {
      perror("testfat:ERROR: opening dest file");
      return -1;
    }
  }
  else
    fDest = stdout;
  prevPos = 0;
  if(fatuc_getfileinfo(uc, sFile, &fInfo, &prevPos) != 0)
  {
    printf("testfat:ERROR: opening src [%s] file\n", sFile);
    return -1;
  }
  fromClus = 0;
  ret = -1;
  fatfs_checkbuf_forloadfileclus(uc->fat, gDataBufSize);
  while(1)
  {
    res=fatfs_loadfileclus_usefileinfo(uc->fat, &fInfo, gDataBuf, gDataBufSize, 
      &dataClusRead, &fromClus);
    if((res != 0)&&(res != -ERROR_TRYAGAIN))
      break;
#if DEBUG_TESTFAT > 15    
    else
      printf("testfat:INFO: loadfileclus clusRead[%ld] next fromClus[%ld]\n", 
        dataClusRead, fromClus);
#endif    
    bytesRead = dataClusRead*uc->fat->bs.secPerClus*uc->fat->bs.bytPerSec;
    resFW=fwrite(gDataBuf,1, bytesRead, fDest);
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
  uint32 prevPos, fromClus, dataClusRead;
  int res, ret, bytesRead;

  prevPos = 0;
  if(fatuc_getfileinfo(uc, sFile, &fInfo, &prevPos) != 0)
  {
    printf("testfat:ERROR: opening src [%s] file\n", sFile);
    return -1;
  }
  *fileSize = fInfo.fileSize;
  fromClus = 0;
  ret = -1;
  fatfs_checkbuf_forloadfileclus(uc->fat, gDataBufSize);
  while(1)
  {
    res=fatfs_loadfileclus_usefileinfo(uc->fat, &fInfo, gDataBuf, gDataBufSize, 
      &dataClusRead, &fromClus);
    if((res != 0)&&(res != -ERROR_TRYAGAIN))
      break;
#if DEBUG_TESTFAT > 15    
    else
      printf("testfat:INFO: loadfileclus clusRead[%ld] next fromClus[%ld]\n", 
        dataClusRead, fromClus);
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

void testfat_normalfsfile_checksum(char *sFile)
{
  FILE *fSrc;
  int cSum = 0, cCur;

  fSrc = fopen(sFile,"rb");
  if(fSrc == NULL)
  {
    fprintf(stderr,"ERR:testfat:normalfsfile_checksum: opening[%s]\n", sFile);
    return;
  }
  while(!feof(fSrc))
  {
    cCur=fgetc(fSrc);
    if(cCur == EOF)
      break;
    cSum += cCur;
  }
  printf("testfat:normalfsfile_checksum: [%s] => [%d]\n",sFile,cSum);
}

int testfat_fatfsfile_checksum(struct TFatFsUserContext *uc, char *sFile)
{
  uint32 prevPos, fromClus, dataClusRead;
  int res, ret, bytesRead,iCur,cSum,fileSize;

  prevPos = 0;
  if(fatuc_getfileinfo(uc, sFile, &fInfo, &prevPos) != 0)
  {
    printf("ERR:testfat:fatfsfile_checksum: opening file[%s]\n", sFile);
    return -1;
  }
  fileSize = fInfo.fileSize;
  fromClus = 0;
  cSum = 0;
  ret = -1;
  fatfs_checkbuf_forloadfileclus(uc->fat, gDataBufSize);
  while(1)
  {
    res=fatfs_loadfileclus_usefileinfo(uc->fat, &fInfo, gDataBuf, gDataBufSize, 
      &dataClusRead, &fromClus);
    if((res != 0)&&(res != -ERROR_TRYAGAIN))
      break;
#if DEBUG_TESTFAT > 15    
    else
      printf("testfat:INFO: loadfileclus clusRead[%ld] next fromClus[%ld]\n", 
        dataClusRead, fromClus);
#endif    
    bytesRead = dataClusRead*uc->fat->bs.secPerClus*uc->fat->bs.bytPerSec;
    for(iCur=0;iCur<bytesRead;iCur++)
    {
      if(fileSize <= 0)
        break;
      cSum += gDataBuf[iCur];
      fileSize--;
    }
    if((res == 0) && (fileSize <= 0))
    {
      ret = 0;
      break;
    }
  }
  if(ret == 0)
    printf("testfat:fatfsfile_checksum: [%s] => [%d]\n", sFile, cSum);
  else
    fprintf(stderr,"ERR:testfat:fatfsfile_checksum: [%s] failed\n", sFile);
  return ret;
}

#include "testfatuc.c"

int main(int argc, char **argv)
{
  int iCur, bExit, grpId, devId, partNo, forceMbr, bdkFlags=0, iNumSecsUsed;
  bdkT *bdk;
  char *pChar;
  uint32 fileSize;
  struct sched_param schedP;
  int32 savedDataBufSize;
  
  printf("[%s]Usage: %s <hd|file|h8b16> <bdGrp,bdDev,partNo> <resetBD|noResetBD> <forceMBR|noForceMBR> <DataBufSize> <y|n interactive> <ni-file>\n", 
    TESTFAT_PRGVER, argv[0]);
  gDataBufSize = DATABUF_MAXSIZE;
  gDataBuf = (uint8*)dataBuf;
  printf("INFO:testfat:MaxDatabuf size is [%ld]\n",gDataBufSize);

  /*** initialization ***/
  lu_starttime(&gTFtv1);
  bdfile_setup(&bdkBDFile);
  bdhdd_setup(&bdkHdd);
  bdh8b16_setup(&bdkH8b16);
  if(argc < 7)
  {
    printf("Not enough args, quiting\n");
    exit(10);
  }
#ifndef NO_REALTIME_SCHED  
  schedP.sched_priority = 50;
  if(sched_setscheduler(0,SCHED_RR,&schedP) != 0)
  {
    fprintf(stderr,"ERR:testfat: Unable to use realtime scheduling\n");	  
    exit(20);
  }
  else
    fprintf(stderr,"INFO:testfat: REALTIME Scheduling enabled\n");
#endif
  if(pa_strncmp(argv[1],"hd",2) == 0)
    bdk = &bdkHdd;
  else if(pa_strncmp(argv[1],"h8b16",5) == 0)
    bdk = &bdkH8b16;
  else
    bdk = &bdkBDFile;
  if(bdk != &bdkBDFile)
  {
    pa_printstr("QUERY:testfat: Force BUSSLOW [y/n]:");
    scanf("%s",sCur); fgetc(stdin);
    if(sCur[0] == 'y')
    {
      bdkFlags |= BDK_FLAG_BUSSLOW;
    }
    else
    {
      bdkFlags &= ~BDK_FLAG_BUSSLOW;
    }
    pa_printstr("QUERY:testfat: Force SECSINGLE [y/n]:");
    scanf("%s",sCur); fgetc(stdin);
    if(sCur[0] == 'y')
    {
      bdkFlags |= BDK_FLAG_SECSINGLE;
    }
    else
    {
      bdkFlags &= ~BDK_FLAG_SECSINGLE;
    }
  }
  grpId = strtoul(argv[2],&pChar,0);
  devId = strtoul(&pChar[1],&pChar,0);
  partNo = strtoul(&pChar[1],NULL,0);
  
  if(argv[3][0] == 'r')
    bdkFlags |= BDK_FLAG_INITRESET;
  else
    bdkFlags &= ~BDK_FLAG_INITRESET;
  if(argv[4][0] == 'f')
    forceMbr = 1;
  else
    forceMbr = 0;
  gDataBufSize = strtoul(argv[5],NULL,0);
  if((gDataBufSize%FATFSCLUS_MAXSIZE) != 0)
  {
    fprintf(stderr,"INFO:testfat: DataBufSize [%ld] not multiple of [%d]\n",
      gDataBufSize, FATFSCLUS_MAXSIZE);
    gDataBufSize = DATABUF_MAXSIZE;
  }
  else
    fprintf(stderr,"INFO:testfat: DataBufSize set to [%ld]\n", gDataBufSize);

  if(fsutils_mount(bdk,grpId,devId,partNo,&fat1,&fat1Buffers,
      forceMbr,bdkFlags) != 0)
  {
    fprintf(stderr,"ERR:testfat: mount failed\n");
    exit(20);
  }
  fatuc_init(&gUC, &fat1);
  lu_stoptimedisp(&gTFtv1,&gTFtv2,&timeInUSECS,"Init");

  if((argc>7) && (argv[6][0] == 'n'))
  {
    lu_starttime(&gTFtv1);
    testfat_checkfile(&gUC, argv[7]);
    lu_stoptimedisp(&gTFtv1,&gTFtv2,&timeInUSECS,"checkFile");
    goto cleanup;
  }
  /*** interactive commands ***/
  do{
    bExit = 0;
    printf("[%s]========curDir [%s]========\n", TESTFAT_PRGVER,gUC.sCurDir);
    printf("(ls)dirListing (cpfp)fileExtract2Posix (bsr|w)BlockDev speed\n");
    printf("(cd)chDir (cf)checkFile (reset)Reset (fs)readspeed (fsa)readspeedALL\n");
    printf("(nfc)normalfsfile checksum (ffc)fatfsfile checksum\n");
    printf("(FUC)FatUserContext Funcs (seek)seektest (fsfreelist)FilesysFreelist\n");
    printf("(cpff)fileExtract2fatfs (del) deletefile (mkdir) mkdir (move) move\n");
    printf("(afree)approxFreeSpace (free)FreeSpace (exit) Exit\n");
    scanf("%s",sCur); fgetc(stdin);
    if(strcmp("ls",sCur) == 0)
    {
      printf("enter directory to list:");
      scanf("%s",sBuf1);
      fgetc(stdin);
      lu_starttime(&gTFtv1);
      testfat_dirlisting(&gUC, sBuf1);
      lu_stoptimedisp(&gTFtv1,&gTFtv2,&timeInUSECS,"dirListing");
    }
    if(strcmp("cpfp",sCur) == 0)
    {
      printf("enter fatfs_src and posix_dest files:");
      scanf("%s %s", sBuf1, sBuf2);
      fgetc(stdin);
      lu_starttime(&gTFtv1);
      if(gFatUCFuncs)
        testfatuc_fileextract2posix(&gUC, sBuf1, sBuf2);
      else
        testfat_fileextract(&gUC, sBuf1, sBuf2);
      lu_stoptimedisp(&gTFtv1,&gTFtv2,&timeInUSECS,"fileExtract");
    }
    if(strcmp("cpff",sCur) == 0)
    {
      printf("enter fatfs_src and fatfs_dest files:");
      scanf("%s %s", sBuf1, sBuf2);
      fgetc(stdin);
      lu_starttime(&gTFtv1);
      //if(gFatUCFuncs)
        testfatuc_fileextract2fatfs(&gUC, sBuf1, sBuf2);
      //else
      //  testfat_fileextract(&gUC, sBuf1, sBuf2);
      lu_stoptimedisp(&gTFtv1,&gTFtv2,&timeInUSECS,"fileExtract");
    }
    if(strcmp("cd",sCur) == 0)
    {
      printf("enter dir to chdir to:");
      scanf("%s", sBuf1);
      fgetc(stdin);
      lu_starttime(&gTFtv1);
      fatuc_chdir(&gUC, sBuf1);
      lu_stoptimedisp(&gTFtv1,&gTFtv2,&timeInUSECS,"chDir");
    }
    if(strcmp("cf",sCur) == 0)
    {
      printf("enter file to check:");
      scanf("%s",sBuf1);
      fgetc(stdin);
      lu_starttime(&gTFtv1);
      testfat_checkfile(&gUC, sBuf1);
      lu_stoptimedisp(&gTFtv1,&gTFtv2,&timeInUSECS,"checkFile");
    }
    if(strcmp("fsfreelist",sCur) == 0)
    {
      lu_starttime(&gTFtv1);
      testfat_fsfreelist(&gUC);
      lu_stoptimedisp(&gTFtv1,&gTFtv2,&timeInUSECS,"fsfreelist");
    }
    if(strcmp("fs",sCur) == 0)
    {
      printf("enter src file for read speed test:");
      scanf("%s",sBuf1);
      fgetc(stdin);
      lu_starttime(&gTFtv1);
      if(gFatUCFuncs)
        testfatuc_checkreadspeed(&gUC, sBuf1, &fileSize);
      else
        testfat_checkreadspeed(&gUC, sBuf1, &fileSize);
      lu_stoptimedisp(&gTFtv1,&gTFtv2,&timeInUSECS,"readspeed");
      fprintf(stderr,"fileSize[%ld] time[%ld]usecs readSpeed/msec[%ld]\n",
        fileSize,timeInUSECS,(fileSize/(timeInUSECS/1000)));
    }
    if(strcmp("fsa",sCur) == 0)
    {
      printf("enter src file for read speed test:");
      scanf("%s",sBuf1);
      fgetc(stdin);
      savedDataBufSize = gDataBufSize;
      gDataBufSize = DATABUF_MAXSIZE;
      while(gDataBufSize >= FATFSCLUS_MAXSIZE)
      {
        fprintf(stderr,"testfat:readspeedALL: BufSize [%ld]\n",gDataBufSize);
        lu_starttime(&gTFtv1);
        if(gFatUCFuncs)
          testfatuc_checkreadspeed(&gUC, sBuf1, &fileSize);
        else
          testfat_checkreadspeed(&gUC, sBuf1, &fileSize);
        lu_stoptimedisp(&gTFtv1,&gTFtv2,&timeInUSECS,"readspeedALL");
        fprintf(stderr,"fileSize[%ld] time[%ld]usecs readSpeed/msec[%ld]\n",
          fileSize,timeInUSECS,(fileSize/(timeInUSECS/1000)));
        gDataBufSize = gDataBufSize/2;
      }
      gDataBufSize = savedDataBufSize;
    }
    if(strcmp("bsr",sCur) == 0)
    {
      printf("BlockDev raw speed test for [%d] sectors\n",TESTFAT_BDBM_SECS);
      lu_starttime(&gTFtv1);
      if(bdk->get_sectors_benchmark == NULL)
      {
        fprintf(stderr,"INFO:testfat: NO BlockDev benchmark function Using normal get_sectors\n");
        for(iCur=0;iCur<TESTFAT_BDBMR_TIMES;iCur++)
        {
          if(bdk->get_sectors(bdk,0,TESTFAT_BDBMR_SECS,gDataBuf)!=0)
            fprintf(stderr,"ERR:testfat: get_sectors_benchmark failed\n");
	}
	iNumSecsUsed = TESTFAT_BDBMR_TIMES*TESTFAT_BDBMR_SECS;
      }
      else
      {
        if(bdk->get_sectors_benchmark(bdk,0,TESTFAT_BDBM_SECS,gDataBuf)!=0)
          fprintf(stderr,"ERR:testfat: get_sectors_benchmark failed\n");
	iNumSecsUsed = TESTFAT_BDBM_SECS;
      }
      lu_stoptimedisp(&gTFtv1,&gTFtv2,&timeInUSECS,"BlockDev raw speed");
      fprintf(stderr,"NumSecs[%d] time[%ld]usecs readSpeed/msec[%ld]\n",
        iNumSecsUsed,timeInUSECS,
        ((iNumSecsUsed*bdk->secSize)/(timeInUSECS/1000)));
    }
    if(strcmp("bsw",sCur) == 0)
    {
      printf("BlockDev raw write speed test for [%d] sectors\n",TESTFAT_BDBMW_SECS*TESTFAT_BDBMW_TIMES);
      if(bdk->get_sectors(bdk,0,TESTFAT_BDBMW_SECS,gDataBuf)!=0)
        fprintf(stderr,"ERR:testfat: get_sectors failed\n");
      lu_starttime(&gTFtv1);
      for(iCur=0;iCur<TESTFAT_BDBMW_TIMES;iCur++)
      {
        if(bdk->put_sectors(bdk,0,TESTFAT_BDBMW_SECS,gDataBuf)!=0)
          fprintf(stderr,"ERR:testfat: put_sectors failed\n");
      }
      lu_stoptimedisp(&gTFtv1,&gTFtv2,&timeInUSECS,"BlockDev raw write speed");
      fprintf(stderr,"NumSecs[%d] time[%ld]usecs writeSpeed/msec[%ld]\n",
        TESTFAT_BDBMW_SECS*TESTFAT_BDBMW_TIMES,timeInUSECS,
        ((TESTFAT_BDBMW_SECS*TESTFAT_BDBMW_TIMES*bdk->secSize)/(timeInUSECS/1000)));
    }
    if(strcmp("reset",sCur) == 0)
    {
      printf("Reseting blockdev [%s]\n",bdk->name);
      bdk->reset(bdk);
    }
    if(strcmp("nfc",sCur) == 0)
    {
      printf("enter normalfsfile to checksum:");
      scanf("%s", sBuf1);
      fgetc(stdin);
      lu_starttime(&gTFtv1);
      testfat_normalfsfile_checksum(sBuf1);
      lu_stoptimedisp(&gTFtv1,&gTFtv2,&timeInUSECS,"normalfsfile_checksum");
    }
    if(strcmp("ffc",sCur) == 0)
    {
      printf("enter fatfsfile to checksum:");
      scanf("%s", sBuf1);
      fgetc(stdin);
      lu_starttime(&gTFtv1);
      if(gFatUCFuncs)
        testfatuc_fatfsfile_checksum(&gUC, sBuf1);
      else
        testfat_fatfsfile_checksum(&gUC, sBuf1);
      lu_stoptimedisp(&gTFtv1,&gTFtv2,&timeInUSECS,"fatfsfile_checksum");
    }
    if(strcmp("FUC",sCur) == 0)
    {
      if(gFatUCFuncs == 0)
      {
        gFatUCFuncs = 1;
        printf("testfat:INFO: FatUserContext functions Selected\n");
      }
      else
      {
        gFatUCFuncs = 0;
        printf("testfat:INFO: FatUserContext functions DESelected\n");
      }
    }
    if(strcmp("seek",sCur) == 0)
    {
      printf("enter file for fseek test:");
      scanf("%s",sBuf1);
      fgetc(stdin);
      lu_starttime(&gTFtv1);
      if(testfatuc_fileseektest(&gUC, sBuf1) != 0)
        pa_printstrErr("testfat:ERROR: fileseektest\n");
      lu_stoptimedisp(&gTFtv1,&gTFtv2,&timeInUSECS,"seekTest");
    }
    if(strcmp("del",sCur) == 0)
    {
      printf("enter file for delete:");
      scanf("%s",sBuf1); fgetc(stdin);
      lu_starttime(&gTFtv1);
      if(testfatuc_deletefile(&gUC, sBuf1) != 0)
        pa_printstrErr("testfat:ERROR: deletefile\n");
      lu_stoptimedisp(&gTFtv1,&gTFtv2,&timeInUSECS,"deletefile");
    }
    if(strcmp("mkdir",sCur) == 0)
    {
      printf("enter dir to create:");
      scanf("%s",sBuf1); fgetc(stdin);
      lu_starttime(&gTFtv1);
      if(testfatuc_mkdir(&gUC, sBuf1) != 0)
        pa_printstrErr("testfat:ERROR: mkdir\n");
      lu_stoptimedisp(&gTFtv1,&gTFtv2,&timeInUSECS,"mkdir");
    }
    if(strcmp("move",sCur) == 0)
    {
      printf("enter src file/dir and destPath and name:");
      scanf("%s %s %s", sBuf1, sBuf2, t8LFN);
      pa_strTostrc16_len(t16LFN,t8LFN,256);
      fgetc(stdin);
      lu_starttime(&gTFtv1);
      if(pa_strncmp(t8LFN,"NULL",4)==0)
        testfatuc_move(&gUC, sBuf1, sBuf2, NULL);
      else
        testfatuc_move(&gUC, sBuf1, sBuf2, t16LFN);
      lu_stoptimedisp(&gTFtv1,&gTFtv2,&timeInUSECS,"move");
    }
    {
    uint32 FreeClusters, Flag;
    int ClusSize;
    if(strcmp("afree",sCur) == 0)
    {
      lu_starttime(&gTFtv1);
      fatuc_approx_getfreeclusters(&gUC,&FreeClusters,&Flag,&ClusSize);
      lu_stoptimedisp(&gTFtv1,&gTFtv2,&timeInUSECS,"approxFreeSpace");
      printf("INFO:testfat:aFree: Clusters[%ld] x ClusSize[%d], Flag[%lx]\n",
        FreeClusters,ClusSize,Flag);
    }
    if(strcmp("free",sCur) == 0)
    {
      lu_starttime(&gTFtv1);
      fatuc_getfreeclusters(&gUC,&FreeClusters,&Flag,&ClusSize);
      lu_stoptimedisp(&gTFtv1,&gTFtv2,&timeInUSECS,"freeSpace");
      printf("INFO:testfat:Free: Clusters[%ld] x ClusSize[%d], Flag[%lx]\n",
        FreeClusters,ClusSize,Flag);
    }
    }
    if(strcmp("exit",sCur) == 0)
      bExit = 1;
  }while(!bExit);

cleanup:  /*** cleanup ***/
  fsutils_umount(bdk,&fat1);
  return 0; 
}

