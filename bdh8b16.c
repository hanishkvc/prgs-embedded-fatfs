/*
 * bdh8b16.c - library for working with a ide hdd
 * v19Nov2004_2002
 * C Hanish Menon <hanishkvc>, 14july2004
 * 
 */

#ifndef BDH8B16_BENCHMARK
#include <stdio.h>

#include <bdh8b16.h>
#include <errs.h>
#include <utilsporta.h>
#include <linuxutils.h>

uint32 gBDH8B16RegionBase;

#include <dm270utils.h>
#ifdef BDH8B16_USE_INSWK_DM270DMA

static inline void bdh8b16_inswk_dm270dma(uint32 port, uint16* buf, int count)
{

#ifdef BDH8B16_DM270DMA_SAFE
  int iBuf;
  /* REFCTL: Bit15-13 = 0,DMAChannel1BetweenEmifToSDRam */
  iBuf=PA_MEMREAD16(DM270_REFCTL);
  PA_MEMWRITE16(DM270_REFCTL,iBuf&0x1fff);
  /* DMACTL: Bit0 = 1,DMATransfer in progress */
  if(PA_MEMREAD16(DM270_DMACTL)&0x1)
  {
    fprintf(stderr,"ERR:BDH8B16:Someother EMIF DMA in progress, quiting\n");
    exit(-ERROR_FAILED);
  }
  /* FIXME: Source, Dest and Size should be multiples of 4 */
#endif
  /* DMADEVSEL: Bit6-4 = 1,DMASrcIsCF; Bit2-0 = 5,DMADestIsSDRam */
  PA_MEMWRITE16(DM270_DMADEVSEL,0x15);
  /* DMASIZE: */
  PA_MEMWRITE16(DM270_DMASIZE,count*2);
  /* SOURCEADDH: Bit12:=1,FixedAddr, Bit9-0: AddrHigh Relative to CF Region */
  port -= gBDH8B16RegionBase;
  if(port > 0x40) /* Not DM270 CF, Maybe later add DM270 IDE support */
  {
    fprintf(stderr,"ERR:BDH8B16:DM270DMA doesn't support nonCF now, quiting\n");
    exit(-ERROR_INVALID);
  }
  PA_MEMWRITE16(DM270_SOURCEADDH,0x1000|((port>>16)&0x3ff));
  PA_MEMWRITE16(DM270_SOURCEADDL,port&0xffff);
  /* Dest high 10bits and low 16bits */
  (int32)buf -= DM270_SDRAM_BASE; /* int as addr !> 2GB */
  PA_MEMWRITE16(DM270_DESTADDH,((uint32)buf>>16)&0x3ff);
  PA_MEMWRITE16(DM270_DESTADDL,(uint32)buf&0xffff); 
#if DEBUG_PRINT_BDH8B16 > 5000
  fprintf(stderr,"INFO:BDH8B16:DM270DMA:Src[%ld] Dest[0x%lx] count [%d]\n",
    port,(uint32)buf,count);
#endif
  /* DMACTL: Bit0 = 1,Start DMA */
  PA_MEMWRITE16(DM270_DMACTL,0x1);
  while(PA_MEMREAD16(DM270_DMACTL)); /* Inefficient CPU Use but for now */
}

#endif

static inline void bdh8b16_inswk_simple(uint32 port, uint16* buf, int count)
{
  int iCur;
  for(iCur=0;iCur<count;iCur++)
  {
    buf[iCur] = BDH8B16_READ16(port);
#if DEBUG_PRINT_BDH8B16 > 25
    fprintf(stderr,"INFO:BDH8B16:inswk_simple: buf16[%d]=[0x%x]\n",
      iCur, buf[iCur]);
#endif
  }
}

