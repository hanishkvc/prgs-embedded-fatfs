/*
 * fatfs.c - library for working with fat filesystem
 * v27Jan2005_1516
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
  pCur = fat->BBuf;
  tVerify=buffer_read_uint8_le(&pCur);
  if((tVerify!=FATFS_BS_STARTBYTE_T0) && (tVerify!=FATFS_BS_STARTBYTE_T1))
  {
    fprintf(stderr,"ERR:fatfs:bootsector: BootSec byte not found at byte0\n");
    return -ERROR_FATFS_NOTBOOTSEC;
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

int fatfs_FN83_expandifspecial_b(char *cFile, char *tcFile)
{
  int flagSpecialCurOrPrev;
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
  return flagSpecialCurOrPrev;
}

int fatfs_LFN_expandifspecial_b(char16 *cFile, char *tcFile)
{
  int flagSpecialCurOrPrev;
  flagSpecialCurOrPrev = 0;
  if(cFile[0] == '.')
  {
    flagSpecialCurOrPrev = 1;
    if(cFile[1] == '.')
    {
      if(cFile[2] == (char16)NULL)
        strcpy(tcFile,"..         ");
      else
        flagSpecialCurOrPrev = 0;
    }
    else if(cFile[1] == (char16)NULL)
      strcpy(tcFile,".          ");
    else
      flagSpecialCurOrPrev = 0;
  }
  return flagSpecialCurOrPrev;
}

int FN83_expand(char *cFile, char *tcFile)
{
  int dCur,sCur,stringOver;
  
  stringOver=0;
  for(sCur=0;sCur<8;sCur++)
  {
    tcFile[sCur] = cFile[sCur];
    if(cFile[sCur] == '.')
      break;
    else if(cFile[sCur] == (char)NULL)
    {
      stringOver=1;
      break;
    }
  }
  for(dCur=sCur;dCur<8;dCur++)
    tcFile[dCur] = ' ';

  if(stringOver == 0)
  {
    if((cFile[sCur] != '.') && (cFile[sCur] != (char)NULL))
      return -ERROR_INVALID;
    sCur++;
    for(;dCur<11;dCur++,sCur++)
    {
      tcFile[dCur] = cFile[sCur];
      if(cFile[sCur] == (char)NULL)
        break;
    }
  }
  for(;dCur<11;dCur++)
    tcFile[dCur] = ' ';
  tcFile[dCur] = (char)NULL;
  return 0;
}

int fatfs_getfileinfo_fromdir(char *cFile, uint8 *dirBuf, uint16 dirBufSize, 
  struct TFileInfo *fInfo, uint32 *prevPos, int useLFN)
{
  int iCur, iCurLFN, flagSpecialCurOrPrev;
  uint8 *pCur = dirBuf;
  int lfnPos = FATFSLFN_SIZE-1;
  char tcFile[FATDIRENTRYNAME_SIZE+1],*fName;

  /* logic added as . and .. directory entries don't have corresponding
   * lfn entry in the directory
   */
  if(useLFN)
    flagSpecialCurOrPrev = fatfs_LFN_expandifspecial_b((char16*)cFile,tcFile);
  else
    flagSpecialCurOrPrev = fatfs_FN83_expandifspecial_b(cFile,tcFile);
  if(flagSpecialCurOrPrev == 0)
  {
    if(useLFN == 0)
    {
      FN83_expand(cFile,tcFile);
      fName = tcFile;
    }
    else
      fName = cFile;
  }
  else
    fName = tcFile;
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
      fInfo->name, (char*)fInfo->lfn, fInfo->attr, fInfo->firstClus, fInfo->fileSize);
