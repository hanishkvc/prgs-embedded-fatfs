/*
 * bdhdd.c - library for working with a ide hdd
 * v19Nov2004_1332
 * C Hanish Menon <hanishkvc>, 14july2004
 * 
 */

#ifndef BDHDD_BENCHMARK
#include <stdio.h>

#include <bdhdd.h>
#include <errs.h>
#include <utilsporta.h>
#include <linuxutils.h>

#ifdef BDHDD_USE_INSWK_DM270DMA
#include <dm270utils.h>

static inline void bdhdd_inswk_dm270dma(uint32 port, uint16* buf, int count)
{

#ifdef BDHDD_DM270DMA_SAFE
  int iBuf;
  /* REFCTL: Bit15-13 = 0,DMAChannel1BetweenEmifToSDRam */
  iBuf=PA_MEMREAD16(DM270_REFCTL);
  PA_MEMWRITE16(DM270_REFCTL,iBuf&0x1fff);
  /* DMACTL: Bit0 = 1,DMATransfer in progress */
  if(PA_MEMREAD16(DM270_DMACTL)&0x1)
  {
    fprintf(stderr,"ERR:BDHDD:Someother EMIF DMA in progress, quiting\n");
    exit(-ERROR_FAILED);
  }
  /* FIXME: Source, Dest and Size should be multiples of 4 */
#endif
  /* DMADEVSEL: Bit6-4 = 1,DMASrcIsCF; Bit2-0 = 5,DMADestIsSDRam */
  PA_MEMWRITE16(DM270_DMADEVSEL,0x15);
  /* DMASIZE: */
  PA_MEMWRITE16(DM270_DMASIZE,count*2);
  /* SOURCEADDH: Bit12:=1,FixedAddr, Bit9-0: AddrHigh Relative to CF Region */
  port -= BDHDD_DM270CF_CMDBR;
  if(port > 16) /* Not DM270 CF, Maybe later add DM270 IDE support */
  {
    fprintf(stderr,"ERR:BDHDD:DM270DMA doesn't support nonCF now, quiting\n");
    exit(-ERROR_INVALID);
  }
  PA_MEMWRITE16(DM270_SOURCEADDH,0x1000|((port>>16)&0x3ff));
  PA_MEMWRITE16(DM270_SOURCEADDL,port&0xffff);
  /* Dest high 10bits and low 16bits */
  (int32)buf -= DM270_SDRAM_BASE; /* int as addr !> 2GB */
  PA_MEMWRITE16(DM270_DESTADDH,((uint32)buf>>16)&0x3ff);
  PA_MEMWRITE16(DM270_DESTADDL,(uint32)buf&0xffff); 
#if DEBUG_PRINT_BDHDD > 5000
  fprintf(stderr,"INFO:BDHDD:DM270DMA:Src[%ld] Dest[0x%lx] count [%d]\n",
    port,(uint32)buf,count);
#endif
  /* DMACTL: Bit0 = 1,Start DMA */
  PA_MEMWRITE16(DM270_DMACTL,0x1);
  while(PA_MEMREAD16(DM270_DMACTL)); /* Inefficient CPU Use but for now */
}

#endif

static inline void bdhdd_inswk_simple(uint32 port, uint16* buf, int count)
{
  int iCur;
  for(iCur=0;iCur<count;iCur++)
    buf[iCur] = BDHDD_READ16(port);
}