static inline void bdh8b16_inswk_unrolled(uint32 port, uint16* buf, int count)
{
  int iBuf,iWord,iLoops,iRem;
  iBuf = 0;
  iLoops = count/16; iRem = count%16;
  for(iWord=0;iWord<iLoops;iWord++)
  {
    buf[iBuf++]=BDH8B16_READ16(port);
    buf[iBuf++]=BDH8B16_READ16(port);
    buf[iBuf++]=BDH8B16_READ16(port);
    buf[iBuf++]=BDH8B16_READ16(port);

    buf[iBuf++]=BDH8B16_READ16(port);
    buf[iBuf++]=BDH8B16_READ16(port);
    buf[iBuf++]=BDH8B16_READ16(port);
    buf[iBuf++]=BDH8B16_READ16(port);

    buf[iBuf++]=BDH8B16_READ16(port);
    buf[iBuf++]=BDH8B16_READ16(port);
    buf[iBuf++]=BDH8B16_READ16(port);
    buf[iBuf++]=BDH8B16_READ16(port);

    buf[iBuf++]=BDH8B16_READ16(port);
    buf[iBuf++]=BDH8B16_READ16(port);
    buf[iBuf++]=BDH8B16_READ16(port);
    buf[iBuf++]=BDH8B16_READ16(port);
  }
  for(iWord=0;iWord<iRem;iWord++)
    buf[iBuf++]=BDH8B16_READ16(port);
}

static inline int bdh8b16_altstat_waitifbitset(bdkT *bd, char bit, int *iStat, int waitCnt)
{
  char iCur, iTest;

  do{
    iCur = BDH8B16_READ8(BDH8B16_CNTBR_ALTSTAT);
    iCur = BDH8B16_READ8(BDH8B16_CMDBR_STATUS);
    iTest = iCur & bit;
    waitCnt--;
  }while((iTest != 0) && waitCnt);
  *iStat = iCur;
  if(iTest)
    return -ERROR_FAILED;
  return 0;
}

static inline int bdh8b16_altstat_waitifbitclear(bdkT *bd, char bit, int *iStat, int waitCnt)
{
  char iCur,iTest;

  do{
    iCur = BDH8B16_READ8(BDH8B16_CNTBR_ALTSTAT);
    iCur = BDH8B16_READ8(BDH8B16_CMDBR_STATUS);
    iTest = iCur & bit;
    waitCnt--;
  }while((iTest == 0) && waitCnt);
  *iStat = iCur;
  if(iTest)
    return 0;
  return -ERROR_FAILED;
}

static inline int bdh8b16_status_waitifbitset(bdkT *bd, char bit, int *iStat, int waitCnt)
{
  char iCur, iTest;

  do{
    iCur = BDH8B16_READ8(BDH8B16_CMDBR_STATUS);
    iTest = iCur & bit;
    waitCnt--;
  }while((iTest != 0) && waitCnt);
  *iStat = iCur;
  if(iTest)
    return -ERROR_FAILED;
  return 0;
}

static inline int bdh8b16_status_waitifbitclear(bdkT *bd, char bit, int *iStat, int waitCnt)
{
  char iCur,iTest;

  do{
    iCur = BDH8B16_READ8(BDH8B16_CMDBR_STATUS);
    iTest = iCur & bit;
    waitCnt--;
  }while((iTest == 0) && waitCnt);
  *iStat = iCur;
  if(iTest)
    return 0;
  return -ERROR_FAILED;
}

static inline int bdh8b16_dev_set(bdkT *bd, int devId, int enLBA)
{
  char iCur;

  iCur = BDH8B16_READ8(BDH8B16_CMDBR_DEVLBA24);
  iCur = iCur & ~BDH8B16_DEV_DEV1BIT;
  if(devId != 0)
    iCur = iCur | BDH8B16_DEV_DEV1BIT;
  iCur = iCur & ~BDH8B16_DEV_LBABIT;
  if(enLBA != 0)
    iCur = iCur | BDH8B16_DEV_LBABIT;
  BDH8B16_WRITE8(BDH8B16_CMDBR_DEVLBA24,iCur);
  return 0;
}

