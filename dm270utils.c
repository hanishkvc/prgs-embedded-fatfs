/*
 * dm270utils.c - library for working on dm270 
 * v07Feb2005_2245
 * C Hanish Menon <hanishkvc>, 14july2004
 */

/*
 * Issues in FPMC_DM270 board:
 * * IOIS16 most probably is a output from Device to Host to tell its doing
 *   a 16bit transfer or not and Not a input to Device as specified in FPMC
 *   by connecting A5 to IOIS16 of Device.
 * * In 16bit mode Addr on the Addr bus is shifted by 1bit, which doesn't
 *   seem to be accounted for. 
 *   * So if we set the BUS to 8bit then again as doing 16bit access on 8bit 
 *   would require use of EM_BEH and EM_BEL from DM270, which is not currently
 *   done, so we CANNT SET BUS TO 8Bit.
 *   * So if we set the BUS to 16bit then again as 1bit shift is not accounted
 *   we will have to use addresses which are shifted that is:
 *   Ata Addr 1 will be Addr 2
 *   Ata Addr 2 will be Addr 4
 *   Ata Addr 7 will be Addr 14
 * * Also as nATA_CS0 and nATA_CS1 are active low signals, in FPMC the 
 *   CNTBR registers come before CMDBR registers
 * Note regarding DM270
 * * CS3CTRL2 as bit to tell if EM_CS3 region is 8bit or 16bit which
 *   on reset is 8bit.
 * * BUSCTRL as EMWE3 to enable checking of nEM_WAIT signal for EM_CS3 region
 */

#include <linuxutils.h>
#include <dm270utils.h>
#include <errs.h>

uint32 gBDH8B16RegionBase;

#ifdef BDH8B16_USE_INSWK_DM270DMA

inline void bdh8b16_inswk_dm270dma(uint32 port, uint16* buf, int count)
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
  /* DMADEVSEL: Bit6-4 = 3,DMASrcIsIDEEMIF; Bit2-0 = 5,DMADestIsSDRam */
  PA_MEMWRITE16(DM270_DMADEVSEL,0x35);
  /* DMASIZE: */
  PA_MEMWRITE16(DM270_DMASIZE,count*2);
  /* SOURCEADDH: Bit12:=1,FixedAddr, Bit9-0: AddrHigh Relative to CF Region */
  port -= gBDH8B16RegionBase;
  if(port > 0x40) /* Not DM270 IDE */
  {
    fprintf(stderr,"ERR:BDH8B16:DM270DMA doesn't support nonIDE now, quiting\n");
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

inline void bdh8b16_outswk_dm270dma(uint32 port, uint16* buf, int count)
{

#ifdef BDH8B16_DM270DMA_SAFE
  int iBuf;
  /* REFCTL: Bit15-13 = 0,DMAChannel1BetweenEmifAndSDRam */
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
  /* DMADEVSEL: Bit6-4 = 5,DMASrcIsSDRam; Bit2-0 = 3,DMADestIsIDEEMIF */
  PA_MEMWRITE16(DM270_DMADEVSEL,0x53);
  /* DMASIZE: */
  PA_MEMWRITE16(DM270_DMASIZE,count*2);
  /* DESTADDH: Bit12:=1,FixedAddr, Bit9-0: AddrHigh Relative to CF Region */
  port -= gBDH8B16RegionBase;
  if(port > 0x40) /* Not DM270 IDE */
  {
    fprintf(stderr,"ERR:BDH8B16:DM270DMA doesn't support nonIDE now, quiting\n");
    exit(-ERROR_INVALID);
  }
  PA_MEMWRITE16(DM270_DESTADDH,0x1000|((port>>16)&0x3ff));
  PA_MEMWRITE16(DM270_DESTADDL,port&0xffff);
  /* Source high 10bits and low 16bits */
  (int32)buf -= DM270_SDRAM_BASE; /* int as addr !> 2GB */
  PA_MEMWRITE16(DM270_SOURCEADDH,((uint32)buf>>16)&0x3ff);
  PA_MEMWRITE16(DM270_SOURCEADDL,(uint32)buf&0xffff); 
#if DEBUG_PRINT_BDH8B16 > 5000
  fprintf(stderr,"INFO:BDH8B16:DM270DMA:Src[0x%lx] Dest[%ld] count [%d]\n",
    (uint32)buf,port,count);
#endif
  /* DMACTL: Bit0 = 1,Start DMA */
  PA_MEMWRITE16(DM270_DMACTL,0x1);
  while(PA_MEMREAD16(DM270_DMACTL)); /* Inefficient CPU Use but for now */
}

#endif

int bdh8b16_init_grpid_dm270ide_h8b16(bdkT *bd, int grpId, int devId)
{
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
    iEmif |= 0x4001;
    fprintf(stderr,"INFO:BDH8B16:DM270:IDE setting EMIF16 [0x%x]\n", iEmif);
    PA_MEMWRITE16(0x30A10,iEmif);
    }
#endif
#ifdef BDH8B16_DM270_FASTEMIF
    {
    uint16 iEmif;
    iEmif = 0x2234;
    fprintf(stderr,"INFO:BDH8B16:DM270:IDE setting EMIFFAST [0x%x]\n", iEmif);
    PA_MEMWRITE16(0x30A0E,iEmif);
    }
#endif
    return 0;
  }
  return -1;
}