static inline void bdhdd_inswk_unrolled(uint32 port, uint16* buf, int count)
{
  int iBuf,iWord,iLoops,iRem;
  iBuf = 0;
  iLoops = count/16; iRem = count%16;
  for(iWord=0;iWord<iLoops;iWord++)
  {
    buf[iBuf++]=BDHDD_READ16(port);
    buf[iBuf++]=BDHDD_READ16(port);
    buf[iBuf++]=BDHDD_READ16(port);
    buf[iBuf++]=BDHDD_READ16(port);

    buf[iBuf++]=BDHDD_READ16(port);
    buf[iBuf++]=BDHDD_READ16(port);
    buf[iBuf++]=BDHDD_READ16(port);
    buf[iBuf++]=BDHDD_READ16(port);

    buf[iBuf++]=BDHDD_READ16(port);
    buf[iBuf++]=BDHDD_READ16(port);
    buf[iBuf++]=BDHDD_READ16(port);
    buf[iBuf++]=BDHDD_READ16(port);

    buf[iBuf++]=BDHDD_READ16(port);
    buf[iBuf++]=BDHDD_READ16(port);
    buf[iBuf++]=BDHDD_READ16(port);
    buf[iBuf++]=BDHDD_READ16(port);
  }
  for(iWord=0;iWord<iRem;iWord++)
    buf[iBuf++]=BDHDD_READ16(port);
}

static inline int bdhdd_altstat_waitifbitset(bdkT *bd, char bit, int *iStat, int waitCnt)
{
  char iCur, iTest;

  do{
    iCur = BDHDD_READ8(BDHDD_CNTBR_ALTSTAT);
    iTest = iCur & bit;
    waitCnt--;
  }while((iTest != 0) && waitCnt);
  *iStat = iCur;
  if(iTest)
    return -ERROR_FAILED;
  return 0;
}

static inline int bdhdd_altstat_waitifbitclear(bdkT *bd, char bit, int *iStat, int waitCnt)
{
  char iCur,iTest;

  do{
    iCur = BDHDD_READ8(BDHDD_CNTBR_ALTSTAT);
    iTest = iCur & bit;
    waitCnt--;
  }while((iTest == 0) && waitCnt);
  *iStat = iCur;
  if(iTest)
    return 0;
  return -ERROR_FAILED;
}

static inline int bdhdd_status_waitifbitset(bdkT *bd, char bit, int *iStat, int waitCnt)
{
  char iCur, iTest;

  do{
    iCur = BDHDD_READ8(BDHDD_CMDBR_STATUS);
    iTest = iCur & bit;
    waitCnt--;
  }while((iTest != 0) && waitCnt);
  *iStat = iCur;
  if(iTest)
    return -ERROR_FAILED;
  return 0;
}

static inline int bdhdd_status_waitifbitclear(bdkT *bd, char bit, int *iStat, int waitCnt)
{
  char iCur,iTest;

  do{
    iCur = BDHDD_READ8(BDHDD_CMDBR_STATUS);
    iTest = iCur & bit;
    waitCnt--;
  }while((iTest == 0) && waitCnt);
  *iStat = iCur;
  if(iTest)
    return 0;
  return -ERROR_FAILED;
}

static inline int bdhdd_dev_set(bdkT *bd, int devId, int enLBA)
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

static inline int bdhdd_devlba_set(bdkT *bd, int devId, int enLBA, int lba2427)
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

static inline int bdhdd_readyforcmd(bdkT *bd, int dev, int enLBA, int lba2427)
{
  int iStat;

  if(bdhdd_status_waitifbitset(bd,
      BDHDD_STATUS_BSYBIT | BDHDD_STATUS_DRQBIT | BDHDD_STATUS_ERRBIT,
      &iStat, BDHDD_WAIT_CMDINIT) != 0)
  {
    fprintf(stderr,"???:BDHDD:readyforcmd:Stat[0x%x]dev not OK,reseting\n",
      iStat);
    bd->reset(bd);
    if(bdhdd_status_waitifbitset(bd,
        BDHDD_STATUS_BSYBIT | BDHDD_STATUS_DRQBIT | BDHDD_STATUS_ERRBIT,
        &iStat, BDHDD_WAIT_CMDINIT) != 0)
    {
      fprintf(stderr,"ERR:BDHDD:readyforcmd:Stat[0x%x]dev still not OK\n",
        iStat);
      return -ERROR_TIMEOUT;
    }
  }
  bdhdd_devlba_set(bd,dev,enLBA,lba2427);
  if(bdhdd_status_waitifbitset(bd,BDHDD_STATUS_BSYBIT,&iStat,
      BDHDD_WAIT_CMDINIT) != 0)
  {
    fprintf(stderr,"ERR:BDHDD:not readyforcmd, suddenly BSY\n");
    return -ERROR_TIMEOUT;
  }
  if(bdhdd_status_waitifbitclear(bd,BDHDD_STATUS_DRDYBIT,&iStat,
      BDHDD_WAIT_CMDINIT) != 0)
  {
    fprintf(stderr,"ERR:BDHDD:not readyforcmd, still not DRDY\n");
    return -ERROR_TIMEOUT;
  }
  return 0;
}

