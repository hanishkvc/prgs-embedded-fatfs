/*
 * bdhdd.h - library for working with a ide HDD
 * v04Nov2004_1300
 * C Hanish Menon, 2004
 * 
 */

#ifndef _BDHDD_H_
#define _BDHDD_H_

#include <rwporta.h>
#include <bdk.h>
#include <sys/io.h>
#include <inall.h>

#define BDHDD_CFG_LBA 1
#undef BDHDD_CFG_RWMULTIPLE 
#undef BDHDD_CHECK_DRQAFTERCMDCOMPLETION
#undef BDHDD_CFG_SETFEATURES  

#ifdef PRG_MODE_DM270
#define BDHDD_USE_MEMMAPPED 1
#undef  BDHDD_USE_IOPERM 
#define BDHDD_USE_INSWK_DM270DMA 1
#define BDHDD_DM270DMA_SAFE 1
#define BDHDD_DM270_FASTEMIF 1
#else
#undef  BDHDD_USE_MEMMAPPED
#define BDHDD_USE_IOPERM 1
//#define BDHDD_USE_INSWK_SIMPLE 1
#define BDHDD_USE_INSWK_UNROLLED 1
#endif

#ifdef BDHDD_USE_INSWK_SIMPLE
#define BDHDD_INSWK bdhdd_inswk_simple
#else
#ifdef BDHDD_USE_INSWK_UNROLLED
#define BDHDD_INSWK bdhdd_inswk_unrolled
#else
#ifdef BDHDD_USE_INSWK_DM270DMA
#define BDHDD_INSWK bdhdd_inswk_dm270dma
#else
#define BDHDD_INSWK insw
#endif
#endif
#endif

#ifdef BDHDD_USE_MEMMAPPED
#define BDHDD_READ8(A) (*(volatile unsigned char*)(A))
#define BDHDD_WRITE8(A,V) (*(volatile unsigned char*)(A)=(V))
#define BDHDD_READ16(A) (*(volatile unsigned short int*)(A))
#define BDHDD_WRITE16(A,V) (*(volatile unsigned short int*)(A)=(V))
#define BDHDD_READ16S(PA,BA,C) BDHDD_INSWK(PA,BA,C)
#else
#define BDHDD_READ8(A) inb(A)
#define BDHDD_WRITE8(A,V) outb(V,A)
#define BDHDD_READ16(A) inw(A)
#define BDHDD_WRITE16(A,V) outw(V,A)
#define BDHDD_READ16S(PA,BA,C) BDHDD_INSWK(PA,BA,C)
#endif

#define BDHDD_WAIT_CMDINIT 3000000
#define BDHDD_WAITNS_DEVUPDATESSTATUS 500
#define BDHDD_WAIT_CMDTIME 6000000
#define BDHDD_WAIT_AFTERCMDCOMPLETION 10
#define BDHDD_WAITSEC_SRST_P1 16
#define BDHDD_WAITSEC_SRST_P2 31
#define BDHDD_WAIT_SRSTBSY 3000
#define BDHDD_WAITSEC_SRST_P3 (2*60)
#define BDHDD_WAIT_SRSTDRDY 3000


#define BDHDD_GRPID_PCIDEPRI 0
#define BDHDD_GRPID_PCIDESEC 1
#define BDHDD_GRPID_DM270CF 100

#define BDHDD_IDE0_CMDBR 0x1f0
#define BDHDD_IDE0_CNTBR 0x3f0
#define BDHDD_IDE1_CMDBR 0x170
#define BDHDD_IDE1_CNTBR 0x370
#define BDHDD_DM270CF_CMDBR 0x2900000
#define BDHDD_DM270CF_CNTBR 0x2900008

#define BDHDD_CMDBR_DATA (bd->CMDBR+0)
#define BDHDD_CMDBR_FEATURES (bd->CMDBR+1)
#define BDHDD_CMDBR_ERROR (bd->CMDBR+1)
#define BDHDD_CMDBR_SECCNT (bd->CMDBR+2)
#define BDHDD_CMDBR_LBA0 (bd->CMDBR+3)
#define BDHDD_CMDBR_LBA8 (bd->CMDBR+4)
#define BDHDD_CMDBR_LBA16 (bd->CMDBR+5)
#define BDHDD_CMDBR_DEVLBA24 (bd->CMDBR+6)
#define BDHDD_CMDBR_COMMAND (bd->CMDBR+7)
#define BDHDD_CMDBR_STATUS (bd->CMDBR+7)

#define BDHDD_CNTBR_DEVCNT (bd->CNTBR+6)
#define BDHDD_CNTBR_ALTSTAT (bd->CNTBR+6)

#define BDHDD_STATUS_BSYBIT 0x80
#define BDHDD_STATUS_DRDYBIT 0x40
#define BDHDD_STATUS_DRQBIT  0x08
#define BDHDD_STATUS_ERRBIT  0x01

#define BDHDD_DEV_LBABIT 0x40
#define BDHDD_DEV_DEV1BIT 0x10

#define BDHDD_DEVCNT_SRSTBIT 0x04
#define BDHDD_DEVCNT_NOINTBIT 0x02

#define BDHDD_CMD_IDENTIFYDEVICE 0xEC
#define BDHDD_CMD_READSECTORS 0x20
#define BDHDD_CMD_READSECTORSNORETRIES 0x21
#define BDHDD_CMD_READMULTIPLE 0xC4
#define BDHDD_CMD_SETMULTIPLEMODE 0xC6

#define BDHDD_SRST_DIAGNOSTICS_ALLOK 0x01
#define BDHDD_SRST_DIAGNOSTICS_D0OK 0x81

#define BDHDD_GETSECTOR_LOOPCNT 256

int bdhdd_setup(bdkT *bdk);

#endif