static inline int bdh8b16_devlba_set(bdkT *bd, int devId, int enLBA, int lba2427)
{
  char iCur;

  iCur = BDH8B16_READ8(BDH8B16_CMDBR_DEVLBA24);
  iCur = iCur & ~BDH8B16_DEV_DEV1BIT;
  if(devId != 0)
    iCur = iCur | BDH8B16_DEV_DEV1BIT;
  iCur = iCur & ~BDH8B16_DEV_LBABIT;
  if(enLBA != 0)
    iCur = iCur | BDH8B16_DEV_LBABIT;
  iCur = iCur | lba2427;
  BDH8B16_WRITE8(BDH8B16_CMDBR_DEVLBA24,iCur);
  return 0;
}

static inline int bdh8b16_readyforcmd(bdkT *bd, int dev, int enLBA, int lba2427)
{
  int iStat;

  if(bdh8b16_status_waitifbitset(bd,
      BDH8B16_STATUS_BSYBIT | BDH8B16_STATUS_DRQBIT | BDH8B16_STATUS_ERRBIT,
      &iStat, BDH8B16_WAIT_CMDINIT) != 0)
  {
    fprintf(stderr,"???:BDH8B16:readyforcmd:Stat[0x%x]dev not OK,reseting\n",
      iStat);
    bd->reset(bd);
    if(bdh8b16_status_waitifbitset(bd,
        BDH8B16_STATUS_BSYBIT | BDH8B16_STATUS_DRQBIT | BDH8B16_STATUS_ERRBIT,
        &iStat, BDH8B16_WAIT_CMDINIT) != 0)
    {
      fprintf(stderr,"ERR:BDH8B16:readyforcmd:Stat[0x%x]dev still not OK\n",
        iStat);
      return -ERROR_TIMEOUT;
    }
  }
  bdh8b16_devlba_set(bd,dev,enLBA,lba2427);
  if(bdh8b16_status_waitifbitset(bd,BDH8B16_STATUS_BSYBIT,&iStat,
      BDH8B16_WAIT_CMDINIT) != 0)
  {
    fprintf(stderr,"ERR:BDH8B16:not readyforcmd, suddenly BSY\n");
    return -ERROR_TIMEOUT;
  }
  if(bdh8b16_status_waitifbitclear(bd,BDH8B16_STATUS_DRDYBIT,&iStat,
      BDH8B16_WAIT_CMDINIT) != 0)
  {
    fprintf(stderr,"ERR:BDH8B16:not readyforcmd, still not DRDY\n");
    return -ERROR_TIMEOUT;
  }
  return 0;
}

static inline int bdh8b16_checkstatus(bdkT *bd,uint8 cmd,int *iStatus,int *iError,int waitCnt)
{
  int ret;

  ret=bdh8b16_altstat_waitifbitset(bd,BDH8B16_STATUS_BSYBIT,iStatus,waitCnt);
  *iStatus = BDH8B16_READ8(BDH8B16_CMDBR_STATUS);
#if DEBUG_PRINT_BDH8B16 > 25
  printf("INFO:[status=0x%x]\n",*iStatus);
#endif
  if(ret!=0)
  {
    fprintf(stderr,"ERR:BDH8B16:checkstatus:0: Cmd[0x%x] Stat[0x%x] timeout\n", 
      cmd,*iStatus);
    return -ERROR_TIMEOUT;
  }
  if(*iStatus & BDH8B16_STATUS_ERRBIT)
  {
    *iError = BDH8B16_READ8(BDH8B16_CMDBR_ERROR);
    fprintf(stderr,"ERR:BDH8B16:checkstatus:0: Cmd[0x%x] Stat[0x%x] Err[0x%x]\n",
      cmd,*iStatus,*iError);
    return -ERROR_FAILED;
  }
  return 0;
}