static inline int bdhdd_checkstatus(bdkT *bd,uint8 cmd,int *iStatus,int *iError,int waitCnt)
{
  int ret;

  ret=bdhdd_altstat_waitifbitset(bd,BDHDD_STATUS_BSYBIT,iStatus,waitCnt);
  *iStatus = BDHDD_READ8(BDHDD_CMDBR_STATUS);
#if DEBUG_PRINT_BDHDD > 25
  printf("INFO:[status=0x%x]\n",*iStatus);
#endif
  if(ret!=0)
  {
    fprintf(stderr,"ERR:BDHDD:checkstatus:0: Cmd[0x%x] Stat[0x%x] timeout\n", 
      cmd,*iStatus);
    return -ERROR_TIMEOUT;
  }
  if(*iStatus & BDHDD_STATUS_ERRBIT)
  {
    *iError = BDHDD_READ8(BDHDD_CMDBR_ERROR);
    fprintf(stderr,"ERR:BDHDD:checkstatus:0: Cmd[0x%x] Stat[0x%x] Err[0x%x]\n",
      cmd,*iStatus,*iError);
    return -ERROR_FAILED;
  }
  return 0;
}

static inline int bdhdd_sendcmd(bdkT *bd, uint8 cmd, int *iStatus, int *iError, int waitCnt)
{
  int ret;
  
  BDHDD_WRITE8(BDHDD_CNTBR_DEVCNT,BDHDD_DEVCNT_NOINTBIT);
  BDHDD_WRITE8(BDHDD_CMDBR_COMMAND,cmd);
  if(lu_nanosleeppaka(0,BDHDD_WAITNS_DEVUPDATESSTATUS) != 0)
    fprintf(stderr,"DEBUG:BDHDD: nanosleeppaka after cmd[0x%d] failed\n",cmd);
  ret = bdhdd_checkstatus(bd,cmd,iStatus,iError,waitCnt);
  return ret;
}

void bdhdd_printsignature(bdkT *bd)
{
#if DEBUG_PRINT_BDHDD > 25
  printf("INFO:BDHDD:Signature: SECCNT[0x%x] LBALow[0x%x] LBAMid[0x%x] LBAHigh[0x%x] DevLBA[0x%x]\n",
    BDHDD_READ8(BDHDD_CMDBR_SECCNT), BDHDD_READ8(BDHDD_CMDBR_LBA0),
    BDHDD_READ8(BDHDD_CMDBR_LBA8), BDHDD_READ8(BDHDD_CMDBR_LBA16),
    BDHDD_READ8(BDHDD_CMDBR_DEVLBA24));
#endif
}