#endif
    if((useLFN == 0) || (flagSpecialCurOrPrev == 1))
    {
      if(pa_strncmp(fInfo->name,fName,FATFSNAME_SIZE) == 0)
        return 0;
    }
    else
    {
      if(pa_strncmp_c16(fInfo->lfn,(char16*)fName,FATFSLFN_SIZE) == 0)
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
      uint32 *fromClus)
{
  uint32 iCL = 0, iCurClus, iPrevClus;
  int iRet;

  if(*fromClus == 0)
  {
    cl[iCL].baseClus = fInfo->firstClus;
    iPrevClus = fInfo->firstClus;
    cl[iCL].adjClusCnt = 0;
  }
  else
  {
    cl[iCL].baseClus = *fromClus;
    iPrevClus = *fromClus;
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
         * call once again with updated *fromClus
         */
        *fromClus = iCurClus;
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
  uint32 fromClus, noSecs, bytesRead;
  struct TClusList cl[16];
  int resOCL, resGS, clSize, iCur;
  
  if(bufLen < fInfo->fileSize)
  {
    fprintf(stderr,"ERR:fatfs:loadfile: insufficient buffer passed\n");
    return -ERROR_INSUFFICIENTRESOURCE;
  }
  *totSecsRead = 0;
  fromClus = 0;
  do
  {
    clSize = 16; 
    resOCL = fatfs_getopticluslist_usefileinfo(fat, fInfo, cl, &clSize, 
      &fromClus);
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

int fatfs__loadfileclus_usefileinfo(struct TFat *fat, struct TFileInfo *fInfo, 
      uint8 *buf, uint32 bytesToRead, uint32 *totalClusRead, 
      uint32 *lastClusRead, uint32 *fromClus)
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
  *lastClusRead = *fromClus;
  totalPosClus2Read = bytesToRead/(fat->bs.secPerClus*fat->bs.bytPerSec);
  do
  {
    clSize = 16; 
    resOCL = fatfs_getopticluslist_usefileinfo(fat, fInfo, cl, &clSize, 
      fromClus);
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
        *fromClus = cl[iCur].baseClus+noClus2Read;
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
      else
      {
        if(noClus2Read)
          *lastClusRead = cl[iCur].baseClus+(noClus2Read-1);
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

/* * bufLen specifies the length of the buffer as well as the number of 
 *   bytes to read and it must be a multiple of clustersize in bytes 
 *   *As bufLen is bufLen as well as bytesToRead no need to check if the 
 *    passed buf can hold the specified number of bytes 
 * * Also this function reads begining from a cluster boundery
 *   (fromClus - the cluster to start reading) till a whole number 
 *   of clusters i.e clusters is the granularity of this function.
 */
int fatfs_loadfileclus_usefileinfo(struct TFat *fat, struct TFileInfo *fInfo, 
      uint8 *buf, uint32 bufLen, uint32 *totalClusRead, uint32 *fromClus)
{
  uint32 lastClusRead;
  return fatfs__loadfileclus_usefileinfo(fat, fInfo, buf, bufLen, totalClusRead,
           &lastClusRead, fromClus);
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


int fatuc__changedir(struct TFatFsUserContext *uc, char *fDirName, int bUpdateCurDir)
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
    res = fatfs_getfileinfo_fromdir(token, dBuf, dBufLen, &fInfo, &prevPos, 0);
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
        fprintf(stderr,"ERR:fatfs:chdir:subdir[%s]dataload failed [%ld]\n",
          fInfo.name, res);
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
  return fatuc__changedir(uc, fDirName, 1);
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
      uc->curDirBufLen, fInfo, prevPos, 0);
  else
  {
#if (DEBUG_PRINT_FATFS > 5)
    printf("INFO:fatuc:getfileinfo: work with curDir Files for EFFICIENCY\n");
#endif
    res = fatuc__changedir(uc, uc->pName, 0);
    if(res != 0)
    {
      printf("ERROR:fatuc:getfileinfo: changedir failed\n");
      return res;
    }
    return fatfs_getfileinfo_fromdir(uc->fName, uc->tempDirBuf, 
      uc->tempDirBufLen, fInfo, prevPos, 0);
  }
}

int fatfs_cleanup(struct TFat *fat)
{
  return 0;
}

int fatuc_fopen(struct TFatFsUserContext *uc, char *cFile, int *fId)
{
  int iCur;
  uint32 prevPos=0;

  *fId = -1;
  for(iCur=0;iCur<FATFSUSERCONTEXT_NUMFILES;iCur++)
  {
    if(uc->files[iCur].state != FATUC_USED)
    {
      *fId = iCur;
      uc->files[*fId].state = FATUC_USED;
      uc->files[*fId].fPos = 0;
      uc->files[*fId].fromClus = 0;
      uc->files[*fId].offInFromClus = 0;
      break;
    }
  }
  if(*fId == -1)
    return -ERROR_INSUFFICIENTHANDLES;
  return fatuc_getfileinfo(uc,cFile,&uc->files[*fId].fInfo,&prevPos);
}

#define FATUC_TEMP_CLSIZE 64
/*
 * FIXME: In worst case if a file is spread such that its FAT Entries are
 * spread across FATTable parts in a FAT_PARTLYMAPPED situation, this could lead
 * to the required FATTable parts getting loaded many times.
 *
 * This could be avoided by keeping the OptiClusList of a file as part of the
 * TFatFile struct. Thus when the file is opened, we could read the OptiClusList
 * once into memory and use this for subsequent accesses. However here as there
 * is a limit on the size of the OptiClusList kept in TFatFile, if some files
 * are fragmented badly to have more fragments than the number of entries in this
 * OptiClusList they cann't be worked on (may be the part of it for which info
 * is loaded will be supported but not beyond that).
 *
 */
int fatuc__fposChangedUpdate(struct TFatFsUserContext *uc, int fId, int oldFPos)
{
  int clSize,ret,clusSize,iCur;
  struct TClusList cl[FATUC_TEMP_CLSIZE];
  int newClusNo, curClusNo, diffClus, startActClus;

#if 0
  pa_printstr("DEBUG:fatfs:fatuc__fposChangedUpdate: called\n");
#endif
  clusSize = uc->fat->bs.bytPerSec * uc->fat->bs.secPerClus;
  newClusNo = (uc->files[fId].fPos/clusSize)+1; /* note starts with 1 */
  uc->files[fId].offInFromClus = uc->files[fId].fPos%clusSize;
  curClusNo = (oldFPos/clusSize)+1;
  if(newClusNo == curClusNo)
    return 0;
  /* start from begining if fPos has gone backwards 
   * cas FATTable is single linked
   */
  if(newClusNo < curClusNo)
  {
    curClusNo = 0;
    uc->files[fId].fromClus = 0;
  }
  else
    curClusNo--;
  startActClus = 0;
  do{
    clSize = FATUC_TEMP_CLSIZE;
    ret=fatfs_getopticluslist_usefileinfo(uc->fat, &uc->files[fId].fInfo,
       cl, &clSize, &uc->files[fId].fromClus);
    if((ret != 0) && (ret != -ERROR_TRYAGAIN))
      return ret;
    for(iCur=0;iCur<clSize;iCur++)
    {
       curClusNo += cl[iCur].adjClusCnt+1;
       diffClus = curClusNo - newClusNo;
       if(diffClus >= 0)
       {
         startActClus = (cl[iCur].baseClus + cl[iCur].adjClusCnt) - diffClus;
         break;
       }
    }
  }while((ret==-ERROR_TRYAGAIN) && (startActClus==0));
  if(startActClus == 0)
    return -ERROR_NOMORE; /* Mostly return EOF reached */
  uc->files[fId].fromClus = startActClus;
  return 0;
}

int fatuc_fseek(struct TFatFsUserContext *uc, int fId,
      int32 offset, int whence)
{
  int retVal = -ERROR_INVALID;
  uint32 prevFPos;

  prevFPos = uc->files[fId].fPos;
  if(uc->files[fId].state != FATUC_USED)
    return -ERROR_INVALID;
  if(whence == FATUC_SEEKSET)
  {
    uc->files[fId].fPos = offset;
    retVal = 0;
  }
  if(whence == FATUC_SEEKCUR)
  {
    uc->files[fId].fPos += offset;
    retVal = 0;
  }
  if(whence == FATUC_SEEKEND)
  {
    uc->files[fId].fPos = uc->files[fId].fInfo.fileSize + offset;
    retVal = 0;
  }
  if(retVal == 0)
    return fatuc__fposChangedUpdate(uc, fId, prevFPos);
  return retVal;
}

/*
 * bufLen requires to be large enough to accomodate bytesToRead
 * plus 2 clusSize (rather FATFSCLUS_MAXSIZE). Because if fPos
 * is unaligned to cluster boundry then additional space is required
 * at the begining and If fPos+bytesToRead is unaligned to cluster
 * boundry then additional space is required at the end. So to be
 * safe in all conditions its better to have additional space of
 * 2*FATFSCLUS_MAXSIZE.
 * *This restriction is there because the passed buf is used directly
 *  by the lower block level driver to fill data using DMA/xxxx available 
 *  and no intermediate buffers are used anywhere.
 * However if access will be made such that things are always aligned
 * then there is no need for this additional space
 */
int fatuc_fread(struct TFatFsUserContext *uc, int fId,
      uint8 *buf, uint32 bufLen, uint32 bytesToRead, 
      uint8 **atBuf, uint32 *bytesRead)
{
  int ret,clusSize, bytAl;
  uint32 totalClusRead, lastClusRead;

  if(uc->files[fId].state != FATUC_USED)
    return -ERROR_INVALID;
  if(uc->files[fId].fPos >= uc->files[fId].fInfo.fileSize)
    return -ERROR_FATFS_EOF;
  clusSize = uc->fat->bs.bytPerSec * uc->fat->bs.secPerClus;

  bytAl = ((bytesToRead-1+uc->files[fId].offInFromClus+clusSize)/clusSize)*clusSize;
  if(bytAl > bufLen)
    return -ERROR_INSUFFICIENTRESOURCE;
  ret=fatfs__loadfileclus_usefileinfo(uc->fat, &uc->files[fId].fInfo, 
      buf, bytAl, &totalClusRead, &lastClusRead, &uc->files[fId].fromClus);
  if((ret!=0)&&(ret!=-ERROR_TRYAGAIN))
    return ret;
  *atBuf = buf + uc->files[fId].offInFromClus;
  *bytesRead = totalClusRead*clusSize-uc->files[fId].offInFromClus;
  if(*bytesRead < bytesToRead)
  {
    /* EOF reached */
    if((uc->files[fId].fPos+*bytesRead) >= (uc->files[fId].fInfo.fileSize))
    {
      *bytesRead = uc->files[fId].fInfo.fileSize - uc->files[fId].fPos;
      uc->files[fId].fPos += *bytesRead; 
      return 0;
    }
    pa_printstrErr("???:fatfs:fatuc_read: bytesRead<bytesToRead CANT OCCUR\n");
    return -ERROR_UNKNOWN;
  }
  *bytesRead = bytesToRead;
  uc->files[fId].fPos += bytesToRead;
  if((uc->files[fId].fPos%clusSize) != 0)
  {
    /* As only part of the lastClusRead above is used, 
     * the remaining part requires to be read again next time, so forcing
     */
    uc->files[fId].fromClus = lastClusRead;
    uc->files[fId].offInFromClus = uc->files[fId].fPos%clusSize;
  }
  return 0;
}

int fatuc_fclose(struct TFatFsUserContext *uc, int fId)
{
  if(uc->files[fId].state == FATUC_USED)
  {
    uc->files[fId].state = FATUC_FREE;
    uc->files[fId].fPos = 0;
    uc->files[fId].fromClus = -1;
    return 0;
  }
  return -ERROR_INVALID;
}

