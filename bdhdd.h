/*
 * bdhdd.h - library for working with a ide HDD
 * v12Oct2004_1103
 * C Hanish Menon, 2004
 * 
 */

#ifndef _BDHDD_H_
#define _BDHDD_H_

#include <rwporta.h>
#include <bdk.h>
#include <sys/io.h>

#define BDHDD_CFG_LBA 1
#define BDHDD_CFG_RWMULTIPLE 1
#undef BDHDD_CHECK_DRQAFTERCMDCOMPLETION
#undef BDHDD_MEMMAPPED

#ifdef BDHDD_MEMMAPPED
#define BDHDD_READ8(A) (*(volatile char*)(A))
#define BDHDD_WRITE8(A,V) *(volatile char*)(A)=V
#define BDHDD_READ16(A) (*(volatile int*)(A))
#define BDHDD_WRITE16(A,V) *(volatile int*)(A)=V
#else
#define BDHDD_READ8(A) inb(A)
#define BDHDD_WRITE8(A,V) outb(V,A)
#define BDHDD_READ16(A) inw(A)
#define BDHDD_WRITE16(A,V) outw(V,A)
#define BDHDD_READ16S(PA,BA,C) insw(PA,BA,C)
#endif

#define BDHDD_WAIT_CMDINIT 3000000
#define BDHDD_WAITNS_DEVUPDATESSTATUS 1000
#define BDHDD_WAIT_CMDTIME 6000000
#define BDHDD_WAIT_AFTERCMDCOMPLETION 10
#define BDHDD_WAITSEC_SRST_P1 16
#define BDHDD_WAITSEC_SRST_P2 31
#define BDHDD_WAIT_SRSTBSY 3000
#define BDHDD_WAITSEC_SRST_P3 (2*60)
#define BDHDD_WAIT_SRSTDRDY 3000


#define BDHDD_IDE0_CMDBR 0x1f0
#define BDHDD_IDE0_CNTBR 0x3f0
#define BDHDD_IDE1_CMDBR 0x170
#define BDHDD_IDE1_CNTBR 0x370
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

bdkT bdkHdd;

int bdhdd_setup();

#endif