int bdhdd_reset(bdkT *bd)
{
  int ret, iStat;
  
  /* P1 Device 0 */
  BDHDD_WRITE8(BDHDD_CNTBR_DEVCNT,BDHDD_DEVCNT_SRSTBIT); /* Set reset */
  if(lu_nanosleeppaka(BDHDD_WAITSEC_SRST_P1,BDHDD_WAITNS_DEVUPDATESSTATUS) != 0)
    fprintf(stderr,"ERR:BDHDD:reset:p1: nanosleep\n");
  ret = BDHDD_READ8(BDHDD_CMDBR_ERROR);
  fprintf(stderr,"INFO:BDHDD:reset:p1: errorreg diagnostics [0x%x]\n",ret);
  /* P2 Device 1 and BSY clear*/
  BDHDD_WRITE8(BDHDD_CNTBR_DEVCNT,0); /* Clear reset */
  if(lu_nanosleeppaka(BDHDD_WAITSEC_SRST_P2,0) != 0)
    fprintf(stderr,"ERR:BDHDD:reset:p2: nanosleep\n");
  bdhdd_printsignature(bd);
  ret = BDHDD_READ8(BDHDD_CMDBR_ERROR);
  fprintf(stderr,"INFO:BDHDD:reset:p2: errorreg diagnostics [0x%x]\n",ret);
  if(ret == BDHDD_SRST_DIAGNOSTICS_ALLOK)
  {
    fprintf(stderr,"INFO:BDHDD:reset: all ok\n");
    ret = 0;
  }
  else
  {
    fprintf(stderr,"ERR:BDHDD:reset: errorreg diagnostics [0x%x]\n",ret);
    if(ret == BDHDD_SRST_DIAGNOSTICS_D0OK)
      fprintf(stderr,"INFO:BDHDD:reset: Dev0 Passed, Dev1 Failed\n");
    ret = -ERROR_FAILED;
  }
  if(bdhdd_status_waitifbitset(bd,BDHDD_STATUS_BSYBIT,&iStat,
      BDHDD_WAIT_SRSTBSY) != 0)
  {
    fprintf(stderr,"ERR:BDHDD:reset:p2: iStat[0x%x] BSY not cleared\n",iStat);
    ret = -ERROR_FAILED;
  }
  /* P3 DRDY Set */
  if(lu_nanosleeppaka(BDHDD_WAITSEC_SRST_P3,0) != 0)
    fprintf(stderr,"ERR:BDHDD:reset:p3: nanosleep\n");
  if(bdhdd_status_waitifbitclear(bd,BDHDD_STATUS_DRDYBIT,&iStat,
      BDHDD_WAIT_SRSTDRDY) != 0)
  {
    fprintf(stderr,"ERR:BDHDD:reset:p3: iStat[0x%x] DRDY not set\n",iStat);
    ret = -ERROR_FAILED;
  }
  return ret;
}

