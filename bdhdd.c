/*
 * bdhdd.c - library for working with a ide hdd
 * v05Oct2004_1059
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

int bdhdd_selectdev(int devId)
{
  char iCur;

  iCur = BDHDD_READ8(BDHDD_CMDBR_DEVLBA24);
  iCur = iCur & ~BDHDD_DEV_DEV1BIT;
  if(devId != 0)
    iCur = iCur | BDHDD_DEV_DEV1BIT;
  BDHDD_WRITE8(BDHDD_CMDBR_DEVLBA24,iCur);
  return 0;
}

int bdhdd_sendcmd(char cmd, int *iStatus)
{
  
  BDHDD_WRITE8(BDHDD_CNTBR_DEVCNT,BDHDD_DEVCNT_NOINTBIT);
  BDHDD_WRITE8(BDHDD_CMDBR_COMMAND,cmd);
  sleep(1);
  *iStatus = bdhdd_status_waitifbitset(BDHDD_STATUS_BSYBIT);
  printf("[status=0x%x]\n",*iStatus);
  if(*iStatus & BDHDD_STATUS_ERRBIT)
    return -ERROR_FAILED;
  return 0;
}

int bdhdd_init(bdkT *bd)
{
  int iStat,iCur;
  
  if(ioperm(0x0,0x3ff,1) != 0)
  {
    printf("ERR: Failed to get ioperm\n");
    exit(1);
  }
  bdhdd_status_waitifbitset(BDHDD_STATUS_BSYBIT);
  bdhdd_selectdev(0);
  bdhdd_status_waitifbitset(BDHDD_STATUS_BSYBIT);
  bdhdd_status_waitifbitclear(BDHDD_STATUS_DRDYBIT);
  if(bdhdd_sendcmd(BDHDD_CMD_IDENTIFYDEVICE,&iStat) != 0)
  {
    fprintf(stderr,"ERR: During IDENTIFYDEVICE cmd\n");
    exit(2);
  }
  printf("IdentifyDevice says:\n");
  for(iStat=0;iStat<256;iStat++)
  {
    iCur=BDHDD_READ16(BDHDD_CMDBR_DATA);
    printf("%d:[0x%x] - ",iStat,iCur);
    printf("[%c%c]\n",((iCur&0xff00)>>8),(iCur&0xff));
  }
  printf("\n*******Over********\n");
	
  bd->secSize = BDK_SECSIZE_512;
  bd->totSecs = 0xFFFFFFFF;
  return 0;
}

int bdhdd_get_sectors(bdkT *bd, long sec, long count, char*buf)
{
  int res, toRead;

#if (DEBUG_PRINT_BDFILE > 10)
  printf("INFO:bdhdd: sec[%ld] count[%ld]\n", sec, count);
#endif

  toRead = count*bd->secSize;
  res = 0;
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

