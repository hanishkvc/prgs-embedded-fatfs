/*
 * bdhdd.c - library for working with a ide hdd
 * v10Oct2004_0025
 * C Hanish Menon <hanishkvc>, 14july2004
 * 
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <inall.h>
#include <bdhdd.h>
#include <errs.h>
#include <utilsporta.h>

int bdhdd_status_waitifbitset(char bit)
{
  char iCur, iTest;

  do{
    iCur = BDHDD_READ8(BDHDD_CMDBR_STATUS);
    iTest = iCur & bit;
  }while(iTest != 0);
  return iCur;
}

int bdhdd_status_waitifbitclear(char bit)
{
  char iCur,iTest;

  do{
    iCur = BDHDD_READ8(BDHDD_CMDBR_STATUS);
    iTest = iCur & bit;
  }while(iTest == 0);
  return iCur;
}

int bdhdd_dev_set(int devId, int enLBA)
{
  char iCur;

  iCur = BDHDD_READ8(BDHDD_CMDBR_DEVLBA24);
  iCur = iCur & ~BDHDD_DEV_DEV1BIT;
  if(devId != 0)
    iCur = iCur | BDHDD_DEV_DEV1BIT;
  iCur = iCur & ~BDHDD_DEV_LBABIT;
  if(enLBA != 0)
    iCur = iCur | BDHDD_DEV_LBABIT;
  BDHDD_WRITE8(BDHDD_CMDBR_DEVLBA24,iCur);
  return 0;
}

int bdhdd_devlba_set(int devId, int enLBA, int lba2427)
{
  char iCur;

  iCur = BDHDD_READ8(BDHDD_CMDBR_DEVLBA24);
  iCur = iCur & ~BDHDD_DEV_DEV1BIT;
  if(devId != 0)
    iCur = iCur | BDHDD_DEV_DEV1BIT;
  iCur = iCur & ~BDHDD_DEV_LBABIT;
  if(enLBA != 0)
    iCur = iCur | BDHDD_DEV_LBABIT;
  iCur = iCur | lba2427;
  BDHDD_WRITE8(BDHDD_CMDBR_DEVLBA24,iCur);
  return 0;
}

int bdhdd_sendcmd(char cmd, int *iStatus, int *iError)
{
  struct timespec ts;
  
  BDHDD_WRITE8(BDHDD_CNTBR_DEVCNT,BDHDD_DEVCNT_NOINTBIT);
  BDHDD_WRITE8(BDHDD_CMDBR_COMMAND,cmd);
  ts.tv_sec = 0;
  ts.tv_nsec = 10000;
  if(nanosleep(&ts,&ts) != 0)
    printf("DEBUG: Interrupted during nanosleep\n");
  *iStatus = bdhdd_status_waitifbitset(BDHDD_STATUS_BSYBIT);
  printf("[status=0x%x]\n",*iStatus);
  if(*iStatus & BDHDD_STATUS_ERRBIT)
  {
    *iError = BDHDD_READ8(BDHDD_CMDBR_ERROR);
    printf("[error=0x%x]\n",*iError);
    return -ERROR_FAILED;
  }
  return 0;
}

void bdhdd_printsignature()
{
#if 0	
  printf("INFO:Signature: SECCNT[0x%x] LBALow[0x%x] LBAMid[0x%x] LBAHigh[0x%x] DevLBA[0x%x]\n",
    BDHDD_READ8(BDHDD_CMDBR_SECCNT), BDHDD_READ8(BDHDD_CMDBR_LBA0),
    BDHDD_READ8(BDHDD_CMDBR_LBA8), BDHDD_READ8(BDHDD_CMDBR_LBA16),
    BDHDD_READ8(BDHDD_CMDBR_DEVLBA24));
#endif
}

int bdhdd_init(bdkT *bd, char *secBuf)
{
  int iStat,iError;
  uint16 *buf16 = (uint16*)secBuf;
  
  if(ioperm(0x0,0x3ff,1) != 0)
  {
    printf("ERR: Failed to get ioperm\n");
    exit(1);
  }
  if(BDHDD_CMDBR == 0x170)
    printf("INFO:BDHDD: ide1 - Secondary \n");
  else if(BDHDD_CMDBR == 0x1f0)
    printf("INFO:BDHDD: ide0 - Primary \n");
  else
    printf("INFO:BDHDD: ide??? - unknown \n");
  bdhdd_status_waitifbitset(BDHDD_STATUS_BSYBIT);
  bdhdd_dev_set(0,1);
  bdhdd_status_waitifbitset(BDHDD_STATUS_BSYBIT);
  bdhdd_status_waitifbitclear(BDHDD_STATUS_DRDYBIT);
  if(bdhdd_sendcmd(BDHDD_CMD_IDENTIFYDEVICE,&iStat,&iError) != 0)
  {
    fprintf(stderr,"ERR: During IDENTIFYDEVICE cmd\n");
    exit(2);
  }
  if((iStat&BDHDD_STATUS_DRQBIT)==0)
  {
    fprintf(stderr,"ERR:IDENTIFYDEVICE DRQ not set\n");
    exit(3);
  }
  bdhdd_printsignature();
  for(iStat=0;iStat<256;iStat++)
  {
    buf16[iStat]=BDHDD_READ16(BDHDD_CMDBR_DATA);
  }
  printf("\n****IDENTIFYDEVICE****\n");
  printf("INFO: numLogical Cyl[%d] Heads[%d] Secs[%d]\n", 
    buf16[1], buf16[3], buf16[6]);
  printf("INFO: Firmware revision [%s]\n",
    buffer_read_string_noup((char*)&buf16[23],8,(char*)&buf16[160],32));
  printf("INFO: Model number [%s]\n",
    buffer_read_string_noup((char*)&buf16[27],40,(char*)&buf16[160],64));
  if(buf16[49]&0x200)
    printf("INFO: LBA Supported, totalUserAddressableSecs[??]\n");
  else
    printf("ERR: LBA NOT Supported, quiting\n");
  printf("\n*******Over********\n");
	
  bd->secSize = BDK_SECSIZE_512;
  bd->totSecs = 0xFFFFFFFF;
  return 0;
}

int bdhdd_get_sectors(bdkT *bd, long sec, long count, char*buf)
{
  int iBuf, toRead, iStat, iError;
  int iLoops,iCurSecs,iLoop,iSec;
  uint16 *buf16 = (uint16*)buf;

#if (DEBUG_PRINT_BDFILE > 10)
  printf("INFO:bdhdd: sec[%ld] count[%ld]\n", sec, count);
#endif

  iBuf=0;
  iLoops = (count/256);
  if((count%256) != 0)
    iLoops++;
  for(iLoop=0;iLoop<iLoops;iLoop++)
  {
    if(count < 256) 
      iCurSecs = count;
    else
      iCurSecs = 0;
    printf("INFO:bdhdd:get: curStartSec[%ld] remainingCount[%ld]\n",sec,count);
    
    bdhdd_status_waitifbitset(BDHDD_STATUS_BSYBIT);
    bdhdd_devlba_set(0,1,((sec&0xf000000)>>24));
    bdhdd_status_waitifbitset(BDHDD_STATUS_BSYBIT);
    bdhdd_status_waitifbitclear(BDHDD_STATUS_DRDYBIT);
    BDHDD_WRITE8(BDHDD_CMDBR_SECCNT,iCurSecs);
    BDHDD_WRITE8(BDHDD_CMDBR_LBA0,(sec&0xff));
    BDHDD_WRITE8(BDHDD_CMDBR_LBA8,((sec&0xff00)>>8));
    BDHDD_WRITE8(BDHDD_CMDBR_LBA16,((sec&0xff0000)>>16));
    if(bdhdd_sendcmd(BDHDD_CMD_READSECTORS,&iStat,&iError) != 0)
    {
      bdhdd_printsignature();
      fprintf(stderr,"ERR: During READSECTORS cmd\n");
      exit(2);
    }
    if(iCurSecs == 0) iCurSecs = 256;
    for(iSec=0;iSec<iCurSecs;iSec++)
    {
      if((iStat&BDHDD_STATUS_DRQBIT)==0)
      {
        fprintf(stderr,"ERR:READSECTORS DRQ not set\n");
        exit(3);
      }
      for(toRead=0;toRead<256;toRead++)
      {
        buf16[iBuf++]=BDHDD_READ16(BDHDD_CMDBR_DATA);
      }
    }
    count-=iCurSecs;
    sec+=iCurSecs;
  }
  return 0;
}

int bdhdd_cleanup(bdkT *bd)
{
  return 0;
}

int bdhdd_setup()
{
  bdkHdd.init = bdhdd_init;
  bdkHdd.cleanup = bdhdd_cleanup;
  bdkHdd.get_sectors = bdhdd_get_sectors;
  pa_strncpy(bdkHdd.name,"bdhdd",BDK_DEVNAMELEN);
  return 0;
}

