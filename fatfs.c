/*
 * fatfs.c - library for working with fat filesystem
 * v15july2004-2200
 * C Hanish Menon, 14july2004
 * 
 */

#include <stdio.h>
#include <bdfile.h>
#include <fatfs.h>

 /* FATBOOTSEC_MAXSIZE/4 , would align to 4byte boundry */
uint32 _FBBuf[FATBOOTSEC_MAXSIZE/4];
uint32 _FFBuf[FATFAT_MAXSIZE/4]; 
uint32 _FRDBuf[FATROOTDIR_MAXSIZE/4];
/*
uint8 *gFat.BBuf = (uint8*)&_FBBuf;
uint8 *gFFBuf = (uint8*)&_FFBuf;
*/
struct TFatBootSector gFB;
struct TFat gFat;

int fatfs_init(int baseSec)
{
  int res;

  gFat.baseSec = baseSec;
  gFat.BBuf = (uint8*)&_FBBuf;
  gFat.FBuf = (uint8*)&_FFBuf;
  gFat.RDBuf = (uint8*)&_FRDBuf;
  res = fatfs_loadbootsector();
  if(res != 0) return res;
  res = fatfs_loadfat();
  if(res != 0) return res;
  if(gFB.isFat16)
    res = fatfs16_loadrootdir();
  else
  {
    printf("ERROR:fatfs: fat32 root dir loading  not supported currently\n");
    return -1;
  }
  return res;
}

int fatfs_loadbootsector()
{
  int CountOfClusters;
  uint8 *pCur, tBuf[32];
  uint32 tVerify;

  bd_get_sectors(gFat.baseSec, 1, gFat.BBuf); 
  /* validate the FAT boot sector */
  pCur = gFat.BBuf+510;
  tVerify = buffer_read_uint16_le(&pCur);
  if(tVerify != 0xaa55)
  {
    printf("ERROR: 0xaa55 missing from offset 510\n");
    return -1;
  }
  /* extract the FAT boot sector info */
  pCur = gFat.BBuf;
  pCur+=11;
  gFB.bytPerSec = buffer_read_uint16_le(&pCur);
  gFB.secPerClus = buffer_read_uint8_le(&pCur);
  gFB.rsvdSecCnt = buffer_read_uint16_le(&pCur);
  gFB.numFats = buffer_read_uint8_le(&pCur);
  gFB.rootEntCnt = buffer_read_uint16_le(&pCur);
  gFB.totSec16 = buffer_read_uint16_le(&pCur);
  pCur++;
  gFB.fatSz16 = buffer_read_uint16_le(&pCur);
  pCur+=8;
  gFB.totSec32 = buffer_read_uint32_le(&pCur);
  gFB.fatSz32 = buffer_read_uint32_le(&pCur);
  gFB.extFlags = buffer_read_uint16_le(&pCur);
  gFB.fsVer = buffer_read_uint16_le(&pCur);
  gFB.rootClus = buffer_read_uint32_le(&pCur);
  gFB.fsInfo = buffer_read_uint16_le(&pCur);

  printf("OEMName [%s] BytPerSec [%d] SecPerClus[%d]\n", 
    buffer_read_string_noup((gFat.BBuf+3),8,tBuf,32),gFB.bytPerSec,gFB.secPerClus);
  printf("TotSec16 [%d] TotSec32 [%ld]\n", 
    gFB.totSec16, gFB.totSec32);
  printf("NumFats [%d] FatSz16 [%d]\n",
    gFB.numFats, gFB.fatSz16);
  printf("Media [%x] RsvdSecCnt [%d] RootEntCnt [%d]\n",
    *BPB_Media(gFat.BBuf), gFB.rsvdSecCnt, gFB.rootEntCnt);

  gFB.rootDirSecs = ((gFB.rootEntCnt*32)+(gFB.bytPerSec-1))/gFB.bytPerSec;
  if(gFB.fatSz16 != 0) gFB.fatSz = gFB.fatSz16; else gFB.fatSz = gFB.fatSz32;
  gFB.firstDataSec = gFB.rsvdSecCnt + gFB.numFats*gFB.fatSz + gFB.rootDirSecs;
  if(gFB.totSec16 != 0) gFB.totSec=gFB.totSec16; else gFB.totSec=gFB.totSec32;
  gFB.dataSec = gFB.totSec - gFB.firstDataSec;
  CountOfClusters = gFB.dataSec/gFB.secPerClus;
  gFB.isFat12 = 0; gFB.isFat16 = 0; gFB.isFat32 = 0;
  if(CountOfClusters < 4085)
  {
    gFB.isFat12 = 1;
    printf("**Fat12**\n");
  }
  else if(CountOfClusters < 65525)
  {
    gFB.isFat16 = 1;
    printf("**Fat16**\n");
  }
  else
  {
    gFB.isFat32 = 1;
    printf("**Fat32**\n");
  }
  if(gFB.isFat32)
  {
    pCur = gFat.BBuf+67;
    gFB.volID = buffer_read_uint32_le(&pCur);
    printf("FatSz32[%ld] ExtFlags[%x] FSVer[%x] RootClus[%ld] FSInfo[%d]\n",
      gFB.fatSz32, gFB.extFlags, gFB.fsVer, gFB.rootClus, gFB.fsInfo);
    printf("BootSig[%x] VolID[%lx] VolLab[%11s] FilSysType [%8s]\n",
      *BS_32BootSig(gFat.BBuf), gFB.volID, 
      buffer_read_string_noup((gFat.BBuf+71),11,tBuf,32),
      buffer_read_string_noup((gFat.BBuf+82),8, (tBuf+16),16));
  }
  else
  {
    /*
    //gFB.rootClus = *BPB_32RootClus(gFat.BBuf);
    gFB.volID = *BS_16VolID(gFat.BBuf);
    printf("BootSig[%x] VolID[%lx] VolLab[%11s] FilSysType [%8s]\n",
      *BS_16BootSig(gFat.BBuf), gFB.volID, BS_16VolLab_sz11(gFat.BBuf), BS_16FilSysType_sz8(gFat.BBuf)); 
    */
    pCur = gFat.BBuf+39;
    gFB.volID = buffer_read_uint32_le(&pCur);
    printf("BootSig[%x] VolID[%lx] VolLab[%11s] FilSysType [%8s]\n",
      *BS_16BootSig(gFat.BBuf), gFB.volID, 
      buffer_read_string_noup((gFat.BBuf+43),11,tBuf,32),
      buffer_read_string_noup((gFat.BBuf+54),8,(tBuf+16),16));
  }
  /* verify limitations of this FatFS implementation */
  if(gFB.fatSz*gFB.bytPerSec > FATFAT_MAXSIZE)
  {
    printf("ERROR:fatfs: Fat size [%ld] > FATFAT_MAXSIZE [%d]\n", 
      gFB.fatSz*gFB.bytPerSec, FATFAT_MAXSIZE);
    return -2;
  }
  if(gFB.isFat12)
  {
    printf("ERROR:fatfs: Fat12 is not currently supported\n");
    return -3;
  }
  return 0;
}

