/*
 * fatfs.c - library for working with fat filesystem
 * v17Mar2005_2008
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
    fat->storerootdir = fatfs16_storerootdir;
    fat->getfatentry = fatfs16_getfatentry;
    fat->setfatentry = fatfs16_setfatentry;
    fat->checkfatbeginok = fatfs16_checkfatbeginok;
    fat->curFatNo = 0;
    fat->maxFatEntry = ((fat->bs.fatSz*fat->bs.bytPerSec)/2)-1;
  }
  else if(fat->bs.isFat32)
  {
    fat->loadrootdir = fatfs32_loadrootdir;
    fat->storerootdir = fatfs32_storerootdir;
    fat->getfatentry = fatfs32_getfatentry;
    fat->setfatentry = fatfs32_setfatentry;
    fat->checkfatbeginok = fatfs32_checkfatbeginok;
    fat->curFatNo = FAT32_EXTFLAGS_ACTIVEFAT(fat->bs.extFlags);
    fat->maxFatEntry = ((fat->bs.fatSz*fat->bs.bytPerSec)/4)-1;
  }
  else
  {
    fprintf(stderr,"ERR:fatfs: unsupported fat type\n");
    return -ERROR_NOTSUPPORTED;
  }
  
  fat->fatUpdated = 0;
  res = fatfs_loadfat(fat,fat->curFatNo,0);
  if(res != 0) return res;
  if((res=fat->checkfatbeginok(fat)) != 0) return res;
  res = fat->loadrootdir(fat);
  if(res != 0) return res;
  fat->freeCl.clSize = -1; fat->freeCl.fromClus = 0;
  fat->freeCl.curMinAdjClusCnt = fat->cntDataClus;
  /* Even if this fails its ok as its required only if WRITE is used 
   * And Write will handle it appropriatly */
  fatfs_update_freecluslist(fat);
  return 0;
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
  if(iTemp2 < FAT16_EOF)
  {
    fprintf(stderr,"ERR:fatfs:0th Entry invalid\n");
    return -ERROR_FATFS_INVALID0ENTRY;
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
  if(iTemp2 < FAT32_EOF)
  {
    fprintf(stderr,"ERR:fatfs:0th Entry invalid\n");
    return -ERROR_FATFS_INVALID0ENTRY;
  }
  return 0;
}