static inline int bdh8b16_sendcmd(bdkT *bd, uint8 cmd, int *iStatus, int *iError, int waitCnt)
{
  int ret;
  
  BDH8B16_WRITE8(BDH8B16_CNTBR_DEVCNT,BDH8B16_DEVCNT_NOINTBIT);
  BDH8B16_WRITE8(BDH8B16_CMDBR_COMMAND,cmd);
  if(lu_nanosleeppaka(0,BDH8B16_WAITNS_DEVUPDATESSTATUS) != 0)
    fprintf(stderr,"DEBUG:BDH8B16: nanosleeppaka after cmd[0x%d] failed\n",cmd);
  ret = bdh8b16_checkstatus(bd,cmd,iStatus,iError,waitCnt);
  return ret;
}

void bdh8b16_printsignature(bdkT *bd)
{
#if DEBUG_PRINT_BDH8B16 > 25
  printf("INFO:BDH8B16:Signature: SECCNT[0x%x] LBALow[0x%x] LBAMid[0x%x] LBAHigh[0x%x] DevLBA[0x%x]\n",
    BDH8B16_READ8(BDH8B16_CMDBR_SECCNT), BDH8B16_READ8(BDH8B16_CMDBR_LBA0),
    BDH8B16_READ8(BDH8B16_CMDBR_LBA8), BDH8B16_READ8(BDH8B16_CMDBR_LBA16),
    BDH8B16_READ8(BDH8B16_CMDBR_DEVLBA24));
#endif
}

int bdh8b16_reset(bdkT *bd)
{
  int ret, iStat;
  
  /* P1 Device 0 */
  BDH8B16_WRITE8(BDH8B16_CNTBR_DEVCNT,BDH8B16_DEVCNT_SRSTBIT); /* Set reset */
  if(lu_nanosleeppaka(BDH8B16_WAITSEC_SRST_P1,BDH8B16_WAITNS_DEVUPDATESSTATUS) != 0)
    fprintf(stderr,"ERR:BDH8B16:reset:p1: nanosleep\n");
  ret = BDH8B16_READ8(BDH8B16_CMDBR_ERROR);
  fprintf(stderr,"INFO:BDH8B16:reset:p1: errorreg diagnostics [0x%x]\n",ret);
  /* P2 Device 1 and BSY clear*/
  BDH8B16_WRITE8(BDH8B16_CNTBR_DEVCNT,0); /* Clear reset */
  if(lu_nanosleeppaka(BDH8B16_WAITSEC_SRST_P2,0) != 0)
    fprintf(stderr,"ERR:BDH8B16:reset:p2: nanosleep\n");
  bdh8b16_printsignature(bd);
  ret = BDH8B16_READ8(BDH8B16_CMDBR_ERROR);
  fprintf(stderr,"INFO:BDH8B16:reset:p2: errorreg diagnostics [0x%x]\n",ret);
  if(ret == BDH8B16_SRST_DIAGNOSTICS_ALLOK)
  {
    fprintf(stderr,"INFO:BDH8B16:reset: all ok\n");
    ret = 0;
  }
  else
  {
    fprintf(stderr,"ERR:BDH8B16:reset: errorreg diagnostics [0x%x]\n",ret);
    if(ret == BDH8B16_SRST_DIAGNOSTICS_D0OK)
      fprintf(stderr,"INFO:BDH8B16:reset: Dev0 Passed, Dev1 Failed\n");
    ret = -ERROR_FAILED;
  }
  if(bdh8b16_status_waitifbitset(bd,BDH8B16_STATUS_BSYBIT,&iStat,
      BDH8B16_WAIT_SRSTBSY) != 0)
  {
    fprintf(stderr,"ERR:BDH8B16:reset:p2: iStat[0x%x] BSY not cleared\n",iStat);
    ret = -ERROR_FAILED;
  }
  /* P3 DRDY Set */
  if(lu_nanosleeppaka(BDH8B16_WAITSEC_SRST_P3,0) != 0)
    fprintf(stderr,"ERR:BDH8B16:reset:p3: nanosleep\n");
  if(bdh8b16_status_waitifbitclear(bd,BDH8B16_STATUS_DRDYBIT,&iStat,
      BDH8B16_WAIT_SRSTDRDY) != 0)
  {
    fprintf(stderr,"ERR:BDH8B16:reset:p3: iStat[0x%x] DRDY not set\n",iStat);
    ret = -ERROR_FAILED;
  }
  return ret;
}

