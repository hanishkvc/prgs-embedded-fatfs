/*
 * testfat.c - a test program for fat filesystem library
 * v07Nov2004_1330
 * C Hanish Menon <hanishkvc>, 14july2004
 * 
 */

#define TESTFAT_PRGVER "v19Nov2004_2234"

#include <sched.h>

#include <inall.h>
#include <errs.h>
#include <bdfile.h>
#include <bdhdd.h>
#include <bdh8b16.h>
#include <fatfs.h>
#include <partk.h>
#include <linuxutils.h>

#define TESTFAT_BDBM_SECS 80000

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
struct timeval gTFtv1, gTFtv2;
int32 timeInUSECS;
void *pArg[8];
char pBuf[1024];
uint32 gDataBufSize;

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
  prevClus = 0;
  ret = -1;
  fatfs_checkbuf_forloadfileclus(uc->fat, gDataBufSize);
  while(1)
  {
    res=fatfs_loadfileclus_usefileinfo(uc->fat, &fInfo, gDataBuf, gDataBufSize, 
      &dataClusRead, &prevClus);
    if((res != 0)&&(res != -ERROR_TRYAGAIN))
      break;
#if DEBUG_TESTFAT > 15    
    else
      printf("testfat:INFO: loadfileclus clusRead[%ld] prevClus[%ld]\n", 
        dataClusRead, prevClus);
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
  fatfs_checkbuf_forloadfileclus(uc->fat, gDataBufSize);
  while(1)
  {
    res=fatfs_loadfileclus_usefileinfo(uc->fat, &fInfo, gDataBuf, gDataBufSize, 
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
  uint32 prevPos, prevClus, dataClusRead;
  int res, ret, bytesRead,iCur,cSum,fileSize;

  prevPos = 0;
  if(fatuc_getfileinfo(uc, sFile, &fInfo, &prevPos) != 0)
  {
    printf("ERR:testfat:fatfsfile_checksum: opening file[%s]\n", sFile);
    return -1;
  }
  fileSize = fInfo.fileSize;
  prevClus = 0;
  cSum = 0;
  ret = -1;
  fatfs_checkbuf_forloadfileclus(uc->fat, gDataBufSize);
  while(1)
  {
    res=fatfs_loadfileclus_usefileinfo(uc->fat, &fInfo, gDataBuf, gDataBufSize, 
      &dataClusRead, &prevClus);
    if((res != 0)&&(res != -ERROR_TRYAGAIN))
      break;
#if DEBUG_TESTFAT > 15    
    else
      printf("testfat:INFO: loadfileclus clusRead[%ld] prevClus[%ld]\n", 
        dataClusRead, prevClus);
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


int main(int argc, char **argv)
{
  int bExit, grpId, devId, partNo, forceMbr, forceReset;
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
  schedP.sched_priority = 50;
  if(sched_setscheduler(0,SCHED_RR,&schedP) != 0)
  {
    fprintf(stderr,"ERR:testfat: Unable to use realtime scheduling\n");	  
    exit(20);
  }
  else
    fprintf(stderr,"INFO:testfat: REALTIME Scheduling enabled\n");
  if(pa_strncmp(argv[1],"hd",2) == 0)
    bdk = &bdkHdd;
  else if(pa_strncmp(argv[1],"h8b16",5) == 0)
    bdk = &bdkH8b16;
  else
    bdk = &bdkBDFile;
  grpId = strtoul(argv[2],&pChar,0);
  devId = strtoul(&pChar[1],&pChar,0);
  partNo = strtoul(&pChar[1],NULL,0);
  
  if(argv[3][0] == 'r')
    forceReset = 1;
  else
    forceReset = 0;
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
      forceMbr,forceReset) != 0)
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
    printf("(l)dirListing (e)fileExtract (E)Exit (b)BlockDev speed\n");
    printf("(c)chDir (f)checkFile (R)Reset (s)readspeed (S)readspeedALL\n");
    printf("(X)normalfsfile checksum (Y)fatfsfile checksum\n");
    cCur = fgetc(stdin); fgetc(stdin);
    switch(cCur)
    {
    case 'l':
      printf("enter directory to list:");
      scanf("%s",sBuf1);
      fgetc(stdin);
      lu_starttime(&gTFtv1);
      testfat_dirlisting(&gUC, sBuf1);
      lu_stoptimedisp(&gTFtv1,&gTFtv2,&timeInUSECS,"dirListing");
      break;
    case 'e':
      printf("enter src and dest files:");
      scanf("%s %s", sBuf1, sBuf2);
      fgetc(stdin);
      lu_starttime(&gTFtv1);
      testfat_fileextract(&gUC, sBuf1, sBuf2);
      lu_stoptimedisp(&gTFtv1,&gTFtv2,&timeInUSECS,"fileExtract");
      break;
    case 'c':
      printf("enter dir to chdir to:");
      scanf("%s", sBuf1);
      fgetc(stdin);
      lu_starttime(&gTFtv1);
      fatuc_chdir(&gUC, sBuf1);
      lu_stoptimedisp(&gTFtv1,&gTFtv2,&timeInUSECS,"chDir");
      break;
    case 'f':
      printf("enter file to check:");
      scanf("%s",sBuf1);
      fgetc(stdin);
      lu_starttime(&gTFtv1);
      testfat_checkfile(&gUC, sBuf1);
      lu_stoptimedisp(&gTFtv1,&gTFtv2,&timeInUSECS,"checkFile");
      break;
    case 's':
      printf("enter src file for read speed test:");
      scanf("%s",sBuf1);
      fgetc(stdin);
      lu_starttime(&gTFtv1);
      testfat_checkreadspeed(&gUC, sBuf1, &fileSize);
      lu_stoptimedisp(&gTFtv1,&gTFtv2,&timeInUSECS,"readspeed");
      fprintf(stderr,"fileSize[%ld] time[%ld]usecs readSpeed/msec[%ld]\n",
        fileSize,timeInUSECS,(fileSize/(timeInUSECS/1000)));
      break;
    case 'S':
      printf("enter src file for read speed test:");
      scanf("%s",sBuf1);
      fgetc(stdin);
      savedDataBufSize = gDataBufSize;
      gDataBufSize = DATABUF_MAXSIZE;
      while(gDataBufSize >= FATFSCLUS_MAXSIZE)
      {
        fprintf(stderr,"testfat:readspeedALL: BufSize [%ld]\n",gDataBufSize);
        lu_starttime(&gTFtv1);
        testfat_checkreadspeed(&gUC, sBuf1, &fileSize);
        lu_stoptimedisp(&gTFtv1,&gTFtv2,&timeInUSECS,"readspeedALL");
        fprintf(stderr,"fileSize[%ld] time[%ld]usecs readSpeed/msec[%ld]\n",
          fileSize,timeInUSECS,(fileSize/(timeInUSECS/1000)));
        gDataBufSize = gDataBufSize/2;
      }
      gDataBufSize = savedDataBufSize;
      break;
    case 'b':
      printf("BlockDev raw speed test for [%d] sectors\n",TESTFAT_BDBM_SECS);
      lu_starttime(&gTFtv1);
      if(bdk->get_sectors_benchmark == NULL)
      {
        fprintf(stderr,"ERR:testfat: NO BlockDev benchmark function\n");
        lu_stoptimedisp(&gTFtv1,&gTFtv2,&timeInUSECS,"FAILED BlockDev raw");
	break;
      }
      if(bdk->get_sectors_benchmark(bdk,0,TESTFAT_BDBM_SECS,gDataBuf)!=0)
        fprintf(stderr,"ERR:testfat: get_sectors_benchmark failed\n");
      lu_stoptimedisp(&gTFtv1,&gTFtv2,&timeInUSECS,"BlockDev raw speed");
      fprintf(stderr,"NumSecs[%d] time[%ld]usecs readSpeed/msec[%ld]\n",
        TESTFAT_BDBM_SECS,timeInUSECS,
        ((TESTFAT_BDBM_SECS*bdk->secSize)/(timeInUSECS/1000)));
      break;
    case 'R':
      printf("Reseting blockdev [%s]\n",bdk->name);
      bdk->reset(bdk);
      break;
    case 'X':
      printf("enter normalfsfile to checksum:");
      scanf("%s", sBuf1);
      fgetc(stdin);
      lu_starttime(&gTFtv1);
      testfat_normalfsfile_checksum(sBuf1);
      lu_stoptimedisp(&gTFtv1,&gTFtv2,&timeInUSECS,"normalfsfile_checksum");
      break;
    case 'Y':
      printf("enter fatfsfile to checksum:");
      scanf("%s", sBuf1);
      fgetc(stdin);
      lu_starttime(&gTFtv1);
      testfat_fatfsfile_checksum(&gUC, sBuf1);
      lu_stoptimedisp(&gTFtv1,&gTFtv2,&timeInUSECS,"fatfsfile_checksum");
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

