/*
 * bdh8b16.h - library for working with a ide HDD
 * v14Mar2005_1448
 * C Hanish Menon, 2004
 * 
 */

#ifndef _BDH8B16_H_
#define _BDH8B16_H_

#include <rwporta.h>
#include <bdk.h>
#include <sys/io.h>
#include <inall.h>

#define BDH8B16_CFG_LBA 1
#define BDH8B16_CFG_RWMULTIPLE 
#define BDH8B16_CHECK_DRQAFTERCMDCOMPLETION 1
#undef BDH8B16_CFG_SETFEATURES  

#ifdef PRG_MODE_DM270
#define BDH8B16_USE_MEMMAPPED 1
#undef  BDH8B16_USE_IOPERM 
#define BDH8B16_USE_INSWK_DM270DMA 1
#undef  BDH8B16_USE_INSWK_SIMPLE
#define BDH8B16_DM270DMA_SAFE 1
#define BDH8B16_DM270_EMIF16 1
#define BDH8B16_DM270_FASTEMIF 1
#else
#define BDH8B16_USE_MEMMAPPED 1
#undef  BDH8B16_USE_IOPERM 
#define BDH8B16_USE_INSWK_SIMPLE 1
#warning "FIXME: Partially SUPPORTED"
#endif

#ifdef BDH8B16_USE_INSWK_SIMPLE
#define BDH8B16_INSWK bdh8b16_inswk_simple
#define BDH8B16_OUTSWK bdh8b16_outswk_simple
#warning "INFO: Using in/outswk_simple"
#else
#ifdef BDH8B16_USE_INSWK_UNROLLED
#define BDH8B16_INSWK bdh8b16_inswk_unrolled
#define BDH8B16_OUTSWK bdh8b16_outswk_simple
#warning "INFO: Using in/outswk_unrolled"
#else
#ifdef BDH8B16_USE_INSWK_DM270DMA
#define BDH8B16_INSWK bdh8b16_inswk_dm270dma
#define BDH8B16_OUTSWK bdh8b16_outswk_dm270dma
#warning "INFO: Using in/outswk_dm270dma"
#else
#define BDH8B16_INSWK insw
#endif
#endif
#endif

#define PA_MEMREAD8(A) (*(volatile unsigned char*)(A))
#define PA_MEMWRITE8(A,V) (*(volatile unsigned char*)(A)=(V))
#define PA_MEMREAD16(A) (*(volatile unsigned short int*)(A))
#define PA_MEMWRITE16(A,V) (*(volatile unsigned short int*)(A)=(V))

#ifdef BDH8B16_USE_MEMMAPPED
#define BDH8B16_READ8(A)     PA_MEMREAD8(A)
#define BDH8B16_WRITE8(A,V)  PA_MEMWRITE8(A,V)
#define BDH8B16_READ16(A)    PA_MEMREAD16(A)
#define BDH8B16_WRITE16(A,V) PA_MEMWRITE16(A,V)
#define BDH8B16_READ16S(PA,BA,C) BDH8B16_INSWK(PA,BA,C)
#define BDH8B16_WRITE16S(PA,BA,C) BDH8B16_OUTSWK(PA,BA,C)
#else
#warning "FIXME: NOT SUPPORTED"
#endif

#define BDH8B16_WAIT_CMDINIT 0x1000000
#define BDH8B16_WAITNS_DEVUPDATESSTATUS 4000
#define BDH8B16_WAIT_CMDTIME 0xf000000
#define BDH8B16_WAIT_AFTERCMDCOMPLETION 0x1000
#define BDH8B16_WAITSEC_SRST_P1 16
#define BDH8B16_WAITSEC_SRST_P2 31
#define BDH8B16_WAIT_SRSTBSY 0x9000
#define BDH8B16_WAITSEC_SRST_P3 (2*60)
#define BDH8B16_WAIT_SRSTDRDY 0x9000
#define BDH8B16_WAITNS_ALTSTATCHECKSET 1000
#define BDH8B16_WAIT_CHECKSTATUS_DRQ 1000
#define BDH8B16_WAITNS_CHECKSTATUS_DRQ 10000000


#define BDH8B16_GRPID_DM270IDE_H8B16 102

#define BDH8B16_CMDBR_OFFSET 0x20
#define BDH8B16_CNTBR_OFFSET 0x10

#define BDH8B16_CMDBR_DATA (bd->CMDBR+0)
#define BDH8B16_CMDBR_FEATURES (bd->CMDBR+2)
#define BDH8B16_CMDBR_ERROR (bd->CMDBR+2)
#define BDH8B16_CMDBR_SECCNT (bd->CMDBR+4)
#define BDH8B16_CMDBR_LBA0 (bd->CMDBR+6)
#define BDH8B16_CMDBR_LBA8 (bd->CMDBR+8)
#define BDH8B16_CMDBR_LBA16 (bd->CMDBR+0xa)
#define BDH8B16_CMDBR_DEVLBA24 (bd->CMDBR+0xc)
#define BDH8B16_CMDBR_COMMAND (bd->CMDBR+0xe)
#define BDH8B16_CMDBR_STATUS (bd->CMDBR+0xe)

#define BDH8B16_CNTBR_DEVCNT (bd->CNTBR+0xc)
#define BDH8B16_CNTBR_ALTSTAT (bd->CNTBR+0xc)

#define BDH8B16_STATUS_BSYBIT 0x80
#define BDH8B16_STATUS_DRDYBIT 0x40
#define BDH8B16_STATUS_DRQBIT  0x08
#define BDH8B16_STATUS_ERRBIT  0x01

#define BDH8B16_DEV_LBABIT 0x40
#define BDH8B16_DEV_DEV1BIT 0x10

#define BDH8B16_DEVCNT_SRSTBIT 0x04
#define BDH8B16_DEVCNT_NOINTBIT 0x02

#define BDH8B16_CMD_IDENTIFYDEVICE 0xEC
#define BDH8B16_CMD_READSECTORS 0x20
#define BDH8B16_CMD_READSECTORSNORETRIES 0x21
#define BDH8B16_CMD_READMULTIPLE 0xC4
#define BDH8B16_CMD_SETMULTIPLEMODE 0xC6
#define BDH8B16_CMD_WRITESECTORS 0x30
#define BDH8B16_CMD_WRITESECTORSNORETRIES 0x31
#define BDH8B16_CMD_WRITEMULTIPLE 0xC5

#define BDH8B16_SRST_DIAGNOSTICS_ALLOK 0x01
#define BDH8B16_SRST_DIAGNOSTICS_D0OK 0x81

#define BDH8B16_ERRREPCNT 6

#define BDH8B16_GETSECTOR_LOOPCNT 256

#define BDH8B16_SECCNT_MAX 256
#define BDH8B16_SECCNT_USE 256

int bdh8b16_get_sectors_single(bdkT *bd, long sec, long count, char *buf);
int bdh8b16_get_sectors(bdkT *bd, long sec, long count, char *buf);
int bdh8b16_put_sectors_single(bdkT *bd, long sec, long count, char *buf);

#endif