int bdhdd_init(bdkT *bd, char *secBuf, int grpId, int devId, int reset)
{
  int iStat,iError,ret;
  uint16 *buf16 = (uint16*)secBuf;
  
#ifdef BDHDD_USE_IOPERM
  if(ioperm(0x0,0x3ff,1) != 0)
  {
    fprintf(stderr,"ERR:BDHDD: Failed to get ioperm\n");
    return -ERROR_FAILED;
  }
#endif
  if(grpId == BDHDD_GRPID_PCIDESEC)
  {
    bd->CMDBR = BDHDD_IDE1_CMDBR; bd->CNTBR = BDHDD_IDE1_CNTBR;
    printf("INFO:BDHDD: PC ide1(Secondary) - devId [%d]\n", devId);
  }
  else if(grpId == BDHDD_GRPID_PCIDEPRI)
  {
    bd->CMDBR = BDHDD_IDE0_CMDBR; bd->CNTBR = BDHDD_IDE0_CNTBR;
    printf("INFO:BDHDD: PC ide0(Primary) - devId [%d] \n", devId);
  }
#ifdef PRG_MODE_DM270
  else if(grpId == BDHDD_GRPID_DM270CF_FPMC)
  {
    bd->CMDBR = PA_MEMREAD16(0x30A4C) * 0x100000;
    bd->CNTBR = bd->CMDBR + 8;
    if((bd->CMDBR!=BDHDD_DM270CF_CMDBR) || (bd->CNTBR!=BDHDD_DM270CF_CNTBR))
      fprintf(stderr,"INFO:BDHDD:init-DM270 CMDBR and or CNTBR different\n");
    fprintf(stderr,"INFO:BDHDD:DM270:FPMC CF - devId [%d] \n",devId);

    /* Enable power to CF */
    gio_dir_output(21);
    gio_bit_clear(21);
    lu_nanosleeppaka(1,0);
    /* BUSWAITMD: Bit1=0,CFRDYisCF; Bit0=1,AccessUsesCFRDY */
    PA_MEMWRITE16(0x30A26,0x1);
#ifdef BDHDD_DM270_FASTEMIF
    fprintf(stderr,"INFO:BDHDD:DM270 CF - FastEMIF mode \n");
    /**** EMIF CF Cycle time => 0x0c0d 0x0901 0x1110 ****/
    /**** EMIF CF Cycle time => 0x0708 0x0401 0x1110 ****/
    /* CS1CTRL1A: Bit12-8=0xc,ChipEnableWidth; Bit4-0=0xd,CycleWidth */
    PA_MEMWRITE16(0x30A04,0x0708);
    /* CS1CTRL1B: Bit12-8=0x09,OutputEnWidth; Bit4-0=0x01,WriteEnWidth */
    PA_MEMWRITE16(0x30A06,0x0401);
    /* CS1CTRL2: Bit13-12=0x1,Idle; Bit11-8=0x1,OutputEnSetup
                 Bit7-4=0x1,WriteEnableSetup; Bit3-0=0,ChipEnSetup */
    PA_MEMWRITE16(0x30A08,0x1110);
#endif
    /* CFCTRL1: Bit0=0,CFInterfaceActive AND SSMCinactive */
    PA_MEMWRITE16(0x30A1A,0x0);   
    /* CFCTRL2: Bit4=0,CFDynBusSzOff=16bit; Bit0=1,REG=CommonMem */
    PA_MEMWRITE16(0x30A1C,0x1);
    /* BUSINTEN: Bit1=0,InttFor0->1; Bit0=0,CFRDYDoesntGenInt */
    PA_MEMWRITE16(0x30A20,0x0);
  }
  else if(grpId == BDHDD_GRPID_DM270CF_MEMCARD3PCTLR)
  {
    bd->CMDBR = PA_MEMREAD16(0x30A4C) * 0x100000;
    bd->CNTBR = bd->CMDBR + 8;
    if((bd->CMDBR!=BDHDD_DM270CF_CMDBR) || (bd->CNTBR!=BDHDD_DM270CF_CNTBR))
      fprintf(stderr,"INFO:BDHDD:init-DM270 CMDBR and or CNTBR different\n");
    fprintf(stderr,"INFO:BDHDD:DM270:MemCARD3PCtlr CF - devId [%d] \n",devId);

    /* BUSWAITMD: Bit1=0,CFRDYisCF; Bit0=1,AccessUsesCFRDY */
    PA_MEMWRITE16(0x30A26,0x1);
#ifdef BDHDD_DM270_MEMCARD3PCTLR_FASTEMIF
    fprintf(stderr,"INFO:BDHDD:DM270 CF - FastEMIF mode \n");
    /**** EMIF CF Cycle time => 0x1415 0x0901 0x1220 ****/
    /**** EMIF CF Cycle time => 0x0c0d 0x0901 0x1110 ****/
    /**** EMIF CF Cycle time => 0x0708 0x0401 0x1110 ****/
    /**** EMIF CF Cycle time => 0x1718 0x1109 0x2442 ****/
    /* CS1CTRL1A: Bit12-8=0xc,ChipEnableWidth; Bit4-0=0xd,CycleWidth */
    PA_MEMWRITE16(0x30A04,0x1112);
    /* CS1CTRL1B: Bit12-8=0x09,OutputEnWidth; Bit4-0=0x01,WriteEnWidth */
    PA_MEMWRITE16(0x30A06,0x0e07);
    /* CS1CTRL2: Bit13-12=0x1,Idle; Bit11-8=0x1,OutputEnSetup
                 Bit7-4=0x1,WriteEnableSetup; Bit3-0=0,ChipEnSetup */
    PA_MEMWRITE16(0x30A08,0x1331);
    fprintf(stderr,"INFO:BDHDD:DM270CF - [0x%x:0x%x:0x%x]\n",
      PA_MEMREAD16(0x30A04), PA_MEMREAD16(0x30A06), PA_MEMREAD16(0x30A08));
#endif
    /* CFCTRL1: Bit0=0,CFInterfaceActive AND SSMCinactive */
    PA_MEMWRITE16(0x30A1A,0x0);   
    /* CFCTRL2: Bit4=0,CFDynBusSzOff=16bit; Bit0=1,REG=CommonMem */
    PA_MEMWRITE16(0x30A1C,0x1);
    /* BUSINTEN: Bit1=0,InttFor0->1; Bit0=0,CFRDYDoesntGenInt */
    PA_MEMWRITE16(0x30A20,0x0);
  }
#endif
  else
  {
    fprintf(stderr,"ERR:BDHDD: ide??? - unknown \n");
    return -ERROR_INVALID;
  }
  printf("INFO:BDHDD: CMDBR [0x%x] CNTBR [0x%x]\n", bd->CMDBR, bd->CNTBR);

  if(reset)
    bdhdd_reset(bd);

  if((ret=bdhdd_readyforcmd(bd,devId,BDHDD_CFG_LBA,0)) != 0) return ret;
  if(bdhdd_sendcmd(bd,BDHDD_CMD_IDENTIFYDEVICE,&iStat,&iError,
      BDHDD_WAIT_CMDTIME) != 0)
  {
    fprintf(stderr,"ERR:BDHDD: During IDENTIFYDEVICE cmd\n");
    return -ERROR_FAILED;
  }
  if((iStat&BDHDD_STATUS_DRQBIT)==0)
  {
    fprintf(stderr,"ERR:BDHDD: IDENTIFYDEVICE DRQ not set\n");
    return -ERROR_FAILED;
  }
  bdhdd_printsignature(bd);
  for(iStat=0;iStat<256;iStat++)
  {
    buf16[iStat]=BDHDD_READ16(BDHDD_CMDBR_DATA);
  }
#ifdef BDHDD_CHECK_DRQAFTERCMDCOMPLETION  
  if(bdhdd_altstat_waitifbitset(bd,BDHDD_STATUS_DRQBIT,&iStat,BDHDD_WAIT_AFTERCMDCOMPLETION) != 0)
    fprintf(stderr,"DEBUG:BDHDD:init: DRQ set even after cmd completion\n");
#endif
  
#if DEBUG_PRINT_BDHDD > 10
  printf("\n****IDENTIFYDEVICE****\n");
  printf("INFO:BDHDD:ID: numLogical Cyl[%d] Heads[%d] Secs[%d]\n", 
    buf16[1], buf16[3], buf16[6]);
  printf("INFO:BDHDD:ID: Firmware revision [%s]\n",
    buffer_read_string_noup((char*)&buf16[23],8,(char*)&buf16[160],32));
  printf("INFO:BDHDD:ID: Model number [%s]\n",
    buffer_read_string_noup((char*)&buf16[27],40,(char*)&buf16[160],64));
  printf("INFO:BDHDD:ID:51:OldPIOMode Supported [%d:0x%x]\n",
    (buf16[51]&0xff00)>>8,buf16[51]);
  printf("INFO:BDHDD:ID:53:FieldValidity AdvancedPIO [%d:0x%x]\n",
    buf16[53]&0x2,buf16[53]);
  printf("INFO:BDHDD:ID:--:APIO 64[%d] 65[%d] 66[%d] 67[%d] 68[%d]\n",
    buf16[64],buf16[65],buf16[66],buf16[67],buf16[68]);
#endif
  if(buf16[49]&0x200)
    printf("INFO:BDHDD:ID: LBA Supported, totalUserAddressableSecs[??]\n");
  else
  {
    if(BDHDD_CFG_LBA)
    {
      fprintf(stderr,"ERR:BDHDD:ID: LBA NOT Supported\n");
      return -ERROR_NOTSUPPORTED;
    }
    printf("INFO:BDHDD:ID: LBA NOT Supported\n");
  }
  if((bd->multiCnt=buf16[47]&0xff) > 0)
  {
    printf("INFO:BDHDD:ID:47: READ/WRITE MULTIPLE[0x%x]\n",buf16[47]);
#ifdef BDHDD_CFG_RWMULTIPLE
    if((ret=bdhdd_readyforcmd(bd,devId,BDHDD_CFG_LBA,0)) != 0) return ret;
    BDHDD_WRITE8(BDHDD_CMDBR_SECCNT,bd->multiCnt);
    if(bdhdd_sendcmd(bd,BDHDD_CMD_SETMULTIPLEMODE,&iStat,&iError, 
        BDHDD_WAIT_CMDTIME) != 0)
    {
      fprintf(stderr,"ERR:BDHDD: During SETMULTIPLEMODE cmd\n");
      return -ERROR_FAILED;
    }
    fprintf(stderr,"INFO:BDHDD: SETMULTIPLEMODE set\n");
#endif
  }
  else
  {
#ifdef BDHDD_CFG_RWMULTIPLE
    {
      fprintf(stderr,"ERR:BDHDD: READ/WRITE MULTIPLE not supported\n");
      return -ERROR_NOTSUPPORTED;
    }
#endif
    printf("INFO:BDHDD:ID:47: READ/WRITE MULTIPLE not Supported\n");
  }
  /* NOTE: Add SETFEATURE cmd if required later */
#ifdef BDHDD_CFG_SETFEATURES  
  if((ret=bdhdd_readyforcmd(bd,devId,BDHDD_CFG_LBA,0)) != 0) return ret;
  BDHDD_WRITE8(BDHDD_CMDBR_SECCNT,0x0b);
  BDHDD_WRITE8(BDHDD_CMDBR_FEATURES,0x03);
  if(bdhdd_sendcmd(bd,0xEF,&iStat,&iError, BDHDD_WAIT_CMDTIME) != 0)
  {
    fprintf(stderr,"ERR:BDHDD: During SETFEATURES cmd\n");
    return -ERROR_FAILED;
  }
  fprintf(stderr,"INFO:BDHDD: SETFEATURES triggered\n");
#endif  
  bd->grpId = grpId; bd->devId = devId;
  bd->secSize = BDK_SECSIZE_512;
  bd->totSecs = 0xFFFFFFFF; /* FIXME */
  return 0;
}

