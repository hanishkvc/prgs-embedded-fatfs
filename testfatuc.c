
//#define TESTFATUC_BUFSIZE_SUB 0
#define TESTFATUC_BUFSIZE_SUB (FATFSCLUS_MAXSIZE*2+11)

int testfatuc_fileextract2posix(struct TFatFsUserContext *uc, 
      char *sFile, char *dFile)
{
  FILE *fDest;
  uint32 dataRead;
  int res, ret, resFW, fId;
  uint8 *nBuf;

  printf("testfatuc:INFO: fileextract2posix [%s] to [%s]\n",sFile,dFile);
  if(strncmp(dFile,"STDOUT",6) != 0)
  {
    fDest = fopen(dFile, "w");
    if(fDest == NULL)
    {
      perror("testfatuc:ERROR: opening dest file");
      return -1;
    }
  }
  else
    fDest = stdout;
  if(fatuc_fopen(uc, sFile, &fId) != 0)
  {
    printf("testfatuc:ERROR:fileextract: opening src [%s] file\n", sFile);
    return -1;
  }
  ret = -1;
  fatfs_checkbuf_forloadfileclus(uc->fat, gDataBufSize);
  while(1)
  {
    res=fatuc_fread(uc, fId, gDataBuf, gDataBufSize,
      gDataBufSize-TESTFATUC_BUFSIZE_SUB,
      &nBuf, &dataRead);
    if(res != 0)
      break;
#if DEBUG_TESTFAT > 15    
    else
      printf("testfatuc:INFO: fatuc_fread dataRead[%ld] fPos[%ld]\n", 
        dataRead, uc->files[fId].fPos);
#endif    
    resFW=fwrite(nBuf, 1, dataRead, fDest);
    if(resFW != dataRead)
    {
      perror("testfatuc:ERROR: less bytes written\n");
      break;
    }
  }
  fclose(fDest);
  fatuc_fclose(uc, fId);
  if(res == -ERROR_FATFS_EOF)
    ret = 0;
  return ret;
}

int testfatuc_fileextract2fatfs(struct TFatFsUserContext *uc, 
      char *sFile, char *dFile)
{
  uint32 dataRead, lastClus, fromClus=0, totalClusWriten;
  int res, fId, fDId;
  uint8 *nBuf;

  printf("testfatuc:INFO: fileextract2fatfs [%s] to [%s]\n",sFile,dFile);
  if(fatuc_fopen(uc, sFile, &fId) != 0)
  {
    printf("testfatuc:ERROR:fileextract: opening src [%s] file\n", sFile);
    return -1;
  }
  if(fatuc_fopen(uc,dFile, &fDId) != 0)
  {
    printf("testfatuc:ERROR:fileextract: opening dest [%s] file\n", dFile);
    return -1;
  }
  fatfs_checkbuf_forloadfileclus(uc->fat, gDataBufSize);
  while(1)
  {
    res=fatuc_fread(uc, fId, gDataBuf, gDataBufSize,
      gDataBufSize,
      //gDataBufSize-TESTFATUC_BUFSIZE_SUB,
      &nBuf, &dataRead);
    if(res == -ERROR_FATFS_EOF)
      break;
    if(res != 0)
    {
      printf("testfatuc:ERROR:fileextract: reading source\n");
      break;
    }
#if DEBUG_TESTFAT > 15    
    else
      printf("testfatuc:INFO: fatuc_fread dataRead[%ld] fPos[%ld]\n", 
        dataRead, uc->files[fId].fPos);
#endif    
    res=fatfs__storefileclus_usefileinfo(uc->fat, &uc->files[fDId].fInfo, 
            gDataBuf, dataRead, 
            &totalClusWriten, &lastClus, &fromClus);
    if(res != 0)
    {
      printf("testfatuc:ERROR:fileextract: writing dest\n");
      break;
    }
  }
  fatuc_fclose(uc, fId);
  fatuc_fclose(uc, fDId);
  if(res == -ERROR_FATFS_EOF)
    res = 0;
  return res;
}

