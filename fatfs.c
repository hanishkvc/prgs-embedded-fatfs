/*
 * fatfs.c - library for working with fat filesystem
 * v12Oct2004-1811
 * C Hanish Menon <hanishkvc>, 14july2004
 * 
 * Notes
 * * doesn't understand . and .. in root dir as those entries are not
 *   present in the rootdir of fatfs
 */

#include <stdio.h>

#include <inall.h>
#include <fatfs.h>
#include <errs.h>


int fatfs_init(struct TFat *fat, struct TFatBuffers *fBufs,
      bdkT *bd, int baseSec, int totSecs)
{
  int res;

  fprintf(stderr,"fatfs:[%s]\n",FATFS_LIBVER);
#ifdef FATFS_FAT_FULLYMAPPED
  fprintf(stderr,"fatfs: FULLYMAPPED\n");
#endif
#ifdef FATFS_FAT_PARTLYMAPPED
  fprintf(stderr,"fatfs: PARTLYMAPPED\n");
#endif
#ifdef FATFS_SIMULATE_ERROR
  fprintf(stderr,"fatfs: SimulateError enabled, some operations can fail\n");
#endif
  fat->bd = bd;
  fat->baseSec = baseSec; fat->totSecs = totSecs;
  fat->BBuf = (uint8*)&(fBufs->FBBuf);
  fat->FBuf = (uint8*)&(fBufs->FFBuf);
  fat->RDBuf = (uint8*)&(fBufs->FRDBuf);
  res = fatfs_loadbootsector(fat);
  if(res != 0) return res;
  /* Check certain limits */
  if(fat->bs.totSec > fat->totSecs)
  {
    fprintf(stderr,"ERR:fatfs:fatfs totSec[%ld] > partition totSecs[%ld]\n",
      fat->bs.totSec, fat->totSecs);
    return -ERROR_INVALID;
  }
  /* Handle fat types */
  if(fat->bs.isFat16)
  {
    fat->loadrootdir = fatfs16_loadrootdir;
    fat->getfatentry = fatfs16_getfatentry;
    fat->checkfatbeginok = fatfs16_checkfatbeginok;
    fat->curFatNo = 0;
    fat->maxFatEntry = ((fat->bs.fatSz*fat->bs.bytPerSec)/2)-1;
  }
  else if(fat->bs.isFat32)
  {
    fat->loadrootdir = fatfs32_loadrootdir;
    fat->getfatentry = fatfs32_getfatentry;
    fat->checkfatbeginok = fatfs32_checkfatbeginok;
    fat->curFatNo = FAT32_EXTFLAGS_ACTIVEFAT(fat->bs.extFlags);
    fat->maxFatEntry = ((fat->bs.fatSz*fat->bs.bytPerSec)/4)-1;
  }
  else
  {
    fprintf(stderr,"ERR:fatfs: unsupported fat type\n");
    return -ERROR_NOTSUPPORTED;
  }
  
  res = fatfs_loadfat(fat,fat->curFatNo,0);
  if(res != 0) return res;
  if((res=fat->checkfatbeginok(fat)) != 0) return res;
  res = fat->loadrootdir(fat);
  return res;
}

int fatfs16_checkfatbeginok(struct TFat *fat)
{
  uint32 iTemp1,iTemp2,iTemp3;

  fat->getfatentry(fat,0,&iTemp1,&iTemp2);
  fat->getfatentry(fat,1,&iTemp3,&iTemp1);
  printf("INFO:Fat[0]=[0x%lx],Fat[1]=[0x%lx]\n",iTemp2,iTemp1);
  if((iTemp1 & FAT16_CLEANSHUTBIT) == 0)
  {
    fprintf(stderr,"ERR:fatfs:Volume wasn't cleanly shutdown, so dirty\n");
    return -ERROR_FATFS_NOTCLEANSHUT;
  }
  if((iTemp1 & FAT16_HARDERRBIT) == 0)
  {
    fprintf(stderr,"ERR:fatfs:disk I/O error encountered during last mount\n");
    return -ERROR_FATFS_HARDERR;
  }
  return 0;
}