#endif /* BDHDD_BENCHMARK */

int bdhdd_get_sectors(bdkT *bd, long sec, long count, char*buf)
{
  int iBuf, iStat, iError, ret;
  int iLoops,iCurSecs,iLoop,iSec;
  uint16 *buf16 = (uint16*)buf;

#if (DEBUG_PRINT_BDHDD > 15)
  printf("INFO:BDHDD: sec[%ld] count[%ld]\n", sec, count);
#endif

  iBuf=0;
  iLoops = (count/BDHDD_SECCNT_USE);
  if((count%BDHDD_SECCNT_USE) != 0)
    iLoops++;
  for(iLoop=0;iLoop<iLoops;iLoop++)
  {
#if BDHDD_SECCNT_MAX == BDHDD_SECCNT_USE
    if(count < BDHDD_SECCNT_MAX)
      iCurSecs = count;
    else
      iCurSecs = 0;
#else
    if(count < BDHDD_SECCNT_USE)
      iCurSecs = count;
    else
      iCurSecs = BDHDD_SECCNT_USE;
#endif
#if DEBUG_PRINT_BDHDD > 25
    printf("INFO:BDHDD:get: curStartSec[%ld] remainingCount[%ld]\n",sec,count);
#endif
    if((ret=bdhdd_readyforcmd(bd,bd->devId,BDHDD_CFG_LBA,
              ((sec&0xf000000)>>24))) != 0) return ret;
    BDHDD_WRITE8(BDHDD_CMDBR_SECCNT,iCurSecs);
    BDHDD_WRITE8(BDHDD_CMDBR_LBA0,(sec&0xff));
    BDHDD_WRITE8(BDHDD_CMDBR_LBA8,((sec&0xff00)>>8));
    BDHDD_WRITE8(BDHDD_CMDBR_LBA16,((sec&0xff0000)>>16));
#ifdef BDHDD_CFG_RWMULTIPLE    
    if(bdhdd_sendcmd(bd,BDHDD_CMD_READMULTIPLE,&iStat,&iError,
        BDHDD_WAIT_CMDTIME) != 0)
#else			    
    if(bdhdd_sendcmd(bd,BDHDD_CMD_READSECTORS,&iStat,&iError,
        BDHDD_WAIT_CMDTIME) != 0)
#endif		    
    {
      bdhdd_printsignature(bd);
      fprintf(stderr,"ERR:BDHDD: During READSECTORS cmd\n");
      return -ERROR_FAILED;
    }
    if(iCurSecs == 0) iCurSecs = BDHDD_SECCNT_MAX;
#ifdef BDHDD_CFG_RWMULTIPLE
    for(iSec=0;iSec<iCurSecs;iSec+=bd->multiCnt)
#else
    for(iSec=0;iSec<iCurSecs;iSec++)
#endif	    
    {
      if(iSec > 0)
      {
        ret=bdhdd_checkstatus(bd,0xFF,&iStat,&iError,BDHDD_WAIT_CMDTIME);
        if(ret != 0)
        {
          fprintf(stderr,"ERR:BDHDD: middle of READSECTORS, iStat[0x%x] iLoop[%d] iSec[%d]\n",iStat,iLoop,iSec);
          return ret;
        }
      }
      if((iStat&BDHDD_STATUS_DRQBIT)==0)
      {
        fprintf(stderr,"ERR:BDHDD: READSECTORS DRQ not set, iStat[0x%x] iLoop[%d] iSec[%d]\n",iStat,iLoop,iSec);
        return -ERROR_FAILED;
      }
#ifdef BDHDD_CFG_RWMULTIPLE
      {
      int iMulti;      
      iMulti = iCurSecs - iSec;
      if(iMulti > bd->multiCnt) iMulti = bd->multiCnt;
      BDHDD_READ16S(BDHDD_CMDBR_DATA,&buf16[iBuf],256*iMulti);
      iBuf+=256*iMulti;
      }
#else
      BDHDD_READ16S(BDHDD_CMDBR_DATA,&buf16[iBuf],256);
      iBuf+=256;
#endif      
#ifdef BDHDD_BENCHMARK
      iBuf = 0;
#endif      
    }
    count-=iCurSecs;
    sec+=iCurSecs;
  }
#ifdef BDHDD_CHECK_DRQAFTERCMDCOMPLETION  
  if(bdhdd_altstat_waitifbitset(bd,BDHDD_STATUS_DRQBIT,&iStat,BDHDD_WAIT_AFTERCMDCOMPLETION) != 0)
    fprintf(stderr,"DEBUG:BDHDD:get: DRQ set even after cmd completion\n");
#endif
  return 0;
}