int fatfs_loadfat()
{
  return bd_get_sectors(gFat.baseSec+gFB.rsvdSecCnt, gFB.fatSz, gFat.FBuf); 
}
 
int fatfs16_loadrootdir()
{
  uint32 firstRootDirSecNum, rootDirSecs;

  firstRootDirSecNum = gFB.rsvdSecCnt + (gFB.numFats*gFB.fatSz);
  rootDirSecs = (gFB.rootEntCnt*FATDIRENTRY_SIZE + (gFB.bytPerSec-1))/gFB.bytPerSec;
  printf("INFO:fatfs16: firstRootDirSecNum [%ld] secs[%ld]\n", 
    firstRootDirSecNum, rootDirSecs);
  if(rootDirSecs*gFB.bytPerSec > FATROOTDIR_MAXSIZE)
  {
    printf("ERROR:fatfs16: rootDir size [%ld] > FATROOTDIR_MAXSIZE [%d]\n", 
      rootDirSecs*gFB.bytPerSec, FATROOTDIR_MAXSIZE);
    return -1;
  }
  gFat.rdSize = rootDirSecs*gFB.bytPerSec;
  return bd_get_sectors(gFat.baseSec+firstRootDirSecNum, 
    rootDirSecs, gFat.RDBuf); 
}

int fatfs_fileinfo_indir(char *cFile, uint8 *dirBuf, uint16 dirBufSize, 
  struct TFileInfo *fInfo)
{
  int iCur = 0, iCurLFN;
  uint8 *pCur = dirBuf;
  int lfnPos = FILEINFOLFN_SIZE-1;