int fatfs32_checkfatbeginok(struct TFat *fat)
{
  uint32 iTemp1,iTemp2,iTemp3;

  fat->getfatentry(fat,0,&iTemp1,&iTemp2);
  fat->getfatentry(fat,1,&iTemp3,&iTemp1);
  printf("INFO:Fat[0]=[0x%lx],Fat[1]=[0x%lx]\n",iTemp2,iTemp1);
  if((iTemp1 & FAT32_CLEANSHUTBIT) == 0)
  {
    fprintf(stderr,"ERR:fatfs:Volume wasn't cleanly shutdown, so dirty\n");
    return -ERROR_FATFS_NOTCLEANSHUT;
  }
  if((iTemp1 & FAT32_HARDERRBIT) == 0)
  {
    fprintf(stderr,"ERR:fatfs:disk I/O error encountered during last mount\n");
    return -ERROR_FATFS_HARDERR;
  }
  return 0;
}

int fatfs_loadbootsector(struct TFat *fat)
{
  int CountOfClusters, res;
  uint8 *pCur;
  uint32 tVerify;
  uint8 bpbMedia;
#ifdef DEBUG_PRINT_FATFS_BOOTSEC
  uint8 tBuf[32];
#endif

  res=fat->bd->get_sectors(fat->bd,fat->baseSec,1,fat->BBuf);
  if(res != 0) return res;
  /* validate the FAT boot sector */
  pCur = fat->BBuf+510;
  tVerify = buffer_read_uint16_le(&pCur);
  if(tVerify != 0xaa55)
  {
    fprintf(stderr,"ERR:fatfs:bootsector: 0xaa55 missing from offset 510\n");
    return -ERROR_INVALID;
  }
  /* extract the FAT boot sector info */
  pCur = fat->BBuf;
  pCur+=11;
  fat->bs.bytPerSec = buffer_read_uint16_le(&pCur);
  fat->bs.secPerClus = buffer_read_uint8_le(&pCur);
  fat->bs.rsvdSecCnt = buffer_read_uint16_le(&pCur);
  fat->bs.numFats = buffer_read_uint8_le(&pCur);
  fat->bs.rootEntCnt = buffer_read_uint16_le(&pCur);
  fat->bs.totSec16 = buffer_read_uint16_le(&pCur);
  bpbMedia=buffer_read_uint8_le(&pCur);
  fat->bs.fatSz16 = buffer_read_uint16_le(&pCur);
  pCur+=8;
  fat->bs.totSec32 = buffer_read_uint32_le(&pCur);
  fat->bs.fatSz32 = buffer_read_uint32_le(&pCur);
  fat->bs.extFlags = buffer_read_uint16_le(&pCur);
  fat->bs.fsVer = buffer_read_uint16_le(&pCur);
  fat->bs.rootClus = buffer_read_uint32_le(&pCur);
  fat->bs.fsInfo = buffer_read_uint16_le(&pCur);

#ifdef DEBUG_PRINT_FATFS_BOOTSEC
  printf("OEMName [%s] BytPerSec [%d] SecPerClus[%d]\n", 
    buffer_read_string_noup((fat->BBuf+3),8,tBuf,32),fat->bs.bytPerSec,
    fat->bs.secPerClus);
  printf("TotSec16 [%d] TotSec32 [%ld]\n", 
    fat->bs.totSec16, fat->bs.totSec32);
  printf("NumFats [%d] FatSz16 [%d]\n",
    fat->bs.numFats, fat->bs.fatSz16);
  printf("Media [0x%x] RsvdSecCnt [%d] RootEntCnt [%d]\n",
    bpbMedia, fat->bs.rsvdSecCnt, fat->bs.rootEntCnt);
#endif

  fat->bs.rootDirSecs = ((fat->bs.rootEntCnt*FATDIRENTRY_SIZE)+(fat->bs.bytPerSec-1))/fat->bs.bytPerSec;
  if(fat->bs.fatSz16 != 0) fat->bs.fatSz = fat->bs.fatSz16; else fat->bs.fatSz = fat->bs.fatSz32;
  fat->bs.firstDataSec = fat->bs.rsvdSecCnt + fat->bs.numFats*fat->bs.fatSz + fat->bs.rootDirSecs;
  if(fat->bs.totSec16 != 0) fat->bs.totSec=fat->bs.totSec16; else fat->bs.totSec=fat->bs.totSec32;
  fat->bs.dataSec = fat->bs.totSec - fat->bs.firstDataSec;
  CountOfClusters = fat->bs.dataSec/fat->bs.secPerClus;
  fat->bs.isFat12 = 0; fat->bs.isFat16 = 0; fat->bs.isFat32 = 0;
  if(CountOfClusters < 4085)
  {
    fat->bs.isFat12 = 1;
    printf("**Fat12**\n");
  }
  else if(CountOfClusters < 65525)
  {
    fat->bs.isFat16 = 1;
    printf("**Fat16**\n");
  }
  else
  {
    fat->bs.isFat32 = 1;
    printf("**Fat32**\n");
  }
  if((fat->bs.rootEntCnt == 0) && (fat->bs.isFat32 == 0))
  {
    fat->bs.isFat12 = 0; fat->bs.isFat16 = 0; fat->bs.isFat32 = 1;
    printf("**AUTOForced to Fat32**\n");
  }
  if(fat->bs.isFat32)
  {
    pCur = fat->BBuf+67;
    fat->bs.volID = buffer_read_uint32_le(&pCur);
#ifdef DEBUG_PRINT_FATFS_BOOTSEC
    printf("FatSz32[%ld]ExtFlags[0x%x]FSVer[0x%x]RootClus[%ld]FSInfo[%d]\n",
      fat->bs.fatSz32, fat->bs.extFlags, fat->bs.fsVer,
      fat->bs.rootClus, fat->bs.fsInfo);
    printf("BootSig[0x%x] VolID[0x%lx] VolLab[%11s] FilSysType [%8s]\n",
      buffer_read_uint8_le_noup((fat->BBuf+66)), fat->bs.volID, 
      buffer_read_string_noup((fat->BBuf+71),11,tBuf,32),
      buffer_read_string_noup((fat->BBuf+82),8, (tBuf+16),16));
#endif
  }
  else
  {
    /*
    printf("BootSig[0x%x] VolID[0x%lx] VolLab[%11s] FilSysType [%8s]\n",
      *BS_16BootSig(fat->BBuf), fat->bs.volID, BS_16VolLab_sz11(fat->BBuf), BS_16FilSysType_sz8(fat->BBuf)); 
    */
    pCur = fat->BBuf+39;
    fat->bs.volID = buffer_read_uint32_le(&pCur);
#ifdef DEBUG_PRINT_FATFS_BOOTSEC
    printf("BootSig[0x%x] VolID[0x%lx] VolLab[%11s] FilSysType [%8s]\n",
      buffer_read_uint8_le_noup(fat->BBuf+38), fat->bs.volID, 
      buffer_read_string_noup((fat->BBuf+43),11,tBuf,32),
      buffer_read_string_noup((fat->BBuf+54),8,(tBuf+16),16));
#endif
  }
  /* verify limitations of this FatFS implementation */
#ifdef FATFS_FAT_FULLYMAPPED
  if(fat->bs.fatSz*fat->bs.bytPerSec > FATFAT_MAXSIZE)
  {
    fprintf(stderr,"ERR:fatfs: Fat size [%ld] > FATFAT_MAXSIZE [%d]\n", 
      fat->bs.fatSz*fat->bs.bytPerSec, FATFAT_MAXSIZE);
    return -ERROR_INSUFFICIENTRESOURCE;
  }
#endif  
  if(fat->bs.isFat12)
  {
    fprintf(stderr,"ERR:fatfs: Fat12 is not supported\n");
    return -ERROR_NOTSUPPORTED;
  }
  return 0;
}