int bdh8b16_init(bdkT *bd, char *secBuf, int grpId, int devId, int reset)
{
  int iStat,iError,ret;
  uint16 *buf16 = (uint16*)secBuf;
  
#ifdef PRG_MODE_DM270
  if(grpId == BDH8B16_GRPID_DM270IDE_H8B16)
  {
    gBDH8B16RegionBase = PA_MEMREAD16(0x30A50) * 0x100000;
    bd->CNTBR = gBDH8B16RegionBase + BDH8B16_CNTBR_OFFSET;
    bd->CMDBR = gBDH8B16RegionBase + BDH8B16_CMDBR_OFFSET;
    fprintf(stderr,"INFO:BDH8B16:DM270:H8B16 IDE - devId [%d] \n",devId);

    /* Enable Buffer for IDE access from DM270 */
    gio_dir_output(29);
    gio_bit_clear(29);
    lu_nanosleeppaka(1,0);
    /* Debug */
    fprintf(stderr,"INFO:BDH8B16:CMDBR registers\n");
    fprintf(stderr,"INFO:BDH8B16:_DATA [0x%x]\n",BDH8B16_CMDBR_DATA);
    fprintf(stderr,"INFO:BDH8B16:_FEATURES [0x%x]\n",BDH8B16_CMDBR_FEATURES);
    fprintf(stderr,"INFO:BDH8B16:_ERROR [0x%x]\n",BDH8B16_CMDBR_ERROR);
    fprintf(stderr,"INFO:BDH8B16:_SECCNT [0x%x]\n",BDH8B16_CMDBR_SECCNT);
    fprintf(stderr,"INFO:BDH8B16:_LBA0 [0x%x]\n",BDH8B16_CMDBR_LBA0);
    fprintf(stderr,"INFO:BDH8B16:_LBA8 [0x%x]\n",BDH8B16_CMDBR_LBA8);
    fprintf(stderr,"INFO:BDH8B16:_LBA16 [0x%x]\n",BDH8B16_CMDBR_LBA16);
    fprintf(stderr,"INFO:BDH8B16:_DEVLBA24 [0x%x]\n",BDH8B16_CMDBR_DEVLBA24);
    fprintf(stderr,"INFO:BDH8B16:_COMMAND [0x%x]\n",BDH8B16_CMDBR_COMMAND);
    fprintf(stderr,"INFO:BDH8B16:_STATUS [0x%x]\n",BDH8B16_CMDBR_STATUS);
    fprintf(stderr,"INFO:BDH8B16:CNTBR registers\n");
    fprintf(stderr,"INFO:BDH8B16:_DEVCNT [0x%x]\n",BDH8B16_CNTBR_DEVCNT);
    fprintf(stderr,"INFO:BDH8B16:_ALTSTAT [0x%x]\n",BDH8B16_CNTBR_ALTSTAT);
    /****/
#ifdef BDH8B16_DM270_EMIF16
    {
    uint16 iEmif;
    iEmif = PA_MEMREAD16(0x30A10);
    iEmif |= 0x4000;
    fprintf(stderr,"INFO:BDH8B16:DM270:IDE setting EMIF16 [0x%x]\n", iEmif);
    PA_MEMWRITE16(0x30A10,iEmif);
    }
#endif
  }
  else
#endif
  {
    fprintf(stderr,"ERR:BDH8B16: ide??? - unknown \n");
    return -ERROR_INVALID;
  }
  fprintf(stderr,"INFO:BDH8B16: CMDBR [0x%x] CNTBR [0x%x]\n", bd->CMDBR, bd->CNTBR);

  if(reset)
    bdh8b16_reset(bd);

  if((ret=bdh8b16_readyforcmd(bd,devId,BDH8B16_CFG_LBA,0)) != 0) return ret;
  if(bdh8b16_sendcmd(bd,BDH8B16_CMD_IDENTIFYDEVICE,&iStat,&iError,
      BDH8B16_WAIT_CMDTIME) != 0)
  {
    fprintf(stderr,"ERR:BDH8B16: During IDENTIFYDEVICE cmd\n");
    return -ERROR_FAILED;
  }
  if((iStat&BDH8B16_STATUS_DRQBIT)==0)
  {
    fprintf(stderr,"ERR:BDH8B16: IDENTIFYDEVICE DRQ not set\n");
    return -ERROR_FAILED;
  }
  bdh8b16_printsignature(bd);
  for(iStat=0;iStat<256;iStat++)
  {
    buf16[iStat]=BDH8B16_READ16(BDH8B16_CMDBR_DATA);
  }
#ifdef BDH8B16_CHECK_DRQAFTERCMDCOMPLETION  
  if(bdh8b16_altstat_waitifbitset(bd,BDH8B16_STATUS_DRQBIT,&iStat,BDH8B16_WAIT_AFTERCMDCOMPLETION) != 0)
    fprintf(stderr,"DEBUG:BDH8B16:init: [0x%x] DRQ even after cmd completion\n",
      iStat);
#if DEBUG_PRINT_BDH8B16 > 25
  fprintf(stderr,"INFO:BDH8B16:ID: after cmd completion [0x%x] \n",iStat);
#endif
#endif
  
#if DEBUG_PRINT_BDH8B16 > 10
  printf("\n****IDENTIFYDEVICE****\n");
  printf("INFO:BDH8B16:ID: numLogical Cyl[%d] Heads[%d] Secs[%d]\n", 
    buf16[1], buf16[3], buf16[6]);
  printf("INFO:BDH8B16:ID: Firmware revision [%s]\n",
    buffer_read_string_noup((char*)&buf16[23],8,(char*)&buf16[160],32));
  printf("INFO:BDH8B16:ID: Model number [%s]\n",
    buffer_read_string_noup((char*)&buf16[27],40,(char*)&buf16[160],64));
  printf("INFO:BDH8B16:ID:51:OldPIOMode Supported [%d:0x%x]\n",
    (buf16[51]&0xff00)>>8,buf16[51]);
  printf("INFO:BDH8B16:ID:53:FieldValidity AdvancedPIO [%d:0x%x]\n",
    buf16[53]&0x2,buf16[53]);
  printf("INFO:BDH8B16:ID:--:APIO 64[%d] 65[%d] 66[%d] 67[%d] 68[%d]\n",
    buf16[64],buf16[65],buf16[66],buf16[67],buf16[68]);
#endif
  if(buf16[49]&0x200)
    printf("INFO:BDH8B16:ID: LBA Supported, totalUserAddressableSecs[??]\n");
  else
  {
    if(BDH8B16_CFG_LBA)
    {
      fprintf(stderr,"ERR:BDH8B16:ID: LBA NOT Supported\n");
      return -ERROR_NOTSUPPORTED;
    }
    printf("INFO:BDH8B16:ID: LBA NOT Supported\n");
  }
  if((bd->multiCnt=buf16[47]&0xff) > 0)
  {
    printf("INFO:BDH8B16:ID:47: READ/WRITE MULTIPLE[0x%x]\n",buf16[47]);
#ifdef BDH8B16_CFG_RWMULTIPLE
    if((ret=bdh8b16_readyforcmd(bd,devId,BDH8B16_CFG_LBA,0)) != 0) return ret;
    BDH8B16_WRITE8(BDH8B16_CMDBR_SECCNT,bd->multiCnt);
    if(bdh8b16_sendcmd(bd,BDH8B16_CMD_SETMULTIPLEMODE,&iStat,&iError, 
        BDH8B16_WAIT_CMDTIME) != 0)
    {
      fprintf(stderr,"ERR:BDH8B16: During SETMULTIPLEMODE cmd\n");
      return -ERROR_FAILED;
    }
    fprintf(stderr,"INFO:BDH8B16: SETMULTIPLEMODE set\n");
#endif
  }
  else
  {
#ifdef BDH8B16_CFG_RWMULTIPLE
    {
      fprintf(stderr,"ERR:BDH8B16: READ/WRITE MULTIPLE not supported\n");
      return -ERROR_NOTSUPPORTED;
    }
#endif
    printf("INFO:BDH8B16:ID:47: READ/WRITE MULTIPLE not Supported\n");
  }
  /* NOTE: Add SETFEATURE cmd if required later */
#ifdef BDH8B16_CFG_SETFEATURES  
  if((ret=bdh8b16_readyforcmd(bd,devId,BDH8B16_CFG_LBA,0)) != 0) return ret;
  BDH8B16_WRITE8(BDH8B16_CMDBR_SECCNT,0x0b);
  BDH8B16_WRITE8(BDH8B16_CMDBR_FEATURES,0x03);
  if(bdh8b16_sendcmd(bd,0xEF,&iStat,&iError, BDH8B16_WAIT_CMDTIME) != 0)
  {
    fprintf(stderr,"ERR:BDH8B16: During SETFEATURES cmd\n");
    return -ERROR_FAILED;
  }
  fprintf(stderr,"INFO:BDH8B16: SETFEATURES triggered\n");
#endif  
  bd->grpId = grpId; bd->devId = devId;
  bd->secSize = BDK_SECSIZE_512;
  bd->totSecs = 0xFFFFFFFF; /* FIXME */
  return 0;
}

