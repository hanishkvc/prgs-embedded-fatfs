/*
 * bdhdd.h - library for working with a ide HDD
 * v10Oct2004_0025
 * C Hanish Menon, 2004
 * 
 */

#ifndef _BDHDD_H_
#define _BDHDD_H_

#include <rwporta.h>
#include <bdk.h>
#include <time.h>
#include <sys/io.h>

#ifdef BDHDD_MEMMAPPED
#define BDHDD_READ8(A) (*(volatile char*)A)
#define BDHDD_WRITE8(A,V) *(volatile char*)A=V
#define BDHDD_READ16(A) (*(volatile int*)A)
#define BDHDD_WRITE16(A,V) *(volatile int*)A=V
#else
#define BDHDD_READ8(A) inb(A)
#define BDHDD_WRITE8(A,V) outb(V,A)
#define BDHDD_READ16(A) inw(A)
#define BDHDD_WRITE16(A,V) outw(V,A)
#endif

#define BDHDD_CMDBR 0x170
#define BDHDD_CMDBR_DATA (BDHDD_CMDBR+0)
#define BDHDD_CMDBR_FEATURES (BDHDD_CMDBR+1)
#define BDHDD_CMDBR_ERROR (BDHDD_CMDBR+1)
#define BDHDD_CMDBR_SECCNT (BDHDD_CMDBR+2)
#define BDHDD_CMDBR_LBA0 (BDHDD_CMDBR+3)
#define BDHDD_CMDBR_LBA8 (BDHDD_CMDBR+4)
#define BDHDD_CMDBR_LBA16 (BDHDD_CMDBR+5)
#define BDHDD_CMDBR_DEVLBA24 (BDHDD_CMDBR+6)
#define BDHDD_CMDBR_COMMAND  (BDHDD_CMDBR+7)
#define BDHDD_CMDBR_STATUS (BDHDD_CMDBR+7)

#define BDHDD_CNTBR 0x370
#define BDHDD_CNTBR_DEVCNT (BDHDD_CNTBR+6)
#define BDHDD_CNTBR_ALTSTAT (BDHDD_CNTBR+6)

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
#define BDHDD_CMD_READSECTORSWITHOUTRETRIES 0x21

bdkT bdkHdd;

int bdhdd_setup();

#endif