int fatfs_loadfat(struct TFat *fat, int fatNo, int startEntry)
{
  int fatStartSec, nEntries, nSecs, startSec;

#ifndef FATFS_SIMULATE_ERROR  
  if(startEntry < 0) startEntry = 0; 
#endif  
  nSecs = (FATFAT_MAXSIZE/fat->bs.bytPerSec);
  nEntries = nSecs*fat->bs.bytPerSec;
  if(fat->bs.isFat16)
  {
    startSec = (startEntry*2)/fat->bs.bytPerSec;
    startEntry = (startSec*fat->bs.bytPerSec)/2;
    nEntries = nEntries/2;
  }
  else if(fat->bs.isFat32) 
  {
    startSec = (startEntry*4)/fat->bs.bytPerSec;
    startEntry = (startSec*fat->bs.bytPerSec)/4;
    nEntries = nEntries/4;
  }
  else 
  {
    fprintf(stderr,"DEBUG:FATFS:loadfat: NOTPOSSIBLE unsupported Fat type\n");
    return -ERROR_NOTSUPPORTED;
  }
  
  fat->curFatStartEntry = startEntry;
  fat->curFatEndEntry = startEntry+nEntries-1;
  if(fat->curFatEndEntry > fat->maxFatEntry)
    fat->curFatEndEntry = fat->maxFatEntry;

  fatStartSec = fat->baseSec+fat->bs.rsvdSecCnt+fatNo*fat->bs.fatSz;
  startSec = fatStartSec + startSec;
  printf("INFO:fatfs:loadfat: curFat->StartEntry[%ld]EndEntry[%ld]\n",
    fat->curFatStartEntry, fat->curFatEndEntry);
  return fat->bd->get_sectors(fat->bd, startSec, nSecs,fat->FBuf); 
}
 