#ifndef BDHDD_BENCHMARK

#include "bdhdd_benchmark.c"

int bdhdd_cleanup(bdkT *bd)
{
  return 0;
}

void bdhdd_info()
{
#ifdef BDHDD_CFG_RWMULTIPLE
  fprintf(stderr,"INFO:BDHDD: CFG_RWMULTIPLE active\n");
#endif
#ifdef BDHDD_CFG_SETFEATURES
  fprintf(stderr,"INFO:BDHDD: CFG_SETFEATURES active\n");
#endif
#ifdef BDHDD_CHECK_DRQAFTERCMDCOMPLETION
  fprintf(stderr,"INFO:BDHDD: CHECK_DRQAFTERCMDCOMPLETION active\n");
#endif
  if(BDHDD_CFG_LBA)
    fprintf(stderr,"INFO:BDHDD: LBA active\n");
}

int bdhdd_setup(bdkT *bdk)
{
  bdhdd_info();
  bdk->init = bdhdd_init;
  bdk->cleanup = bdhdd_cleanup;
  bdk->reset = bdhdd_reset;
  bdk->get_sectors = bdhdd_get_sectors;
  bdk->get_sectors_benchmark = bdhdd_get_sectors_benchmark;
  pa_strncpy(bdk->name,"bdhdd",BDK_DEVNAMELEN);
  return 0;
}

#endif /* BDHDD_BENCHMARK */