#endif /* BDH8B16_BENCHMARK */

int bdh8b16_get_sectors(bdkT *bd, long sec, long count, char*buf)
{
  int iBuf, iStat, iError, ret;
  int iLoops,iCurSecs,iLoop,iSec;
  uint16 *buf16 = (uint16*)buf;

#if (DEBUG_PRINT_BDH8B16 > 15)
  printf("INFO:BDH8B16: sec[%ld] count[%ld]\n", sec, count);
#endif

  iBuf=0;
  iLoops = (count/BDH8B16_SECCNT_USE);
  if((count%BDH8B16_SECCNT_USE) != 0)
    iLoops++;
  for(iLoop=0;iLoop<iLoops;iLoop++)
  {
#if BDH8B16_SECCNT_MAX == BDH8B16_SECCNT_USE
    if(count < BDH8B16_SECCNT_MAX)
      iCurSecs = count;
    else
      iCurSecs = 0;
#else
    if(count < BDH8B16_SECCNT_USE)
      iCurSecs = count;
    else
      iCurSecs = BDH8B16_SECCNT_USE;
#endif
#if DEBUG_PRINT_BDH8B16 > 25
    printf("INFO:BDH8B16:get: curStartSec[%ld] remainingCount[%ld]\n",sec,count);
#endif
    if((ret=bdh8b16_readyforcmd(bd,bd->devId,BDH8B16_CFG_LBA,
              ((sec&0xf000000)>>24))) != 0) return ret;
    BDH8B16_WRITE8(BDH8B16_CMDBR_SECCNT,iCurSecs);
    BDH8B16_WRITE8(BDH8B16_CMDBR_LBA0,(sec&0xff));
    BDH8B16_WRITE8(BDH8B16_CMDBR_LBA8,((sec&0xff00)>>8));
    BDH8B16_WRITE8(BDH8B16_CMDBR_LBA16,((sec&0xff0000)>>16));
#ifdef BDH8B16_CFG_RWMULTIPLE    
    if(bdh8b16_sendcmd(bd,BDH8B16_CMD_READMULTIPLE,&iStat,&iError,
        BDH8B16_WAIT_CMDTIME) != 0)
#else			    
    if(bdh8b16_sendcmd(bd,BDH8B16_CMD_READSECTORS,&iStat,&iError,
        BDH8B16_WAIT_CMDTIME) != 0)
#endif		    
    {
      bdh8b16_printsignature(bd);
      fprintf(stderr,"ERR:BDH8B16: During READSECTORS cmd\n");
      return -ERROR_FAILED;
    }
    if(iCurSecs == 0) iCurSecs = BDH8B16_SECCNT_MAX;
#ifdef BDH8B16_CFG_RWMULTIPLE
    for(iSec=0;iSec<iCurSecs;iSec+=bd->multiCnt)
#else
    for(iSec=0;iSec<iCurSecs;iSec++)
#endif	    
    {
      if(iSec > 0)
      {
        ret=bdh8b16_checkstatus(bd,0xFF,&iStat,&iError,BDH8B16_WAIT_CMDTIME);
        if(ret != 0)
        {
          fprintf(stderr,"ERR:BDH8B16: middle of READSECTORS, iStat[0x%x] iLoop[%d] iSec[%d]\n",iStat,iLoop,iSec);
          return ret;
        }
      }
      if((iStat&BDH8B16_STATUS_DRQBIT)==0)
      {
        fprintf(stderr,"ERR:BDH8B16: READSECTORS DRQ not set, iStat[0x%x] iLoop[%d] iSec[%d]\n",iStat,iLoop,iSec);
        return -ERROR_FAILED;
      }
#ifdef BDH8B16_CFG_RWMULTIPLE
      {
      int iMulti;      
      iMulti = iCurSecs - iSec;
      if(iMulti > bd->multiCnt) iMulti = bd->multiCnt;
      BDH8B16_READ16S(BDH8B16_CMDBR_DATA,&buf16[iBuf],256*iMulti);
      iBuf+=256*iMulti;
      }
#else
      BDH8B16_READ16S(BDH8B16_CMDBR_DATA,&buf16[iBuf],256);
      iBuf+=256;
#endif      
#ifdef BDH8B16_BENCHMARK
      iBuf = 0;
#endif      
    }
    count-=iCurSecs;
    sec+=iCurSecs;
  }
#ifdef BDH8B16_CHECK_DRQAFTERCMDCOMPLETION  
  if(bdh8b16_altstat_waitifbitset(bd,BDH8B16_STATUS_DRQBIT,&iStat,BDH8B16_WAIT_AFTERCMDCOMPLETION) != 0)
    fprintf(stderr,"DEBUG:BDH8B16:get: [0x%x] DRQ even after cmd completion\n",
      iStat);
#if DEBUG_PRINT_BDH8B16 > 25
  fprintf(stderr,"INFO:BDH8B16:get: after cmd completion [0x%x] \n",iStat);
#endif
#endif
  return 0;
}