int fatfs16_loadrootdir(struct TFat *fat)
{
  uint32 firstRootDirSecNum, rootDirSecs;

  firstRootDirSecNum = fat->bs.rsvdSecCnt + (fat->bs.numFats*fat->bs.fatSz);
  rootDirSecs = (fat->bs.rootEntCnt*FATDIRENTRY_SIZE + (fat->bs.bytPerSec-1))/fat->bs.bytPerSec;
#if (DEBUG_PRINT_FATFS > 15)
  printf("INFO:fatfs16: firstRootDirSecNum [%ld] secs[%ld]\n", 
    firstRootDirSecNum, rootDirSecs);
#endif
  if(rootDirSecs*fat->bs.bytPerSec > FATROOTDIR_MAXSIZE)
  {
    printf("ERROR:fatfs16: rootDir size [%ld] > FATROOTDIR_MAXSIZE [%d]\n", 
      rootDirSecs*fat->bs.bytPerSec, FATROOTDIR_MAXSIZE);
    return -ERROR_INSUFFICIENTRESOURCE;
  }
  fat->rdSize = rootDirSecs*fat->bs.bytPerSec;
  return fat->bd->get_sectors(fat->bd,fat->baseSec+firstRootDirSecNum, 
    rootDirSecs,fat->RDBuf); 
}