#ifdef BDHDD_USE_INSWK_DM270DMA

inline void bdhdd_inswk_dm270dma(uint32 port, uint16* buf, int count)
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

inline void bdhdd_outswk_dm270dma(uint32 port, uint16* buf, int count)
{

#ifdef BDHDD_DM270DMA_SAFE
  int iBuf;
  /* REFCTL: Bit15-13 = 0,DMAChannel1BetweenEmifAndSDRam */
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
  /* DMADEVSEL: Bit6-4 = 5,DMASrcIsSDRam; Bit2-0 = 1,DMADestIsCF */
  PA_MEMWRITE16(DM270_DMADEVSEL,0x51);
  /* DMASIZE: */
  PA_MEMWRITE16(DM270_DMASIZE,count*2);
  /* DESTADDH: Bit12:=1,FixedAddr, Bit9-0: AddrHigh Relative to CF Region */
  port -= BDHDD_DM270CF_CMDBR;
  if(port > 16) /* Not DM270 CF, Maybe later add DM270 IDE support */
  {
    fprintf(stderr,"ERR:BDHDD:DM270DMA doesn't support nonCF now, quiting\n");
    exit(-ERROR_INVALID);
  }
  PA_MEMWRITE16(DM270_DESTADDH,0x1000|((port>>16)&0x3ff));
  PA_MEMWRITE16(DM270_DESTADDL,port&0xffff);
  /* Source high 10bits and low 16bits */
  (int32)buf -= DM270_SDRAM_BASE; /* int as addr !> 2GB */
  PA_MEMWRITE16(DM270_SOURCEADDH,((uint32)buf>>16)&0x3ff);
  PA_MEMWRITE16(DM270_SOURCEADDL,(uint32)buf&0xffff); 
#if DEBUG_PRINT_BDHDD > 5000
  fprintf(stderr,"INFO:BDHDD:DM270DMA:Src[0x%lx] Dest[%ld] count [%d]\n",
    (uint32)buf,port,count);
#endif
  /* DMACTL: Bit0 = 1,Start DMA */
  PA_MEMWRITE16(DM270_DMACTL,0x1);
  while(PA_MEMREAD16(DM270_DMACTL)); /* Inefficient CPU Use but for now */
}

#endif

int bdhdd_init_grpid_dm270cf_fpmc(bdkT *bd, int grpId, int devId)
{
  if(grpId == BDHDD_GRPID_DM270CF_FPMC)
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
    return 0;
  }
  return -ERROR_INVALID;
}

int bdhdd_init_grpid_dm270cf_MemCARD3PCtlr(bdkT *bd, int grpId, int devId)
{
  if(grpId == BDHDD_GRPID_DM270CF_MEMCARD3PCTLR)
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
    return 0;
  }
  return -ERROR_INVALID;
}