int fatfs_loadbootsector(struct TFat *fat)
{
  int res;
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
  fat->clusSize = fat->bs.bytPerSec*fat->bs.secPerClus;
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
  fat->cntDataClus = fat->bs.dataSec/fat->bs.secPerClus;
  fat->bs.isFat12 = 0; fat->bs.isFat16 = 0; fat->bs.isFat32 = 0;
  if(fat->cntDataClus < 4085)
  {
    fat->bs.isFat12 = 1;
    printf("**Fat12**\n");
  }
  else if(fat->cntDataClus < 65525)
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

int fatfs_storefat(struct TFat *fat)
{
  int iRet, nSecs;
  if(fat->fatUpdated == 1)
  {
    nSecs = (FATFAT_MAXSIZE/fat->bs.bytPerSec);
    if((fat->curFatPartStartSecAbs+nSecs-1) > fat->curFatEndSecAbs)
      nSecs = (fat->curFatEndSecAbs - fat->curFatPartStartSecAbs) + 1;
    iRet=fat->bd->put_sectors(fat->bd,fat->curFatPartStartSecAbs,nSecs,fat->FBuf);
    if(iRet != 0) return iRet;
    fat->fatUpdated = 0;
  }
  return 0;
}

int fatfs_loadfat(struct TFat *fat, int fatNo, int startEntry)
{
  int fatStartSec, nEntries, nSecs, startSec, iRet;

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
  iRet = fatfs_storefat(fat);
  if(iRet != 0) return iRet;
  iRet = fat->bd->get_sectors(fat->bd, startSec, nSecs,fat->FBuf); 
  if(iRet != 0) return iRet;
  fat->curFatPartStartSecAbs = startSec;
  fat->curFatEndSecAbs = fatStartSec+fat->bs.fatSz-1;
  return 0;
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
  fat->RDUpdated = 0;
  return fat->bd->get_sectors(fat->bd,fat->baseSec+firstRootDirSecNum, 
    rootDirSecs,fat->RDBuf); 
}

int fatfs16_storerootdir(struct TFat *fat)
{
  uint32 firstRootDirSecNum, rootDirSecs;
  int iRet;

  if(fat->RDUpdated != 0)
  {
    firstRootDirSecNum = fat->bs.rsvdSecCnt + (fat->bs.numFats*fat->bs.fatSz);
    rootDirSecs = fat->rdSize/fat->bs.bytPerSec;
    iRet = fat->bd->put_sectors(fat->bd,fat->baseSec+firstRootDirSecNum, 
             rootDirSecs,fat->RDBuf); 
    if(iRet != 0) return iRet;
    fat->RDUpdated = 0;
  }
  return 0;
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
  fat->RDUpdated = 0;
  if(res !=  0)
     fprintf(stderr,"ERR:fatfs32:rootdir:load failed with[%d]\n", res);
  return res;
}

int fatfs32_storerootdir(struct TFat *fat)
{
  int res;
  uint32 totClusRead, fromClus, lastClus;
  struct TFileInfo fInfo;
  
  if(fat->RDUpdated != 0)
  {
    fInfo.firstClus = fat->bs.rootClus;
    fInfo.fileSize = FATROOTDIR_MAXSIZE;
    fromClus = fInfo.firstClus;
    lastClus = 0;
    res = fatfs__storefileclus_usefileinfo(fat, &fInfo, fat->RDBuf, 
            fat->rdSize, &totClusRead, &lastClus, &fromClus);
    if(res !=  0)
    {
      fprintf(stderr,"ERR:fatfs32:rootdir:store failed with[%d]\n", res);
      return res;
    }
    fat->RDUpdated = 0;
  }
  return 0;
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
    if(cFile[sCur] == '.')
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

int fatfs_getfreedentry_indirbuf(uint8 *dirBuf, uint32 dirBufSize, 
      int totalContigEntries, uint8 **freeDEntry, uint32 *fromPos)
{
  uint8 *pCur, tName[FATFSNAME_SIZE];
  uint32 iCur,iContEnts;

  iCur = *fromPos;
  iContEnts = 0;
  while(iCur+FATDIRENTRY_SIZE  <= dirBufSize)
  {
    pCur = dirBuf+iCur;
    iCur+=FATDIRENTRY_SIZE;
    *fromPos = iCur;
    buffer_read_buffertostring(&pCur,FATDIRENTRYNAME_SIZE,tName,FATFSNAME_SIZE);
    if(tName[0] == 0) /* no more entries in dir */
    {
      *freeDEntry = dirBuf+iCur-FATDIRENTRY_SIZE;
      iCur += (totalContigEntries-1)*FATDIRENTRY_SIZE;
      if(iCur <= dirBufSize) return 0;
      return -ERROR_NOMORE;
    }
    else if(tName[0] == 0xe5) /* free dir entry */
    {
      if(iContEnts == 0)
        *freeDEntry = dirBuf+iCur-FATDIRENTRY_SIZE;
      iContEnts++;
      if(iContEnts >= totalContigEntries)
        return 0;
      continue;
    }
    else
      iContEnts = 0;
  }
  return -ERROR_NOMORE;
}

#define FATFS_FNAME_FINFO_83EXPANDED 999
int fatfs_getfileinfo_fromdir(char *cFile, uint8 *dirBuf, uint32 dirBufSize, 
  struct TFileInfo *fInfo, uint32 *prevPos, int useLFN)
{
  int iCur, iCurLFN, flagSpecialCurOrPrev;
  uint8 *pCur = dirBuf;
  int lfnPos = FATFSLFN_SIZE-1;
  char tcFile[FATFSNAME_SIZE],*fName;

  if(useLFN != FATFS_FNAME_FINFO_83EXPANDED)
  {
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
  }
  else
  {
    useLFN = 0;
    flagSpecialCurOrPrev = 999; /* To keep compiler happy */
    fName = cFile;
  }
  /****************/
  fInfo->fDEntryPos = -1;
  fInfo->lDEntryPos = -1;
  iCur = *prevPos;
  while(iCur+FATDIRENTRY_SIZE  <= dirBufSize)
  {
    pCur = dirBuf+iCur;
    iCur+=FATDIRENTRY_SIZE;
    *prevPos = iCur;
    buffer_read_buffertostring(&pCur,FATDIRENTRYNAME_SIZE,fInfo->name,FATFSNAME_SIZE);
    if(fInfo->name[0] == 0) /* no more entries in dir */
      return -ERROR_NOMORE;
    else if(fInfo->name[0] == 0xe5) /* free dir entry */
      continue;
    fInfo->attr = buffer_read_uint8_le(&pCur);
    if(fInfo->attr == FATATTR_LONGNAME)
    {
      if(lfnPos == FATFSLFN_SIZE-1)
        fInfo->fDEntryPos = iCur-FATDIRENTRY_SIZE;
      /*
      printf("INFO:fatfs:getfileinfo_fromdir long file name \n");
      */
      lfnPos-=FATDIRENTRYLFN_PARTIALCHARS; 
      pCur = dirBuf+iCur-FATDIRENTRY_SIZE;
      pCur++; /* skip ordinal */
      fInfo->lfn[lfnPos++] = buffer_read_uint16_le(&pCur);
      fInfo->lfn[lfnPos++] = buffer_read_uint16_le(&pCur);
      fInfo->lfn[lfnPos++] = buffer_read_uint16_le(&pCur);
      fInfo->lfn[lfnPos++] = buffer_read_uint16_le(&pCur);
      fInfo->lfn[lfnPos++] = buffer_read_uint16_le(&pCur);
      pCur+=3; /* skip attr, type and checksum */
      fInfo->lfn[lfnPos++] = buffer_read_uint16_le(&pCur);
      fInfo->lfn[lfnPos++] = buffer_read_uint16_le(&pCur);
      fInfo->lfn[lfnPos++] = buffer_read_uint16_le(&pCur);
      fInfo->lfn[lfnPos++] = buffer_read_uint16_le(&pCur);
      fInfo->lfn[lfnPos++] = buffer_read_uint16_le(&pCur);
      fInfo->lfn[lfnPos++] = buffer_read_uint16_le(&pCur);
      pCur+=2; /* cluster */
      fInfo->lfn[lfnPos++] = buffer_read_uint16_le(&pCur);
      fInfo->lfn[lfnPos++] = buffer_read_uint16_le(&pCur);
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
    {
      fInfo->fDEntryPos = iCur-FATDIRENTRY_SIZE;
      fInfo->lfn[0] = 0;
    }
    fInfo->lDEntryPos = iCur;
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
    fInfo->newFileSize = fInfo->fileSize;
    fInfo->updated = FINFO_UPDATED_NONE;
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
    fprintf(stderr,"ERR:fatfs16:get fatentry[%ld] > maxFatEntry[%ld]\n",
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

int fatfs16_setfatentry(struct TFat *fat, uint32 iEntry, uint32 iValue)
{
  uint16 iActValue;

  if(iValue == FATFS_EOF)
    iActValue = FAT16_EOF;
  else if(iValue == FATFS_BADCLUSTER)
    iActValue = FAT16_BADCLUSTER;
  else
    iActValue = iValue;
  if(iEntry > fat->maxFatEntry)
  {
    fprintf(stderr,"ERR:fatfs16:set fatentry[%ld] > maxFatEntry[%ld]\n",
      iEntry,fat->maxFatEntry);
    return -ERROR_INVALID;
  }
#ifdef FATFS_FAT_FULLYMAPPED 
  ((uint16*)fat->FBuf)[iEntry] = iActValue;
#endif
#ifdef FATFS_FAT_PARTLYMAPPED
  {
  uint32 iMappedEntry;
  if(fatfs_partlymappedfat_handle(fat,iEntry,&iMappedEntry) != 0) 
    return -ERROR_FAILED;
  ((uint16*)fat->FBuf)[iMappedEntry] = iActValue;
  }
#endif
  fat->fatUpdated = 1;
  return 0;
}

int fatfs32_getfatentry(struct TFat *fat, uint32 iEntry, 
      uint32 *iValue, uint32 *iActual)
{
  uint32 iCurClus;

  if(iEntry > fat->maxFatEntry)
  {
    fprintf(stderr,"ERR:fatfs32:get fatentry[%ld] > maxFatEntry[%ld]\n",
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

int fatfs32_setfatentry(struct TFat *fat, uint32 iEntry, uint32 iValue)
{
  uint32 iActValue;

  if(iValue == FATFS_EOF)
    iActValue = FAT32_EOF;
  else if(iValue == FATFS_BADCLUSTER)
    iActValue = FAT32_BADCLUSTER;
  else
    iActValue = iValue;
  if(iEntry > fat->maxFatEntry)
  {
    fprintf(stderr,"ERR:fatfs32:set fatentry[%ld] > maxFatEntry[%ld]\n",
      iEntry,fat->maxFatEntry);
    return -ERROR_INVALID;
  }
#ifdef FATFS_FAT_FULLYMAPPED 
  ((uint32*)fat->FBuf)[iEntry] = iActValue;
#endif
#ifdef FATFS_FAT_PARTLYMAPPED
  {
  uint32 iMappedEntry;
  if(fatfs_partlymappedfat_handle(fat,iEntry,&iMappedEntry) != 0) 
    return -ERROR_FAILED;
  ((uint32*)fat->FBuf)[iMappedEntry] = iActValue;
  }
#endif
  fat->fatUpdated = 1;
  return 0;
}

int fatfs__getoptifreecluslist(struct TFat *fat, struct TClusList *cl,
      int *clSize, int minAdjClusCnt, uint32 *fromClus)
{
  int iCL = -1;
  uint32 iCurClus, iPrevClus;
  int iRet;

#if (DEBUG_PRINT_FATFS > 25)
  pa_printstr("fatfs:optifreecluslist: cntDataClus [");
  pa_printints(fat->cntDataClus,"] maxFatEntry ["); 
  pa_printints(fat->maxFatEntry,"]\n");
#endif  
  if(*fromClus < 2)
    *fromClus = 2;
  iPrevClus=0;
  for(iCurClus=*fromClus;iCurClus<fat->cntDataClus;iCurClus++) /* FIXMAYBE:cntDataClus+2 */
  {
    uint32 iValue,iActualVal;
    iRet=fat->getfatentry(fat,iCurClus,&iValue,&iActualVal);
    if(iRet == -ERROR_FATFS_FREECLUSTER)
    {
#if (DEBUG_PRINT_FATFS > 15)
      printf("fatfs:optifreecluslist: iCurClus[%ld] iActualVal[%ld]\n", 
        iCurClus, iActualVal);
#endif
      if((iCurClus-iPrevClus) == 1)
      {
        cl[iCL].adjClusCnt++;
        iPrevClus = iCurClus;
      }
      else
      {
	if(cl[iCL].adjClusCnt >= minAdjClusCnt)
	{
          iCL++;
          if(iCL >= *clSize) 
          {
            /* we have used up all the cl so 
             * call once again with updated *fromClus
             */
            *fromClus = iCurClus;
            *clSize = iCL;
            return -ERROR_TRYAGAIN;
          }
	}
        cl[iCL].baseClus = iCurClus;
        cl[iCL].adjClusCnt = 0;
        iPrevClus = iCurClus;
      }
    }
  }
  *clSize = iCL+1; /* *clSize returns the count of entries */
  if(*clSize <= 0)
    return -ERROR_NOMORE;
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
	if(cl[iCL].baseClus != 0)
          *clSize = iCL+1; /* *clSize returns the count of entries */
	else
          *clSize = 0; /* or iCL */
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

int fatfs__freefilefatentries_usefileinfo(struct TFat *fat, 
      struct TFileInfo *fInfo, uint32 fromClus)
{
  uint32 iCurClus, iPrevClus;
  int iRet;

  if(fromClus == 0)
    iPrevClus = fInfo->firstClus;
  else
    iPrevClus = fromClus;
  while(1)
  {
    uint32 iActualEnt;
    if((iRet=fat->getfatentry(fat,iPrevClus,&iCurClus,&iActualEnt)) != 0)
    {
      if(iRet == -ERROR_FATFS_EOF) /* EOF reached */
      {
	if(iPrevClus != 0)
          return fat->setfatentry(fat,iPrevClus,FATFS_FREECLUSTER);
	return 0;
      }
      else
        return iRet;
    }
    iRet = fat->setfatentry(fat,iPrevClus,FATFS_FREECLUSTER);
    if(iRet != 0) return iRet;
    iPrevClus = iCurClus;
  }
  return -ERROR_UNKNOWN;
}

int fatfs__getlastclus_usefileinfo(struct TFat *fat, struct TFileInfo *fInfo,
      uint32 fromClus, uint32 *lastClus)
{
  uint32 iCurClus, iPrevClus;
  int iRet;

  if(fromClus == 0)
    iPrevClus = fInfo->firstClus;
  else
    iPrevClus = fromClus;
  *lastClus = 0;
  while(1)
  {
    uint32 iActualEnt;
    if((iRet=fat->getfatentry(fat,iPrevClus,&iCurClus,&iActualEnt)) != 0)
    {
      if(iRet == -ERROR_FATFS_EOF) /* EOF reached */
      {
        *lastClus = iPrevClus;
        return 0;
      }
      else
        return iRet;
    }
    iPrevClus = iCurClus;
  }
  return -ERROR_UNKNOWN;
}

#define FATFS__LINKNEWSEGMENT_USEFILEINFO_OPTI 1
int fatfs__linknewsegment_usefileinfo(struct TFat *fat, struct TFileInfo *fInfo,
      uint32 oldLastClus, uint32 newSegStartClus, uint32 newSegEndClus)
{
  uint32 iCur,actLastClus = oldLastClus;
  int iRet;

#ifdef FATFS__LINKNEWSEGMENT_USEFILEINFO_OPTI  
  if(oldLastClus == 0)
#endif	  
    if(fatfs__getlastclus_usefileinfo(fat,fInfo,oldLastClus,&actLastClus) != 0)
      return -ERROR_FAILED;
  if(actLastClus != 0)
  {
    if((iRet=fat->setfatentry(fat,actLastClus,newSegStartClus)) != 0) 
      return iRet;
  }
  else
  {
    fInfo->firstClus=newSegStartClus;	  
    fInfo->updated |= FINFO_UPDATED_FIRSTCLUS;
  }
  for(iCur=newSegStartClus;iCur<newSegEndClus;iCur++)
    if((iRet=fat->setfatentry(fat,iCur,iCur+1)) != 0) return iRet;
  if((iRet=fat->setfatentry(fat,newSegEndClus,FATFS_EOF)) != 0) return iRet;
  return 0;
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

int fatfs_update_freecluslist(struct TFat *fat)
{
  int resOCL;

  if((fat->freeCl.clSize == -1) && (fat->freeCl.fromClus == 0))
  {
    if(fat->freeCl.curMinAdjClusCnt == 0)
      return -ERROR_NOMORE;
    fat->freeCl.curMinAdjClusCnt >>= 3; /* divBy 8 */
  }
  fat->freeCl.clIndex = 0;
  fat->freeCl.clSize = FATFS_FREECLUSLIST_SIZE; 
  resOCL = fatfs__getoptifreecluslist(fat, fat->freeCl.cl, &fat->freeCl.clSize, fat->freeCl.curMinAdjClusCnt, &fat->freeCl.fromClus);
  if((resOCL != 0) && (resOCL != -ERROR_TRYAGAIN))
  {
    fat->freeCl.clSize = -1;
    if(resOCL == -ERROR_NOMORE)
    {
      fat->freeCl.fromClus = 0;
      resOCL = -ERROR_TRYAGAIN;
    }
  }
  return resOCL;
}

int fatfs_getopti_freecluslist(struct TFat *fat, struct TClusList *cl,
      int *clSize, int32 clusRequired, uint32 *fromClusHint)
{
  int oCur, res;
  int32 clusGot;
  
  oCur = 0;
  do
  {
    if(fat->freeCl.clSize == -1)
    {
      res = fatfs_update_freecluslist(fat);
      if((res != 0) && (res != -ERROR_TRYAGAIN))
        return res;
    }
    for(;(fat->freeCl.clIndex<fat->freeCl.clSize) && (clusRequired>0); oCur++)
    {
      if(oCur >= *clSize)
        return -ERROR_TRYAGAIN;
      cl[oCur].baseClus = fat->freeCl.cl[fat->freeCl.clIndex].baseClus;
      clusRequired -= (fat->freeCl.cl[fat->freeCl.clIndex].adjClusCnt+1);
      if(clusRequired >= 0)
      {
        clusGot = fat->freeCl.cl[fat->freeCl.clIndex].adjClusCnt+1;
	/* Note: Even thou current freeCl.cl content is used up 
	 * its not directly reflected by making its adjClusCnt 0 (or rather -1)
	 * cas clIndex is updated to skip this used up entry in cl */
        fat->freeCl.clIndex++;
      }
      else
      {
        clusGot = (fat->freeCl.cl[fat->freeCl.clIndex].adjClusCnt+1)+clusRequired;
	fat->freeCl.cl[fat->freeCl.clIndex].baseClus += clusGot;
	fat->freeCl.cl[fat->freeCl.clIndex].adjClusCnt -= clusGot;
      }
      cl[oCur].adjClusCnt = clusGot-1;
    }
    if(fat->freeCl.clIndex >= fat->freeCl.clSize)
      fat->freeCl.clSize = -1;
  }while(clusRequired > 0);
  *clSize = oCur;
  return 0;
}

#undef FATFS__STOREFILECLUS_SAFE  
#define FATFS_WRITENEW_MINADJCLUSCNT 0
#define FATFS__STOREFILECLUS_FROMCLUS_ALLOC FATFS_EOF

int fatfs__storefileclus_usefileinfo(struct TFat *fat, struct TFileInfo *fInfo, 
      uint8 *buf, uint32 bytesToWrite, uint32 *totalClusWriten, 
      uint32 *lastClusWriten, uint32 *fromClus)
{
  uint32 noSecs, bytesWriten, oldLastClus;
  int32 totalPosClus2Write, noClus2Write;
  struct TClusList cl[16];
  int resOCL, resGS, clSize, iCur, bAllWriten;

#ifdef FATFS_FORCEARGCHECK_FIXME
  if(fatfs_checkbuf_forstorefileclus(fat,buf,bufLen)!=0) return -ERROR_INVALID;
  if(*fromClus) 
    if(fatfs_checkclusispartoffile_usefileinfo(fat,fInfo,*fromClus)!=0)
      return -ERROR_INVALID;
#endif 
 
  bAllWriten = 0;
  *totalClusWriten = 0;
#ifdef FATFS__STOREFILECLUS_SAFE  
  *lastClusWriten = *fromClus;
  /* NOTE: as long as FROMCLUS_ALLOC is NOT DEFINED AS 0; 
   * which is the case, as keeping it EOF helped optimize logic better */
  if(*fromClus == FATFS__STOREFILECLUS_FROMCLUS_ALLOC) 
    *lastClusWriten = 0;
#endif  
  totalPosClus2Write = (bytesToWrite+fat->clusSize-1)/fat->clusSize;
  /* NOTE: if FROMCLUS_ALLOC is defined as 0, then remove this goto.
   * Also fatfs_getopticluslist_usefileinfo will have to be updated
   * such that if 0 is passed thro fromClus then it shouldn't try to 
   * go to the begining of file */
  if(*fromClus == FATFS__STOREFILECLUS_FROMCLUS_ALLOC)
    goto fatfs__storefileclus_fromclus_alloc;
  do
  {
    clSize = 16; 
    resOCL = fatfs_getopticluslist_usefileinfo(fat, fInfo, cl, &clSize, 
      fromClus);
    if((resOCL != 0) && (resOCL != -ERROR_TRYAGAIN))
      break;
    for(iCur=0; ((iCur<clSize) && !bAllWriten); iCur++)
    {
      totalPosClus2Write -= (cl[iCur].adjClusCnt+1);
      if(totalPosClus2Write >= 0)
        noClus2Write = cl[iCur].adjClusCnt+1;
      else
      {
        noClus2Write = (cl[iCur].adjClusCnt+1)+totalPosClus2Write;
        *fromClus = cl[iCur].baseClus+noClus2Write;
        bAllWriten = 1;
      }
      *totalClusWriten += noClus2Write;
      noSecs = noClus2Write*fat->bs.secPerClus;
      bytesWriten = noSecs*fat->bs.bytPerSec;
      resGS=fat->bd->put_sectors(fat->bd,fat->baseSec+
        fatfs_firstsecofclus(fat,cl[iCur].baseClus),noSecs,buf);
      if(resGS != 0)
      {
        printf("DEBUG:fatfs:load: bd_put_sectors failed\n");
        return resGS;
      }
      else
      {
        if(noClus2Write)
          *lastClusWriten = cl[iCur].baseClus+(noClus2Write-1);
      }
      buf += bytesWriten; 
    }
  }while((resOCL!=0) && !bAllWriten);
  if(bAllWriten)
    return 0;
  if(resOCL == -ERROR_TRYAGAIN)
    return -ERROR_UNKNOWN;
  if(resOCL != 0)
    return resOCL;
  /* eof reached for file as it exists now */
  fInfo->newFileSize = ((fInfo->newFileSize+fat->clusSize-1)/fat->clusSize)*fat->clusSize;
  if(fInfo->newFileSize < *totalClusWriten*fat->clusSize) /* Just in case */
    fInfo->newFileSize = *totalClusWriten*fat->clusSize;
  fInfo->updated |= FINFO_UPDATED_SIZE;
fatfs__storefileclus_fromclus_alloc:  
  // As getopti_freecluslist ignores fromClus hint passed to it, its OK 
  // Otherwise it may be better to assign this to lastClusWriten, if 
  // one wants files to grow towards the end of partition as far as possible
  *fromClus = 0; 
  do
  {
    clSize = 16; 
    resOCL = fatfs_getopti_freecluslist(fat, cl, &clSize, totalPosClus2Write, fromClus);
    if((resOCL != 0) && (resOCL != -ERROR_TRYAGAIN))
      break;
    for(iCur=0; ((iCur<clSize) && !bAllWriten); iCur++)
    {
      if(cl[iCur].adjClusCnt < FATFS_WRITENEW_MINADJCLUSCNT)
        continue;
      totalPosClus2Write -= (cl[iCur].adjClusCnt+1);
      if(totalPosClus2Write >= 0)
        noClus2Write = cl[iCur].adjClusCnt+1;
      else
      {
        noClus2Write = (cl[iCur].adjClusCnt+1)+totalPosClus2Write;
        bAllWriten = 1;
      }
      *totalClusWriten += noClus2Write;
      noSecs = noClus2Write*fat->bs.secPerClus;
      bytesWriten = noSecs*fat->bs.bytPerSec;
      resGS=fat->bd->put_sectors(fat->bd,fat->baseSec+
        fatfs_firstsecofclus(fat,cl[iCur].baseClus),noSecs,buf);
      if(resGS != 0)
      {
        printf("DEBUG:fatfs:load: bd_put_sectors failed\n");
        return resGS;
      }
      else
      {
        if(noClus2Write)
        {
          *fromClus = cl[iCur].baseClus+noClus2Write;
          oldLastClus = *lastClusWriten;
          *lastClusWriten = cl[iCur].baseClus+(noClus2Write-1);
          resGS=fatfs__linknewsegment_usefileinfo(fat,fInfo,oldLastClus,cl[iCur].baseClus,*lastClusWriten);
          if(resGS != 0) return resGS;
          fInfo->newFileSize = fInfo->newFileSize + noClus2Write*fat->clusSize;
          fInfo->updated |= FINFO_UPDATED_SIZE;
	}
	if(bAllWriten)
          *fromClus = FATFS__STOREFILECLUS_FROMCLUS_ALLOC;
      }
      buf += bytesWriten; 
    }
  }while((resOCL!=0) && !bAllWriten);
  if(bAllWriten)
    return 0;
  if(resOCL == -ERROR_TRYAGAIN)
    return -ERROR_UNKNOWN;
  if(resOCL != 0)
    return resOCL;
  if(totalPosClus2Write == 0)
  {
    *fromClus = FATFS__STOREFILECLUS_FROMCLUS_ALLOC;
    return 0;
  }
  return -ERROR_UNKNOWN;
}

int fatfs_updatefileinfo_indirbuf(char *dirBuf, uint32 dirBufSize, 
      struct TFileInfo *fInfo)
{
  uint32 prevPos=0;
  int iRet;
  uint8 *pCur, *pECur;
  struct TFileInfo gInfo;
  
  iRet=fatfs_getfileinfo_fromdir(fInfo->name,dirBuf,dirBufSize,&gInfo,
         &prevPos,FATFS_FNAME_FINFO_83EXPANDED);
  if(iRet != 0)
    return -ERROR_NOTFOUND;
  pECur=dirBuf+prevPos;
  if((fInfo->updated & FINFO_UPDATED_SIZE) 
        || (fInfo->updated & FINFO_UPDATED_ALL))
  {
    // MAYBE: Check and make a dir's filesize to zero 
    pCur = pECur-4; /* offset of filesize */
    buffer_write_uint32_le_noup(pCur, fInfo->newFileSize);
  }
  if((fInfo->updated & FINFO_UPDATED_FIRSTCLUS)
        || (fInfo->updated & FINFO_UPDATED_ALL))
  {
    pCur = pECur-6; /* offset of ls16bits of firstCluster */
    buffer_write_uint16_le_noup(pCur, fInfo->firstClus & 0xffff);
    pCur = pECur-12; /* offset of ms16bits of firstCluster */
    buffer_write_uint16_le_noup(pCur, (fInfo->firstClus & 0xffff0000)>>16);
  }
  if((fInfo->updated & FINFO_UPDATED_NAME)
        || (fInfo->updated & FINFO_UPDATED_ALL))
  {
    /* Updating of 83 Name not required as it is the 83 Name
     * which is used to find this particulart dirEntry 
     * in the first place. Also renaming of file is handled
     * as a seperate atomic function
     */
    pCur = pECur-FATDIRENTRY_SIZE;
    buffer_write_buffer_noup(pCur,fInfo->name,FATDIRENTRYNAME_SIZE);
    pa_printstrErr("fatfs:updatefileinfo:MAYBE FIXME: complete me\n");
  }
  if(fInfo->updated & FINFO_UPDATED_ALL)
  {
    pCur = pECur-FATDIRENTRY_SIZE;
    buffer_write_uint8_le_noup(pCur+11, fInfo->attr);
  }
  pCur = pECur-FATDIRENTRY_SIZE;
  buffer_write_uint16_le_noup(pCur+22, fInfo->wrtTime);
  buffer_write_uint16_le_noup(pCur+24, fInfo->wrtDate);
  return 0;
}

int fatfs__deletefileinfo_indirbuf(uint8 *dirBuf, uint32 dirBufSize,
      struct TFileInfo *fInfo)
{
  uint8 *pCur;
  pCur = dirBuf+fInfo->fDEntryPos;
  while(pCur <= (dirBuf+fInfo->lDEntryPos))
  {
    pCur[0] = 0xe5;
    pCur += FATDIRENTRY_SIZE;
  }
  return 0;
}

uint8 fatfs_calc_f83name_checksum(char *f83Name)
{
  uint8 chksum;
  int iCur;
  for(chksum=iCur=0;iCur<11;iCur++)
    chksum = (((chksum&1)<<7)|((chksum&0xfe)>>1)) + f83Name[iCur];
  return chksum;
}

int fatfs_cleanup(struct TFat *fat)
{
  int iRet;
  
  iRet = fat->storerootdir(fat);
  if(iRet != 0) return iRet;
  iRet = fatfs_storefat(fat);
  if(iRet != 0) return iRet;
  return 0;
}

/* Fat user context routines */

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

int fatuc__changedir(struct TFatFsUserContext *uc, char *fDirName, int bUpdateCurDir)
{
  uint16 tokenLen = (FATDIRENTRYLFN_SIZE+1);
  uint8 *dBuf;
  uint32 dBufLen;
  char *sDirName = fDirName, token[tokenLen];
  struct TFileInfo fInfo;

  if(sDirName[0] == FATFS_DIRSEP)
  {
    sDirName++;
    dBuf = uc->fat->RDBuf;
    dBufLen = uc->fat->rdSize;
    fInfo.firstClus = 0; fInfo.fileSize = 0;
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
    pa_strncpy(uc->sCurDir, fDirName, FATFSPATHNAME_MAXSIZE);
    fatuc_updatetempdirbufinfo(uc);
    pa_memcpy(&uc->curDirInfo,&fInfo,sizeof(fInfo));
  }
  else
  {
    uc->tempDirBuf = dBuf;
    uc->tempDirBufLen = dBufLen;
    pa_memcpy(&uc->tempDirInfo,&fInfo,sizeof(fInfo));
    //uc->tempDirSClus = fInfo.firstClus;
  }
  return 0;
}
 
int fatuc_chdir(struct TFatFsUserContext *uc, char *fDirName)
{
  return fatuc__changedir(uc, fDirName, 1);
}

#define FATUC__GETFILEINFO_STAGEPASSED_NONE 0
#define FATUC__GETFILEINFO_STAGEPASSED_DINFOCURDIR 1
#define FATUC__GETFILEINFO_STAGEPASSED_DINFOTEMPDIR 2
int fatuc__getfileinfo(struct TFatFsUserContext *uc, char *cFile,  
  struct TFileInfo *fInfo, struct TFileInfo *dInfo, uint32 *prevPos, 
  int *stagePassed)
{
  int res;

  *stagePassed = FATUC__GETFILEINFO_STAGEPASSED_NONE;
  res = util_extractfilefrompath(cFile, uc->pName, FATFSPATHNAME_MAXSIZE, 
    uc->fName, FATFSLFN_SIZE);
  if(res != 0)
  {
    printf("ERROR:fatuc:getfileinfo: extractfilefrompath failed\n");
    return res;
  }
  
  if( (uc->pName[0] == 0) 
        || ((uc->pName[0]=='.')&&(uc->pName[1]=='\\')) )
  {
    pa_memcpy(dInfo,&uc->curDirInfo,sizeof(struct TFileInfo));
    *stagePassed = FATUC__GETFILEINFO_STAGEPASSED_DINFOCURDIR;
    return fatfs_getfileinfo_fromdir(uc->fName, uc->curDirBuf, 
      uc->curDirBufLen, fInfo, prevPos, 0);
  }
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
    pa_memcpy(dInfo,&uc->tempDirInfo,sizeof(struct TFileInfo));
    *stagePassed = FATUC__GETFILEINFO_STAGEPASSED_DINFOTEMPDIR;
    return fatfs_getfileinfo_fromdir(uc->fName, uc->tempDirBuf, 
      uc->tempDirBufLen, fInfo, prevPos, 0);
  }
}

int fatuc_getfileinfo(struct TFatFsUserContext *uc, char *cFile,  
      struct TFileInfo *fInfo, uint32 *prevPos)
{
  struct TFileInfo dInfo;
  int stagePassed;
  return fatuc__getfileinfo(uc,cFile,fInfo,&dInfo,prevPos,&stagePassed);
}

int fatfs__allocatedentry_indirbuf(uint8 *dirBuf, uint32 dirBufLen, int totContEntries,
      struct TFileInfo *fInfoPart)
{
  uint32 prevPos=0;
  uint8 *freeDEntry, *pCur;
  int iRet, iCur;
  uint8 chksum;
  
  iRet = fatfs_getfreedentry_indirbuf(dirBuf, dirBufLen, totContEntries, &freeDEntry, &prevPos); 
  if(iRet != 0) return iRet;
  chksum = fatfs_calc_f83name_checksum(fInfoPart->name);
  pCur=freeDEntry;
  for(iCur=1;iCur<totContEntries;iCur++) /* so that updatefileinfo doesn't get a shock */
  {
    pa_memset(pCur, 0, FATDIRENTRY_SIZE);
#if 1
    buffer_write_uint8_le_noup(pCur, 0xe5);
#else
    {
    uint8 ordinal;
    ordinal = totContEntries-iCur;
    if(iCur == 1)
      ordinal |= 0x40;
    buffer_write_uint8_le_noup(pCur, ordinal); /* ordinal */
    buffer_write_uint8_le_noup(pCur+11, FATATTR_LONGNAME);
    buffer_write_uint8_le_noup(pCur+13, chksum);
    }
#endif
    pCur+=FATDIRENTRY_SIZE;
  }
  pa_memset(pCur, 0, FATDIRENTRY_SIZE);
  buffer_write_buffer_noup(pCur,fInfoPart->name,FATDIRENTRYNAME_SIZE);
  buffer_write_uint8_le_noup(pCur+11, fInfoPart->attr);
  return 0;
}

int fatuc__fopen(struct TFatFsUserContext *uc, char *cFile, int *fId,
      int flag, int totContDEntries)
{
  int iCur,iRet,stagePassed;
  uint8 *tDirBuf;
  uint32 prevPos=0, tDirBufLen;
  char tcFile[FATFSNAME_SIZE];
  struct TFileInfo *fInfo;

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
  iRet=fatuc__getfileinfo(uc,cFile,&uc->files[*fId].fInfo,
         &uc->files[*fId].dInfo,&prevPos,&stagePassed);
  if(iRet == 0) return iRet; 
  if((flag&FATUC_FOPEN_CREATE) == 0) goto error_cleanup;
  if(stagePassed == FATUC__GETFILEINFO_STAGEPASSED_NONE) goto error_cleanup;
  if(stagePassed == FATUC__GETFILEINFO_STAGEPASSED_DINFOCURDIR)
  {
    tDirBuf = uc->curDirBuf; tDirBufLen = uc->curDirBufLen;
  }
  else
  {
    tDirBuf = uc->tempDirBuf; tDirBufLen = uc->tempDirBufLen;
  }
  iRet = util_extractfilefrompath(cFile, uc->pName, FATFSPATHNAME_MAXSIZE, 
           uc->fName, FATFSLFN_SIZE);
  if(iRet != 0) goto error_cleanup;
  FN83_expand(uc->fName,tcFile);
  fInfo = &uc->files[*fId].fInfo; 
  /* firstClus, fileSize, lfn[*] = 0 */
  pa_memset(fInfo,0,sizeof(struct TFileInfo));
  fInfo->attr = FATATTR_ARCHIVE;
  buffer_write_buffer_noup(fInfo->name,tcFile,FATDIRENTRYNAME_SIZE);
  iRet = fatfs__allocatedentry_indirbuf(tDirBuf, tDirBufLen, totContDEntries, fInfo); 
  if(iRet != 0) goto error_cleanup;
  fInfo->updated |= FINFO_UPDATED_ALL; /* fileclose writes dentry to device */
  return 0;
error_cleanup:
  fatuc_fclose(uc,*fId);
  return iRet;
}

int fatuc_fopen(struct TFatFsUserContext *uc, char *cFile, int *fId,
      int flag)
{
  return fatuc__fopen(uc,cFile,fId,flag,FATUC_FOPEN_CREATE_DENTRIESPERFILE);
}
	
int fatuc_mkdir(struct TFatFsUserContext *uc, char *dirName, uint8* tBuf, int tBufLen)
{
  struct TFileInfo *fInfo, *dInfo;
  int iRet,fId, retVal=0;
  uint32 totalClusWriten, lastClus, fromClus;

  if(tBufLen < FATFSUSERCONTEXTDIRBUF_MAXSIZE) 
  {
    pa_printstr("ERR:fatuc_mkdir: pass buffer with atleast length [");
    pa_printints(FATFSUSERCONTEXTDIRBUF_MAXSIZE,"] bytes\n");
    return -ERROR_INVALID;
  }
  else
    tBufLen = FATFSUSERCONTEXTDIRBUF_MAXSIZE;
  if(((unsigned int)tBuf&0x3) != 0)
  {
    pa_printstr("ERR:fatuc_mkdir: passed buffer is not 32bit aligned\n");
    return -ERROR_INVALID;
  }

  iRet=fatuc_fopen(uc,dirName,&fId,FATUC_FOPEN_CREATE);
  if(iRet != 0) return iRet;
  fInfo = &uc->files[fId].fInfo;
  dInfo = &uc->files[fId].dInfo;
  /* Check if dir/file by this name already exists */
  if(fInfo->firstClus != 0)
  {
    retVal = -ERROR_INVALID;
    goto mkdir_cleanup;
  }
  fInfo->attr = FATATTR_DIR;
  
  pa_memset(tBuf,0,tBufLen);
  fromClus=lastClus=0;
  iRet = fatfs__storefileclus_usefileinfo(uc->fat,fInfo,tBuf,tBufLen,
           &totalClusWriten,&lastClus,&fromClus);
  if(iRet != 0)
  {
    retVal = iRet;
    goto mkdir_cleanup;
  }
 
  /* create . entry - curdir */
  buffer_write_buffer_noup(tBuf,".            ",11); /* have more spaces just to be safe */
  buffer_write_uint8_le_noup(tBuf+11,FATATTR_DIR);
  buffer_write_uint16_le_noup(tBuf+26, fInfo->firstClus & 0xffff);
  buffer_write_uint16_le_noup(tBuf+20, (fInfo->firstClus & 0xffff0000)>>16);
  /* create .. entry - parentdir */
  buffer_write_buffer_noup(tBuf+32,"..           ",11); /* have more spaces just to be safe */
  buffer_write_uint8_le_noup(tBuf+32+11,FATATTR_DIR);
  buffer_write_uint16_le_noup(tBuf+32+26, dInfo->firstClus & 0xffff);
  buffer_write_uint16_le_noup(tBuf+32+20, (dInfo->firstClus & 0xffff0000)>>16);

  fromClus=lastClus=0;
  iRet = fatfs__storefileclus_usefileinfo(uc->fat,fInfo,tBuf,tBufLen,
           &totalClusWriten,&lastClus,&fromClus);
  if(iRet != 0)
  {
    retVal = iRet;
    goto mkdir_cleanup;
  }
  fInfo->newFileSize = 0; // Cas this is a directory
mkdir_cleanup:
  fatuc_fclose(uc,fId);
  return retVal;
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

int fatuc__getdirbufoffile(struct TFatFsUserContext *uc, int fId,
      uint8 **dBuf, uint32 *dBufLen)
{
  struct TFatFile *f;

  f=&uc->files[fId];
  if(f->dInfo.firstClus == uc->curDirInfo.firstClus)
  {
    *dBuf = uc->curDirBuf; *dBufLen = uc->curDirBufLen;
  }
  else if(f->dInfo.firstClus == uc->tempDirInfo.firstClus)
  {
    *dBuf = uc->tempDirBuf; *dBufLen = uc->tempDirBufLen;
  }
  else if(f->dInfo.firstClus == 0)
  {
    *dBuf = uc->fat->RDBuf; *dBufLen = uc->fat->rdSize;
  }
  else
  {
    /*
     * FIXME: Have to add support for loading the directory data
     * using the firstClus info available for it
     * Till this support is added, if one has to write files see to it that 
     * the directory to which file has to be written should be the current 
     * directory. Thus when copying/moving files in the same filesystem
     * see that dest file's directory is the current directory and the 
     * source file could be from any directory.
     */
    return -ERROR_NOTSUPPORTED;
  }
  return 0;
}

int fatuc__savedirbuf(struct TFatFsUserContext *uc, struct TFileInfo *dInfo, 
      uint8 *dBuf, uint32 dBufLen)
{
  uint32 totalClusWriten, lastClus=0, fromClus=0;
  int iRet;

  if(dInfo->firstClus != 0)
  {
    iRet = fatfs__storefileclus_usefileinfo(uc->fat,dInfo,dBuf,dBufLen,
             &totalClusWriten,&lastClus,&fromClus);
    if(iRet != 0) return iRet;
  }
  else  /* rootdir, will be saved during unmount and not now */ 
    uc->fat->RDUpdated = 1;
  return 0;
}

int fatuc__syncfileinfo(struct TFatFsUserContext *uc, int fId)
{
  struct TFatFile *f;
  uint8 *dBuf;
  uint32 dBufLen;
  int iRet;
  int8 day, month, hr, min, sec;
  int32 year;

  f=&uc->files[fId];
  iRet = fatuc__getdirbufoffile(uc,fId,&dBuf,&dBufLen);
  if(iRet != 0) return iRet;
  pa_getdatetime(&year, &month, &day, &hr, &min, &sec);
  if(year >= 80) year -= 80; else year = 0;
  f->fInfo.wrtDate = ((day&0x1f)|((month&0xf)<<5)|((year&0x7f)<<9));
  f->fInfo.wrtTime = ((sec&0x1f)|((min&0x3f)<<5)|((hr&0x1f)<<11));
  iRet = fatfs_updatefileinfo_indirbuf(dBuf, dBufLen, &f->fInfo);
  if(iRet != 0) return iRet;
  iRet = fatuc__savedirbuf(uc,&f->dInfo,dBuf,dBufLen);
  if(iRet != 0) return iRet;
  f->fInfo.fileSize = f->fInfo.newFileSize;
  f->fInfo.updated = FINFO_UPDATED_NONE;
  return 0;
}

int fatuc__remove_dentry(struct TFatFsUserContext *uc, int fId)
{
  struct TFatFile *f;
  uint8 *dBuf;
  uint32 dBufLen;
  int iRet;

  f=&uc->files[fId];
  iRet = fatuc__getdirbufoffile(uc,fId,&dBuf,&dBufLen);
  if(iRet != 0) return iRet;
  iRet = fatfs__deletefileinfo_indirbuf(dBuf, dBufLen, &f->fInfo);
  if(iRet != 0) return iRet;
  iRet = fatuc__savedirbuf(uc,&f->dInfo,dBuf,dBufLen);
  if(iRet != 0) return iRet;
  f->fInfo.updated = FINFO_UPDATED_NONE;
  return 0;
}

int fatuc__deletefile(struct TFatFsUserContext *uc, int fId)
{
  struct TFatFile *f;
  int iRet;

  f=&uc->files[fId];
  iRet = fatfs__freefilefatentries_usefileinfo(uc->fat, &f->fInfo, 0);
  if(iRet != 0) return iRet;
  return fatuc__remove_dentry(uc,fId);
}

int util_prep8d3_fromLFN(char *s8d3Name, char16 *s16LFN, 
      int s8d3Len, int *tildePos)
{
  int iCur,iTemp;
  
  *tildePos = 0;
  pa_strc16Tostr_len(s8d3Name,s16LFN,s8d3Len);
  pa_toupper(s8d3Name);
  for(iCur=0;iCur<9;iCur++)
  {
    if(s8d3Name[iCur] == (char)NULL)
      return 0;
    if(s8d3Name[iCur] == '.')
      return 0;
  }
  *tildePos = 6;
  s8d3Name[6] = '~'; s8d3Name[7] = '1';
  for(iCur=0;iCur<255;iCur++)
  {
    if((s16LFN[iCur]==(char16)NULL) || (s16LFN[iCur]==(char16)'.'))
      break;
  }
  if((iCur==255) || (s16LFN[iCur]==(char16)NULL))
  {
    s8d3Name[8] = (char)NULL;
    return 0;
  }
  s8d3Name[8] = '.';
  for(iTemp=9,iCur++;(iTemp<12)&&(iCur<255);iCur++,iTemp++)
    s8d3Name[iTemp] = (char)(s16LFN[iCur]&0xff);
  pa_toupper(s8d3Name);
  return 0;
}

#define FATFS8d3NAME_SIZE (FATFSNAME_SIZE+1)

int fatuc_move_dentry(struct TFatFsUserContext *uc, char *src, 
      char* destPath, char16* u16FName, char* tBuf, int tBufLen)
{
  int totContDEntries = 0;
  struct TFatFile *sF, *dF;
  int iRet, fSId=-1, fDId=-1, destMaxLen, iPos;
  int tildePos, iNameCnt;
  char s83FName[FATFS8d3NAME_SIZE];

  iRet = fatuc_fopen(uc,src,&fSId,0);
  if(iRet != 0) return iRet;
  sF=&uc->files[fSId];
  if(u16FName == NULL)
    totContDEntries = ((sF->fInfo.lDEntryPos - sF->fInfo.fDEntryPos)/FATDIRENTRY_SIZE)+1;
  else
  {
    totContDEntries = (pa_strnlen_c16(u16FName,FATDIRENTRYLFN_SIZE)/FATDIRENTRYLFN_PARTIALCHARS) + 1;
    util_prep8d3_fromLFN(s83FName,u16FName,FATFS8d3NAME_SIZE,&tildePos);
  }
  iRet = fatuc__remove_dentry(uc,fSId);
  if(iRet != 0) goto error_cleanup;

  if(tBufLen < FATFSPATHNAME_MAXSIZE)
  {
    pa_printstr("ERR:fatuc_move_dentry: tempBuf should be atleast [");
    pa_printints(FATFSPATHNAME_MAXSIZE,"] bytes\n");
    iRet = -ERROR_INSUFFICIENTRESOURCE;
    goto error_cleanup;
  }
  if(u16FName == NULL)
  {
    iRet = util_extractfilefrompath(src, tBuf, FATFSPATHNAME_MAXSIZE, 
             s83FName, FATFS8d3NAME_SIZE);
    if(iRet != 0) goto error_cleanup;
  }
  destMaxLen = FATFSPATHNAME_MAXSIZE-FATFSLFN_SIZE;
  iRet = pa_strncpyEx(tBuf,destPath,destMaxLen,&iPos);
  if(iRet != 0) goto error_cleanup;
  if(tBuf[iPos-1] != FATFS_DIRSEP)
  {
    tBuf[iPos] = FATFS_DIRSEP;
    iPos++;
    tBuf[iPos] = (char)NULL;
  }
  if(tildePos != 0)
    tildePos += iPos;
  iRet = pa_strncpy(&tBuf[iPos],s83FName,FATFSLFN_SIZE);
  if(iRet != 0) goto error_cleanup;
  iNameCnt=1;
NextName:
  iRet = fatuc__fopen(uc,tBuf,&fDId,FATUC_FOPEN_CREATE,totContDEntries);
  if(iRet != 0) goto error_cleanup;
  dF=&uc->files[fDId];
  if(dF->fInfo.firstClus != 0)
  {
    if((iNameCnt>9) || (tildePos==0))
    {
    pa_printstr("ERR:fatuc_move_dentry: destPath already contains the file\n");
    iRet = -ERROR_INVALID;
    goto error_cleanup;
    }
    fatuc_fclose(uc,fDId); fDId = -1;
    iNameCnt++;
    tBuf[tildePos+1] = (char)((int)'0'+iNameCnt);
    goto NextName;
  }
  dF->fInfo.attr = sF->fInfo.attr;
  dF->fInfo.firstClus = sF->fInfo.firstClus;
  dF->fInfo.newFileSize = sF->fInfo.fileSize;
  /* FIXME: LFN name to be handled */
  
error_cleanup:
  if(fSId >= 0)
    fatuc_fclose(uc,fSId);
  if(fDId >= 0)
    fatuc_fclose(uc,fDId);
  return iRet;
}

int fatuc_fclose(struct TFatFsUserContext *uc, int fId)
{
  if(uc->files[fId].state == FATUC_USED)
  {
    if(uc->files[fId].fInfo.updated)
    {
      pa_printstr("INFO: File has been updated [o:n]=[");
      pa_printints(uc->files[fId].fInfo.fileSize,":");
      pa_printints(uc->files[fId].fInfo.newFileSize,"], updating its dir info\n");
      if(fatuc__syncfileinfo(uc,fId) != 0)
        pa_printstrErr("fatfs:uc:fclose syncfileinfo failed\n");
    }
    uc->files[fId].state = FATUC_FREE;
    uc->files[fId].fPos = 0;
    uc->files[fId].fromClus = -1;
    return 0;
  }
  return -ERROR_INVALID;
}