  while(iCur+FATDIRENTRY_SIZE  <= dirBufSize)
  {
    pCur = dirBuf+iCur;
    iCur+=FATDIRENTRY_SIZE;
    buffer_read_string(&pCur, FATDIRENTRYNAME_SIZE, fInfo->name, FILEINFONAME_SIZE);
    if(fInfo->name[0] == 0) /* no more entries in dir */
      return -1;
    else if(fInfo->name[0] == 0xe5) /* free dir entry */
      continue;
    fInfo->attr = buffer_read_uint8_le(&pCur);
    if(fInfo->attr == FATATTR_LONGNAME)
    {
      /* The code below forcibly converts unicode to ascii
      printf("INFO:fatfs:fileinfo_indir long file name \n");
      should be PARTIALCHARS*2 for unicode
      */
      lfnPos-=FATDIRENTRYLFN_PARTIALCHARS; 
      pCur = dirBuf+iCur-FATDIRENTRY_SIZE;
      pCur++; /* skip ordinal */
      fInfo->lfn[lfnPos++] = buffer_read_uint8_le(&pCur);
      pCur++; /* skip unicode 1stbyte of char just read */
      fInfo->lfn[lfnPos++] = buffer_read_uint8_le(&pCur);
      pCur++; /* skip unicode 1stbyte of char just read */
      fInfo->lfn[lfnPos++] = buffer_read_uint8_le(&pCur);
      pCur++; /* skip unicode 1stbyte of char just read */
      fInfo->lfn[lfnPos++] = buffer_read_uint8_le(&pCur);
      pCur++; /* skip unicode 1stbyte of char just read */
      fInfo->lfn[lfnPos++] = buffer_read_uint8_le(&pCur);
      pCur++; /* skip unicode 1stbyte of char just read */
      pCur+=3; /* skip attr, type and checksum */
      fInfo->lfn[lfnPos++] = buffer_read_uint8_le(&pCur);
      pCur++; /* skip unicode 1stbyte of char just read */
      fInfo->lfn[lfnPos++] = buffer_read_uint8_le(&pCur);
      pCur++; /* skip unicode 1stbyte of char just read */
      fInfo->lfn[lfnPos++] = buffer_read_uint8_le(&pCur);
      pCur++; /* skip unicode 1stbyte of char just read */
      fInfo->lfn[lfnPos++] = buffer_read_uint8_le(&pCur);
      pCur++; /* skip unicode 1stbyte of char just read */
      fInfo->lfn[lfnPos++] = buffer_read_uint8_le(&pCur);
      pCur++; /* skip unicode 1stbyte of char just read */
      fInfo->lfn[lfnPos++] = buffer_read_uint8_le(&pCur);
      pCur++; /* skip unicode 1stbyte of char just read */
      pCur+=2; /* cluster */
      fInfo->lfn[lfnPos++] = buffer_read_uint8_le(&pCur);
      pCur++; /* skip unicode 1stbyte of char just read */
      fInfo->lfn[lfnPos++] = buffer_read_uint8_le(&pCur);
      pCur++; /* skip unicode 1stbyte of char just read */
      lfnPos-=FATDIRENTRYLFN_PARTIALCHARS;
      continue;
    }
    if(lfnPos != FILEINFOLFN_SIZE-1)
    {
      for(iCurLFN = 0; lfnPos != FILEINFOLFN_SIZE-1; iCurLFN++, lfnPos++)
        fInfo->lfn[iCurLFN] = fInfo->lfn[lfnPos];
      fInfo->lfn[iCurLFN] = 0;
    }
    fInfo->ntRes = buffer_read_uint8_le(&pCur);
    fInfo->crtTimeTenth = buffer_read_uint8_le(&pCur);
    fInfo->crtTime = buffer_read_uint16_le(&pCur);
    fInfo->crtDate = buffer_read_uint16_le(&pCur);
    fInfo->lastAccDate = buffer_read_uint16_le(&pCur);
    fInfo->firstClus = buffer_read_uint16_le(&pCur);
    fInfo->firstClus =  (fInfo->firstClus << 16);
    fInfo->wrtTime = buffer_read_uint16_le(&pCur);
    fInfo->wrtDate = buffer_read_uint16_le(&pCur);
    fInfo->firstClus |= buffer_read_uint16_le(&pCur);
    fInfo->fileSize = buffer_read_uint32_le(&pCur);
    printf("Info:fatfs: File[%s][%s] attr[%x] firstClus[%ld] fileSize[%ld]\n",
      fInfo->name, fInfo->lfn, fInfo->attr, fInfo->firstClus, fInfo->fileSize);
    if(strncmp(fInfo->lfn,cFile,FILEINFOLFN_SIZE) == 0)
      return 0;
  }
  return -1;
}

int fatfs16_getfile_opticlusterlist(struct TFileInfo fInfo, struct TClusList *cl, int *clSize, uint32 *prevClus)
{
  uint32 iCL = 0, iCurClus, iPrevClus;

  if(*prevClus == 0)
  {
    cl[iCL].baseClus = fInfo.firstClus;
    iPrevClus = fInfo.firstClus;
    cl[iCL].adjClusCnt = 0;
  }
  else
  {
    cl[iCL].baseClus = *prevClus;
    iPrevClus = *prevClus;
    cl[iCL].adjClusCnt = 0;
  }
  while(iCL < *clSize)
  {
    iCurClus = ((uint16*)gFat.FBuf)[FAT16Offset(iPrevClus)/2];
    if(iCurClus >= 0xFFF8) /* EOF reached */
    {
       *clSize = iCL+1; /* *clSize returns the count of entries */
       return 0;
    }
    else
       printf("fatfs16:opticluslist: File[%s] nextClus[%ld]\n", 
         fInfo.name, iCurClus);
    if((iCurClus-iPrevClus) == 1)
    {
      cl[iCL].adjClusCnt++;
    }
    else
    {
      iCL++;
      if(iCL >= *clSize) 
      {
        /* we have used up all the cl so 
         * call once again with updated *prevClus
         */
        *prevClus = iCurClus;
        break;
      }
      cl[iCL].baseClus = iCurClus;
      cl[iCL].adjClusCnt = 0;
    }
    iPrevClus = iCurClus;
  }
  *clSize = iCL;
  return -1;
}

int fatfs_cleanup()
{
  return 0;
}