int fatfs32_loadrootdir(struct TFat *fat)
{
  int res;
  uint32 totSecsRead;
  struct TFileInfo fInfo;

  fInfo.firstClus = fat->bs.rootClus;
  fInfo.fileSize = FATROOTDIR_MAXSIZE;
  res = fatfs_loadfileallsec_usefileinfo(fat, &fInfo, fat->RDBuf, 
       FATROOTDIR_MAXSIZE, &totSecsRead);
  fat->rdSize = totSecsRead*fat->bs.bytPerSec;
  if(res !=  0)
     fprintf(stderr,"ERR:fatfs32:rootdir:load failed with[%d]\n", res);
  return res;
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
    buffer_read_string(&pCur,FATDIRENTRYNAME_SIZE,fInfo->name,FATFSNAME_SIZE);
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
    printf("INFO:fatfs:File[%s][%s] attr[0x%x] firstClus[%ld]fileSize[%ld]\n",
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

int fatfs_partlymappedfat_handle(struct TFat *fat, 
      uint32 iEntry, uint32 *pMappedEntry)
{
  if((iEntry < fat->curFatStartEntry) || (iEntry > fat->curFatEndEntry))
  {
    if(fatfs_loadfat(fat,fat->curFatNo,
      iEntry-((fat->curFatEndEntry-fat->curFatStartEntry)/2)) != 0)
    {
      fprintf(stderr,"ERR:fatfs: failed partlymappedfat loading\n");
      return -ERROR_FAILED;
    }
    printf("INFO:fatfs: fat partly mapped in\n");
  }
  *pMappedEntry = iEntry-fat->curFatStartEntry;
  return 0;
}

int fatfs16_getfatentry(struct TFat *fat, uint32 iEntry,
      uint32 *iValue, uint32 *iActual)
{
  uint32 iCurClus;

  if(iEntry > fat->maxFatEntry)
  {
    fprintf(stderr,"ERR:fatfs16: fatentry[%ld] > maxFatEntry[%ld]\n",
      iEntry,fat->maxFatEntry);
    return -ERROR_INVALID;
  }
#ifdef FATFS_FAT_FULLYMAPPED 
  *iActual = ((uint16*)fat->FBuf)[iEntry];
#endif
#ifdef FATFS_FAT_PARTLYMAPPED
  {
  uint32 iMappedEntry;
  if(fatfs_partlymappedfat_handle(fat,iEntry,&iMappedEntry) != 0) 
    return -ERROR_FAILED;
  *iActual = ((uint16*)fat->FBuf)[iMappedEntry];
  }
#endif
  iCurClus = *iActual;
  if(iCurClus >= FAT16_EOF) /* EOF reached */
    return -ERROR_FATFS_EOF;
  else if(iCurClus == FAT16_BADCLUSTER) /* Bad cluster */
    return -ERROR_FATFS_BADCLUSTER;
  else if(iCurClus == FATFS_FREECLUSTER) /* Free Cluster */
    return -ERROR_FATFS_FREECLUSTER;
  *iValue = iCurClus;
  return 0;
}

int fatfs32_getfatentry(struct TFat *fat, uint32 iEntry, 
      uint32 *iValue, uint32 *iActual)
{
  uint32 iCurClus;

  if(iEntry > fat->maxFatEntry)
  {
    fprintf(stderr,"ERR:fatfs32: fatentry[%ld] > maxFatEntry[%ld]\n",
      iEntry,fat->maxFatEntry);
    return -ERROR_INVALID;
  }
#ifdef FATFS_FAT_FULLYMAPPED 
  *iActual = ((uint32*)fat->FBuf)[iEntry];
#endif
#ifdef FATFS_FAT_PARTLYMAPPED
  {
  uint32 iMappedEntry;
  if(fatfs_partlymappedfat_handle(fat,iEntry,&iMappedEntry) != 0) 
    return -ERROR_FAILED;
  *iActual = ((uint32*)fat->FBuf)[iMappedEntry];
  }
#endif
  
  iCurClus = *iActual & 0x0FFFFFFF;
  if(iCurClus >= FAT32_EOF) /* EOF reached */
    return -ERROR_FATFS_EOF;
  else if(iCurClus == FAT32_BADCLUSTER) /* Bad cluster */
    return -ERROR_FATFS_BADCLUSTER;
  else if(iCurClus == FATFS_FREECLUSTER) /* Free Cluster */
    return -ERROR_FATFS_FREECLUSTER;
  *iValue = iCurClus;
  return 0;
}

int fatfs_getopticluslist_usefileinfo(struct TFat *fat, 
      struct TFileInfo *fInfo, struct TClusList *cl, int *clSize, 
      uint32 *prevClus)
{
  uint32 iCL = 0, iCurClus, iPrevClus;
  int iRet;

  if(*prevClus == 0)
  {
    cl[iCL].baseClus = fInfo->firstClus;
    iPrevClus = fInfo->firstClus;
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
    uint32 iActualEnt;
    if((iRet=fat->getfatentry(fat,iPrevClus,&iCurClus,&iActualEnt)) != 0)
    {
      if(iRet == -ERROR_FATFS_EOF) /* EOF reached */
      {
        *clSize = iCL+1; /* *clSize returns the count of entries */
        return 0;
      }
      else
      {
        *clSize = 0;
        return iRet;
      }
    }
#if (DEBUG_PRINT_FATFS > 15)
    else
       printf("fatfs:opticluslist: File[%s] nextClus[%ld]\n", 
         fInfo->name, iCurClus);
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

int fatfs_loadfileallsec_usefileinfo(struct TFat *fat, struct TFileInfo *fInfo,
      uint8 *buf, int32 bufLen, uint32 *totSecsRead)
{
  uint32 prevClus, noSecs, bytesRead;
  struct TClusList cl[16];
  int resOCL, resGS, clSize, iCur;
  
  if(bufLen < fInfo->fileSize)
  {
    fprintf(stderr,"ERR:fatfs:loadfile: insufficient buffer passed\n");
    return -ERROR_INSUFFICIENTRESOURCE;
  }
  *totSecsRead = 0;
  prevClus = 0;
  do
  {
    clSize = 16; 
    resOCL = fatfs_getopticluslist_usefileinfo(fat, fInfo, cl, &clSize, 
      &prevClus);
    if((resOCL != 0) && (resOCL != -ERROR_TRYAGAIN))
      break;
    for(iCur=0; iCur < clSize; iCur++)
    {
      noSecs = (cl[iCur].adjClusCnt+1)*fat->bs.secPerClus;
      *totSecsRead += noSecs;
      bytesRead = noSecs*fat->bs.bytPerSec;
      bufLen -= bytesRead;
      if(bufLen < 0)
      {
        if(resOCL != 0)
        {
          fprintf(stderr,"???:1:fatfs:loadfile: bufferspace less by >Clus\n");
          return -ERROR_INSUFFICIENTRESOURCE;
        }
        if(bufLen <= -(fat->bs.secPerClus*fat->bs.bytPerSec))
        {
          fprintf(stderr,"???:2:fatfs:loadfile: bufferspace less by >Clus\n");
          return -ERROR_INSUFFICIENTRESOURCE;
        }
        fprintf(stderr,"DEBUG:3:fatfs:loadfile: bufferspace less by <Clus\n");
        return -ERROR_INSUFFICIENTRESOURCE;
      }
      resGS=fat->bd->get_sectors(fat->bd,
        fat->baseSec+fatfs_firstsecofclus(fat,cl[iCur].baseClus),noSecs,buf);
      if(resGS != 0)
      {
        fprintf(stderr,"ERR:fatfs:loadfile: bd_get_sectors failed\n");
        return resGS;
      }
      buf += bytesRead; 
    }
  }while(resOCL != 0);
  return resOCL;
}

int fatfs_checkbuf_forloadfileclus(struct TFat *fat, uint32 bufLen)
{
  if(bufLen%FATFSCLUS_MAXSIZE != 0)
  {
    printf("ERROR:fatfs:loadfileclus: bufLen not multiple of [%d]\n", 
      FATFSCLUS_MAXSIZE);
    return -ERROR_INVALID;
  }
  if((bufLen%(fat->bs.secPerClus*fat->bs.bytPerSec)) != 0)
  {
    printf("ERROR:fatfs:loadclus: bufLen[%ld] notMultipleOf clusSize[%d*%d]\n",
      bufLen, fat->bs.secPerClus, fat->bs.bytPerSec);
    printf("DEBUG:fatfs:loadclus: also NOTINSYNC with FATFSCLUS_MAXSIZE[%d]\n",
      FATFSCLUS_MAXSIZE);
    return -ERROR_INVALID; 
  }
  return 0;
}

int fatfs_loadfileclus_usefileinfo(struct TFat *fat, struct TFileInfo *fInfo, 
      uint8 *buf, uint32 bufLen, uint32 *totalClusRead, uint32 *prevClus)
{
  uint32 noSecs, bytesRead;
  int32 totalPosClus2Read, noClus2Read;
  struct TClusList cl[16];
  int resOCL, resGS, clSize, iCur, bOutOfMemory;

#ifdef FATFS_FORCEARGCHECK
  if(fatfs_checkbuf_forloadfileclus(fat, bufLen) != 0) return -ERROR_INVALID;
#endif 
 
  bOutOfMemory = 0;
  *totalClusRead = 0;
  totalPosClus2Read = bufLen/(fat->bs.secPerClus*fat->bs.bytPerSec);
  do
  {
    clSize = 16; 
    resOCL = fatfs_getopticluslist_usefileinfo(fat, fInfo, cl, &clSize, 
      prevClus);
    if((resOCL != 0) && (resOCL != -ERROR_TRYAGAIN))
      break;
    bOutOfMemory = 0;
    for(iCur=0; ((iCur<clSize) && !bOutOfMemory); iCur++)
    {
      totalPosClus2Read -= (cl[iCur].adjClusCnt+1);
      if(totalPosClus2Read >= 0)
        noClus2Read = cl[iCur].adjClusCnt+1;
      else
      {
        noClus2Read = (cl[iCur].adjClusCnt+1)+totalPosClus2Read;
        *prevClus = cl[iCur].baseClus+noClus2Read;
        bOutOfMemory = 1;
      }
      *totalClusRead += noClus2Read;
      noSecs = noClus2Read*fat->bs.secPerClus;
      bytesRead = noSecs*fat->bs.bytPerSec;
      resGS=fat->bd->get_sectors(fat->bd,fat->baseSec+
        fatfs_firstsecofclus(fat,cl[iCur].baseClus),noSecs,buf);
      if(resGS != 0)
      {
        printf("DEBUG:fatfs:load: bd_get_sectors failed\n");
        return resGS;
      }
      buf += bytesRead; 
    }
  }while((resOCL!=0) && !bOutOfMemory);
  if((resOCL==-ERROR_TRYAGAIN) || bOutOfMemory)
    return -ERROR_TRYAGAIN;
  else if(resOCL != 0)
    return resOCL;
  return 0;
}

int fatuc_init(struct TFatFsUserContext *uc, struct TFat *fat)
{
  uc->fat = fat;
  uc->curDirBuf = fat->RDBuf;
  uc->curDirBufLen = fat->rdSize;
  uc->sCurDir[0] = 0;
  uc->sTempDir[0] = 0;
  fatuc_updatetempdirbufinfo(uc);
  return 0;
}

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

int util_strcpy(uint8 *src, uint8 *dest, uint32 destLen)
{
  int iCur;
  for(iCur=0; iCur < destLen; iCur++, src++)
  {
    dest[iCur] = *src;
    if(*src == (uint8)NULL)
      return 0;
  }
  dest[destLen-1] = 0;
  return -1;
}

int util_memcpy(uint8 *src, uint32 srcLen, uint8 *dest, uint32 destLen)
{
  int iLen, iCur, iRet;
  
  if(srcLen > destLen)
  {
    iLen = destLen;
    iRet = -1;
  }
  else
  {
    iLen = srcLen;
    iRet = 0;
  }
  for(iCur=0; iCur < iLen; iCur++)
    dest[iCur] = src[iCur];
  return iRet;
}


int fatuc_changedir(struct TFatFsUserContext *uc, char *fDirName, int bUpdateCurDir)
{
  uint16 tokenLen = (FATDIRENTRYLFN_SIZE+1);
  uint8 *dBuf;
  uint32 dBufLen;
  char *sDirName = fDirName, token[tokenLen];;

  if(sDirName[0] == FATFS_DIRSEP)
  {
    sDirName++;
    dBuf = uc->fat->RDBuf;
    dBufLen = uc->fat->rdSize;
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
    uint32 prevPos, res, totSecsRead;
    struct TFileInfo fInfo;

#if (DEBUG_PRINT_FATFS > 10)
    printf("fatfs:chdir:token [%s]\n", token);
#endif
    prevPos = 0;
    res = fatfs_getfileinfo_fromdir(token, dBuf, dBufLen, &fInfo, &prevPos);
    if(res != 0)
    {
      fprintf(stderr,"ERR:fatfs:chdir:subdir[%s] not found\n", token);
      return -ERROR_NOTFOUND;
    }
    if(fInfo.attr != FATATTR_DIR)
    {
      fprintf(stderr,"ERR:fatfs:chdir:NOT A directory [%s]\n", token);
      return -ERROR_INVALID;
    }
    /* special case of root dir got from a rootdirs_subdir\.. */
    if((fInfo.firstClus == 0) && (fInfo.fileSize == 0))
    {
      dBuf = uc->fat->RDBuf;
      dBufLen = uc->fat->rdSize;
    }
    else
    {
      res = fatfs_loadfileallsec_usefileinfo(uc->fat, &fInfo, uc->tempDirBuf, 
        FATFSUSERCONTEXTDIRBUF_MAXSIZE, &totSecsRead);
      dBuf = uc->tempDirBuf;
      dBufLen = totSecsRead*uc->fat->bs.bytPerSec;
      if(res !=  0)
      {
        fprintf(stderr,"ERR:fatfs:chdir:subdir[%s]dataload failed with[%ld]\n",
          fInfo.lfn, res);
        return res;
      }
    }
  }
  if(bUpdateCurDir)
  {
    uc->curDirBuf = dBuf;
    uc->curDirBufLen = dBufLen;
    util_strcpy(fDirName, uc->sCurDir, FATFSPATHNAME_MAXSIZE); 
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
#if (DEBUG_PRINT_FATFS > 5)
    printf("INFO:fatuc:getfileinfo: work with curDir Files for EFFICIENCY\n");
#endif
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

int fatfs_cleanup(struct TFat *fat)
{
  return 0;
}