#ifndef BDH8B16_BENCHMARK

int bdh8b16_cleanup(bdkT *bd)
{
  return 0;
}

void bdh8b16_info()
{
#ifdef BDH8B16_CFG_RWMULTIPLE
  fprintf(stderr,"INFO:BDH8B16: CFG_RWMULTIPLE active\n");
#endif
#ifdef BDH8B16_CFG_SETFEATURES
  fprintf(stderr,"INFO:BDH8B16: CFG_SETFEATURES active\n");
#endif
#ifdef BDH8B16_CHECK_DRQAFTERCMDCOMPLETION
  fprintf(stderr,"INFO:BDH8B16: CHECK_DRQAFTERCMDCOMPLETION active\n");
#endif
  if(BDH8B16_CFG_LBA)
    fprintf(stderr,"INFO:BDH8B16: LBA active\n");
}

int bdh8b16_setup(bdkT *bdk)
{
  bdh8b16_info();
  bdk->init = bdh8b16_init;
  bdk->cleanup = bdh8b16_cleanup;
  bdk->reset = bdh8b16_reset;
  bdk->get_sectors = bdh8b16_get_sectors;
  bdk->get_sectors_benchmark = NULL;
  pa_strncpy(bdk->name,"bdh8b16",BDK_DEVNAMELEN);
  return 0;
}

#endif /* BDH8B16_BENCHMARK */

