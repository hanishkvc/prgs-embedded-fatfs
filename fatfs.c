/*
 * fatfs.c - library for working with fat filesystem
 * v17july2004-2130
 * C Hanish Menon <hanishkvc>, 14july2004
 * 
 * Notes
 * * doesn't understand . and .. in root dir as those entries are not
 *   present in the rootdir of fatfs
 */

#include <stdio.h>

#include <inall.h>
#include <bdfile.h>
#include <fatfs.h>
#include <errs.h>

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
    printf("ERROR:fatfs: fat32 rootDir loading  not supported\n");
    return -ERROR_NOTSUPPORTED;
  }
  return res;
}

int fatfs_loadbootsector()
{
  int CountOfClusters;
  uint8 *pCur;
  uint32 tVerify;
#if (DEBUG_PRINT_FATFS > 10)
  uint8 tBuf[32];
#endif

  bd_get_sectors(gFat.baseSec, 1, gFat.BBuf); 
  /* validate the FAT boot sector */
  pCur = gFat.BBuf+510;
  tVerify = buffer_read_uint16_le(&pCur);
  if(tVerify != 0xaa55)
  {
    printf("ERROR:fatfs:bootsector: 0xaa55 missing from offset 510\n");
    return -ERROR_INVALID;
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

#if (DEBUG_PRINT_FATFS > 10)
  printf("OEMName [%s] BytPerSec [%d] SecPerClus[%d]\n", 
    buffer_read_string_noup((gFat.BBuf+3),8,tBuf,32),gFB.bytPerSec,gFB.secPerClus);
  printf("TotSec16 [%d] TotSec32 [%ld]\n", 
    gFB.totSec16, gFB.totSec32);
  printf("NumFats [%d] FatSz16 [%d]\n",
    gFB.numFats, gFB.fatSz16);
  printf("Media [0x%x] RsvdSecCnt [%d] RootEntCnt [%d]\n",
    *BPB_Media(gFat.BBuf), gFB.rsvdSecCnt, gFB.rootEntCnt);
#endif

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
#if (DEBUG_PRINT_FATFS > 10)
    printf("FatSz32[%ld] ExtFlags[0x%x] FSVer[0x%x] RootClus[%ld] FSInfo[%d]\n",
      gFB.fatSz32, gFB.extFlags, gFB.fsVer, gFB.rootClus, gFB.fsInfo);
    printf("BootSig[0x%x] VolID[0x%lx] VolLab[%11s] FilSysType [%8s]\n",
      *BS_32BootSig(gFat.BBuf), gFB.volID, 
      buffer_read_string_noup((gFat.BBuf+71),11,tBuf,32),
      buffer_read_string_noup((gFat.BBuf+82),8, (tBuf+16),16));
#endif
  }
  else
  {
    /*
    printf("BootSig[0x%x] VolID[0x%lx] VolLab[%11s] FilSysType [%8s]\n",
      *BS_16BootSig(gFat.BBuf), gFB.volID, BS_16VolLab_sz11(gFat.BBuf), BS_16FilSysType_sz8(gFat.BBuf)); 
    */
    pCur = gFat.BBuf+39;
    gFB.volID = buffer_read_uint32_le(&pCur);
#if (DEBUG_PRINT_FATFS > 10)
    printf("BootSig[0x%x] VolID[0x%lx] VolLab[%11s] FilSysType [%8s]\n",
      *BS_16BootSig(gFat.BBuf), gFB.volID, 
      buffer_read_string_noup((gFat.BBuf+43),11,tBuf,32),
      buffer_read_string_noup((gFat.BBuf+54),8,(tBuf+16),16));
#endif
  }
  /* verify limitations of this FatFS implementation */
  if(gFB.fatSz*gFB.bytPerSec > FATFAT_MAXSIZE)
  {
    printf("ERROR:fatfs: Fat size [%ld] > FATFAT_MAXSIZE [%d]\n", 
      gFB.fatSz*gFB.bytPerSec, FATFAT_MAXSIZE);
    return -ERROR_INSUFFICIENTRESOURCE;
  }
  if(gFB.isFat12)
  {
    printf("ERROR:fatfs: Fat12 is not supported\n");
    return -ERROR_NOTSUPPORTED;
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
#if (DEBUG_PRINT_FATFS > 15)
  printf("INFO:fatfs16: firstRootDirSecNum [%ld] secs[%ld]\n", 
    firstRootDirSecNum, rootDirSecs);
#endif
  if(rootDirSecs*gFB.bytPerSec > FATROOTDIR_MAXSIZE)
  {
    printf("ERROR:fatfs16: rootDir size [%ld] > FATROOTDIR_MAXSIZE [%d]\n", 
      rootDirSecs*gFB.bytPerSec, FATROOTDIR_MAXSIZE);
    return -ERROR_INSUFFICIENTRESOURCE;
  }
  gFat.rdSize = rootDirSecs*gFB.bytPerSec;
  return bd_get_sectors(gFat.baseSec+firstRootDirSecNum, 
    rootDirSecs, gFat.RDBuf); 
}

int fatfs_getfileinfo_fromdir(char *cFile, uint8 *dirBuf, uint16 dirBufSize, 
  struct TFileInfo *fInfo, uint32 *prevPos)
{
  int iCur, iCurLFN, flagSpecialCurOrPrev;
  uint8 *pCur = dirBuf;
  int lfnPos = FATFSLFN_SIZE-1;
  char tcFile[FATDIRENTRYNAME_SIZE+1];

  /* logic added as . and .. directory entries don't have corresponding
   * lfn entry in the directory
   */
  flagSpecialCurOrPrev = 0;
  if(cFile[0] == '.')
  {
    flagSpecialCurOrPrev = 1;
    if(cFile[1] == '.')
    {
      if(cFile[2] == (char)NULL)
        strcpy(tcFile,"..         ");
      else
        flagSpecialCurOrPrev = 0;
    }
    else if(cFile[1] == (char)NULL)
      strcpy(tcFile,".          ");
    else
      flagSpecialCurOrPrev = 0;
  }
  /****************/
  iCur = *prevPos;
  while(iCur+FATDIRENTRY_SIZE  <= dirBufSize)
  {
    pCur = dirBuf+iCur;
    iCur+=FATDIRENTRY_SIZE;
    *prevPos = iCur;
    buffer_read_string(&pCur, FATDIRENTRYNAME_SIZE, fInfo->name, FATFSNAME_SIZE);
    if(fInfo->name[0] == 0) /* no more entries in dir */
      return -ERROR_NOMORE;
    else if(fInfo->name[0] == 0xe5) /* free dir entry */
      continue;
    fInfo->attr = buffer_read_uint8_le(&pCur);
    if(fInfo->attr == FATATTR_LONGNAME)
    {
      /* FIXME: The code below forcibly converts unicode to ascii
      printf("INFO:fatfs:getfileinfo_fromdir long file name \n");
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
    if(lfnPos != FATFSLFN_SIZE-1)
    {
      for(iCurLFN = 0; lfnPos != FATFSLFN_SIZE-1; iCurLFN++, lfnPos++)
        fInfo->lfn[iCurLFN] = fInfo->lfn[lfnPos];
      fInfo->lfn[iCurLFN] = 0;
    }
    else
      fInfo->lfn[0] = 0;
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
#if (DEBUG_PRINT_FATFS > 15)
    printf("INFO:fatfs: File[%s][%s] attr[0x%x] firstClus[%ld] fileSize[%ld]\n",
      fInfo->name, fInfo->lfn, fInfo->attr, fInfo->firstClus, fInfo->fileSize);
#endif
    if(flagSpecialCurOrPrev == 0)
    {
      if(strncmp(fInfo->lfn,cFile,FATFSLFN_SIZE) == 0)
        return 0;
    }
    else
    {
      if(strncmp(fInfo->name,tcFile,FATFSNAME_SIZE) == 0)
        return 0;
    }
    if(cFile[0] == 0)
      return 0;
  }
  return -ERROR_NOTFOUND;
}

int fatfs16_getopticluslist_usefileinfo(struct TFileInfo fInfo, struct TClusList *cl, int *clSize, uint32 *prevClus)
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
#if (DEBUG_PRINT_FATFS > 15)
    else
       printf("fatfs16:opticluslist: File[%s] nextClus[%ld]\n", 
         fInfo.name, iCurClus);
#endif
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
  return -ERROR_TRYAGAIN;
}

int fatfs_loadfileallsec_usefileinfo(struct TFileInfo fInfo, uint8 *buf, uint32 bufLen)
{
  uint32 prevClus, totNoSecs, noSecs, bytesRead;
  struct TClusList cl[16];
  int resOCL, resGS, clSize, iCur;
  
  if(bufLen < fInfo.fileSize)
  {
    printf("ERROR:fatfs:load: insufficient buffer passed\n");
    return -ERROR_INSUFFICIENTRESOURCE;
  }
  totNoSecs = 0;
  prevClus = 0;
  do
  {
    clSize = 16; 
    resOCL = fatfs16_getopticluslist_usefileinfo(fInfo, cl, &clSize, &prevClus);
    for(iCur=0; iCur < clSize; iCur++)
    {
      noSecs = (cl[iCur].adjClusCnt+1)*gFB.secPerClus;
      totNoSecs += noSecs;
      bytesRead = noSecs*gFB.bytPerSec;
      bufLen -= bytesRead;
      if(bufLen < 0)
      {
        if(resOCL != 0)
        {
          printf("NOTPOSSIBLE:1:fatfs:load: buffer space less by > a Clus\n");
          return -ERROR_INSUFFICIENTRESOURCE;
        }
        if(bufLen <= -(gFB.secPerClus*gFB.bytPerSec))
        {
          printf("NOTPOSSIBLE:2:fatfs:load: buffer space less by > a Clus\n");
          return -ERROR_INSUFFICIENTRESOURCE;
        }
        printf("DEBUGERROR:3:fatfs:load: buffer space less by < a clus\n");
        return -ERROR_INSUFFICIENTRESOURCE;
      }
      resGS=bd_get_sectors(fatfs_firstsecofclus(cl[iCur].baseClus),noSecs,buf);
      if(resGS != 0)
      {
        printf("DEBUG:fatfs:load: bd_get_sectors\n");
        return resGS;
      }
      buf += bytesRead; 
    }
  }while(resOCL != 0);
  return totNoSecs;
}

int fatfs_loadfilefull_usefileinfo(struct TFileInfo fInfo, uint8 *buf, uint32 bufLen)
{
  uint32 prevClus, totNoSecs, noSecs, bytesRead;
  struct TClusList cl[16];
  int resOCL, resGS, clSize, iCur;
  
  if(bufLen < fInfo.fileSize)
  {
    printf("ERROR:fatfs:load: insufficient buffer passed\n");
    return -ERROR_INSUFFICIENTRESOURCE;
  }
  totNoSecs = 0;
  prevClus = 0;
  do
  {
    clSize = 16; 
    resOCL = fatfs16_getopticluslist_usefileinfo(fInfo, cl, &clSize, &prevClus);
    for(iCur=0; iCur < clSize; iCur++)
    {
      noSecs = (cl[iCur].adjClusCnt+1)*gFB.secPerClus;
      totNoSecs += noSecs;
      bytesRead = noSecs*gFB.bytPerSec;
      bufLen -= bytesRead;
      if(bufLen < 0)
      {
        if(resOCL != 0)
        {
          printf("NOTPOSSIBLE:1:fatfs:load: buffer space less by > a Clus\n");
          return -ERROR_INSUFFICIENTRESOURCE;
        }
        if(bufLen <= -(gFB.secPerClus*gFB.bytPerSec))
        {
          printf("NOTPOSSIBLE:2:fatfs:load: buffer space less by > a Clus\n");
          return -ERROR_INSUFFICIENTRESOURCE;
        }
        printf("DEBUGERROR:3:fatfs:load: buffer space less by < a clus\n");
        bytesRead-=bufLen;
      }
      resGS=bd_get_sectors_fine(fatfs_firstsecofclus(cl[iCur].baseClus),noSecs,buf, bytesRead);
      if(resGS != 0)
      {
        printf("DEBUG:fatfs:load: bd_get_sectors_fine failed\n");
        return resGS;
      }
      buf += bytesRead; 
    }
  }while(resOCL != 0);
  return totNoSecs;
}

int fatuc_init(struct TFatFsUserContext *uc)
{
  uc->curDirBuf = gFat.RDBuf;
  uc->curDirBufLen = gFat.rdSize;
  fatuc_updatetempdirbufinfo(uc);
  return 0;
}

/*
 * Always call this before using tempDirBuf of uc
 * as tempDirBuf could point to gFat.RDBuf in some cases
 */
void fatuc_updatetempdirbufinfo(struct TFatFsUserContext *uc)
{
  if(uc->curDirBuf == (uint8*)uc->dirBuf1)
    uc->tempDirBuf = (uint8*)uc->dirBuf2;
  else
    uc->tempDirBuf = (uint8*)uc->dirBuf1;
  uc->tempDirBufLen = 0;
}

int util_extractfilefrompath(char *cPath, char *dName, int dLen, 
  char *fName, int fLen)
{
  int lastSepLoc, iPCur, iFCur, iFStart, iFEnd;

  iPCur = 0;
  lastSepLoc = -1;
  for(iPCur = 0; cPath[iPCur] != (char)NULL; iPCur++)
  {
    if(iPCur >= dLen)
      return -ERROR_INSUFFICIENTRESOURCE;
    if(cPath[iPCur] == FATFS_DIRSEP)
      lastSepLoc = iPCur;
    dName[iPCur] = cPath[iPCur]; 
  }
  if(lastSepLoc == -1)
  {
    dName[0] = 0;
    iFStart = 0;
    iFEnd = iPCur;
  }
  else
  {
    dName[lastSepLoc+1] = 0;
    iFStart = lastSepLoc+1;
    iFEnd = iPCur;
  }
  if((iFEnd-iFStart+1) > fLen)
    return -ERROR_INSUFFICIENTRESOURCE;
  for(iFCur=iFStart; iFCur <= iFEnd; iFCur++)
  {
    *fName = cPath[iFCur];
    fName++;
  }
  return 0;
}

int util_gettoken(char **cBuf, char *token, int tokenLen, char sep)
{
  int iPos = 0;
  char cCur;

  token[iPos] = (uint8)NULL;
  while((cCur=**cBuf) != (uint8)NULL)
  {
    (*cBuf)++;
    if(cCur == sep)
    {
      token[iPos] = 0;
      return 0;
    }
    token[iPos++] = cCur;
    if(iPos >= tokenLen)
      return -ERROR_INSUFFICIENTRESOURCE;
  }
  if(iPos == 0)
    return -ERROR_NOMORE;
  token[iPos] = 0;
  return 0;
}

/*
 * if user gives wrong path then it won't affect the uc
 */
int fatuc_changedir(struct TFatFsUserContext *uc, char *fDirName, int bUpdateCurDir)
{
  uint16 tokenLen = (FATDIRENTRYLFN_SIZE+1);
  uint8 *dBuf;
  uint32 dBufLen;
  char *sDirName = fDirName, token[tokenLen];;

  if(sDirName[0] == FATFS_DIRSEP)
  {
    sDirName++;
    dBuf = gFat.RDBuf;
    dBufLen = gFat.rdSize;
  }
  else
  {
    dBuf = uc->curDirBuf;
    dBufLen = uc->curDirBufLen;
  }
  fatuc_updatetempdirbufinfo(uc);

  /* Extract individual dir names and get the corresponding dirBuf
   * next get the subsequent subdir mentioned in the path
   * till the leaf dir is found
   */
  while(util_gettoken(&sDirName, token, tokenLen, FATFS_DIRSEP) == 0)
  {
    uint32 prevPos, res;
    struct TFileInfo fInfo;

    printf("fatfs:chdir:token [%s]\n", token);
    prevPos = 0;
    res = fatfs_getfileinfo_fromdir(token, dBuf, dBufLen, &fInfo, &prevPos);
    if(res != 0)
    {
      printf("fatfs:chdir:subdir[%s] not found\n", token);
      return -ERROR_NOTFOUND;
    }
    if(fInfo.attr != FATATTR_DIR)
    {
      printf("fatfs:chdir:NOT A directory [%s]\n", token);
      return -ERROR_INVALID;
    }
    /* special case of root dir got from a rootdirs_subdir\.. */
    if((fInfo.firstClus == 0) && (fInfo.fileSize == 0))
    {
      dBuf = gFat.RDBuf;
      dBufLen = gFat.rdSize;
    }
    else
    {
      res = fatfs_loadfileallsec_usefileinfo(fInfo, uc->tempDirBuf, 
        FATFSUSERCONTEXTDIRBUF_MAXSIZE);
      dBuf = uc->tempDirBuf;
      dBufLen = res*gFB.bytPerSec;
      if(res <=  0)
      {
        printf("fatfs:chdir:loading subdir[%s] data failed with [%ld]\n",
          fInfo.lfn, res);
        return res;
      }
    }
  }
  if(bUpdateCurDir)
  {
    uc->curDirBuf = dBuf;
    uc->curDirBufLen = dBufLen;
    fatuc_updatetempdirbufinfo(uc);
  }
  else
  {
    uc->tempDirBuf = dBuf;
    uc->tempDirBufLen = dBufLen;
  }
  return 0;
}
 
int fatuc_chdir(struct TFatFsUserContext *uc, char *fDirName)
{
  return fatuc_changedir(uc, fDirName, 1);
}

int fatuc_getfileinfo(struct TFatFsUserContext *uc, char *cFile,  
  struct TFileInfo *fInfo, uint32 *prevPos)
{
  int res;

  res = util_extractfilefrompath(cFile, uc->pName, FATFSPATHNAME_MAXSIZE, 
    uc->fName, FATFSLFN_SIZE);
  if(res != 0)
  {
    printf("ERROR:fatuc:getfileinfo: extractfilefrompath failed\n");
    return res;
  }
  
  if(uc->pName[0] == 0)
    return fatfs_getfileinfo_fromdir(uc->fName, uc->curDirBuf, 
      uc->curDirBufLen, fInfo, prevPos);
  else
  {
    printf("INFO:fatuc:getfileinfo: work with curDir Files for EFFICIENCY\n");
    res = fatuc_changedir(uc, uc->pName, 0);
    if(res != 0)
    {
      printf("ERROR:fatuc:getfileinfo: changedir failed\n");
      return res;
    }
    return fatfs_getfileinfo_fromdir(uc->fName, uc->tempDirBuf, 
      uc->tempDirBufLen, fInfo, prevPos);
  }
}

int fatfs_cleanup()
{
  return 0;
}