int testfatuc_checkreadspeed(struct TFatFsUserContext *uc, 
      char *sFile, uint32 *fileSize)
{
  uint32 dataRead;
  int res, ret, fId;
  uint8 *nBuf;

  printf("testfatuc:INFO: readspeed [%s]\n",sFile);
  if(fatuc_fopen(uc, sFile, &fId) != 0)
  {
    printf("testfatuc:ERROR:readspeed: opening src [%s] file\n", sFile);
    return -1;
  }
  *fileSize = uc->files[fId].fInfo.fileSize;
  ret = -1;
  fatfs_checkbuf_forloadfileclus(uc->fat, gDataBufSize);
  while(1)
  {
    res=fatuc_fread(uc, fId, gDataBuf, gDataBufSize,
          gDataBufSize-TESTFATUC_BUFSIZE_SUB,
          &nBuf, &dataRead);
    if(res != 0)
      break;
#if DEBUG_TESTFAT > 15    
    else
      printf("testfatuc:INFO:readspeed: fatuc_fread dataRead[%ld] fPos[%ld]\n", 
        dataRead, uc->files[fId].fPos);
#endif    
  }
  fatuc_fclose(uc, fId);
  if(res == -ERROR_FATFS_EOF)
    ret = 0;
  return ret;
}

int testfatuc_fatfsfile_checksum(struct TFatFsUserContext *uc, char *sFile)
{
  uint32 dataRead;
  int res, ret, iCur, cSum, fileSize, fId;
  uint8 *nBuf;

  printf("testfatuc:INFO: checksum [%s]\n",sFile);
  if(fatuc_fopen(uc, sFile, &fId) != 0)
  {
    printf("ERR:testfatuc:checksum: opening file[%s]\n", sFile);
    return -1;
  }
  fileSize = uc->files[fId].fInfo.fileSize;
  cSum = 0;
  ret = -1;
  fatfs_checkbuf_forloadfileclus(uc->fat, gDataBufSize);
  while(1)
  {
    res=fatuc_fread(uc, fId, gDataBuf, gDataBufSize, 
          gDataBufSize-TESTFATUC_BUFSIZE_SUB,
          &nBuf, &dataRead);
    if(res != 0)
      break;
#if DEBUG_TESTFAT > 15    
    else
      printf("testfatuc:INFO:checksum: fatuc_fread dataRead[%ld] fPos[%ld]\n", 
        dataRead, uc->files[fId].fPos);
#endif    
    for(iCur=0;iCur<dataRead;iCur++)
    {
      if(fileSize <= 0)
        break;
      cSum += nBuf[iCur];
      fileSize--;
    }
    if(fileSize <= 0)
    {
      ret = 0;
      break;
    }
  }
  fatuc_fclose(uc, fId);
  if(res == -ERROR_FATFS_EOF)
    ret = 0;
  if(ret == 0)
    printf("testfatuc:fatfsfile_checksum: [%s] => [%d]\n", sFile, cSum);
  else
    fprintf(stderr,"ERR:testfatuc:fatfsfile_checksum: [%s] failed\n", sFile);
  return ret;
}

int testfatuc_fileseektest(struct TFatFsUserContext *uc, char *sFile)
{
  FILE *fDest;
  uint32 dataRead;
  int res, ret, resFW, fId, iCur;
  uint8 *nBuf;
  char dFile[64];
  randT tRand;

  printf("testfatuc:INFO: fileseektest [%s]\n",sFile);
  if(fatuc_fopen(uc, sFile, &fId) != 0)
  {
    printf("testfatuc:ERROR:fileseektest: opening src [%s] file\n", sFile);
    return -1;
  }
  ret = 0;
  fatfs_checkbuf_forloadfileclus(uc->fat, gDataBufSize);
  for(iCur=0;(iCur<4)&&(ret==0);iCur++)
  {
    pa_rand_getclipped(&tRand,uc->files[fId].fInfo.fileSize);  
    fatuc_fseek(uc,fId,tRand.rand,FATUC_SEEKSET);
    sprintf(dFile,"/tmp/%d.fatucseek",tRand.rand);
    fDest = fopen(dFile, "w");
    if(fDest == NULL)
    {
      perror("testfatuc:ERROR: opening dest file");
      return -1;
    }
    else
      printf("testfatuc:fileseektest:[%s]\n",dFile);
    while(1)
    {
      res=fatuc_fread(uc, fId, gDataBuf, gDataBufSize,
        gDataBufSize-TESTFATUC_BUFSIZE_SUB,
        &nBuf, &dataRead);
      if(res != 0)
        break;
#if DEBUG_TESTFAT > 15    
      else
        printf("testfatuc:INFO: fatuc_fread dataRead[%ld] fPos[%ld]\n", 
          dataRead, uc->files[fId].fPos);
#endif    
      resFW=fwrite(nBuf, 1, dataRead, fDest);
      if(resFW != dataRead)
      {
        perror("testfatuc:ERROR: less bytes written\n");
	ret = -10;
        break;
      }
    }
    fclose(fDest);
    if(res != -ERROR_FATFS_EOF)
    {
      ret = -10;
      break;
    }
  }
  fatuc_fclose(uc, fId);
  return ret;
}

