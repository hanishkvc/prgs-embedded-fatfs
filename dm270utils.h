/*
 * dm270utils.h - library for working on dm270 
 * v03Nov2004_1343
 * C Hanish Menon <hanishkvc>, 14july2004
 * Originaly written for a earlier linux port to dm270
 */

#ifndef _DM270UTILS_H_
#define _DM270UTILS_H_

/* GIO Register Define */
#define	DIR0			0x30580
#define	DIR1			0x30582
#define	DIR2			0x30584
#define	INV0			0x30586
#define	INV1			0x30588
#define	INV2			0x3058A
#define	BITSET0			0x3058C
#define	BITSET1			0x3058E
#define	BITSET2			0x30590
#define	BITCLR0			0x30592
#define	BITCLR1			0x30594
#define	BITCLR2			0x30596
#define	IRQPORT			0x30598
#define	IRQEDGE			0x3059A
#define	CHAT0			0x3059C
#define	CHAT1			0x3059E
#define	CHAT2			0x305A0
#define	NCHAT			0x305A2
#define	FSEL0			0x305A4
#define	FSEL1			0x305A6

/* SDRAM Controller Register Define */
#define	SDBUF_D0L		0x30980
#define SDBUF_D0H		0x30982
#define SDBUF_D1L		0x30984
#define SDBUF_D1H		0x30986
#define SDBUF_D2L		0x30988
#define SDBUF_D2H		0x3098A
#define SDBUF_D3L		0x3098C
#define SDBUF_D3H		0x3098E
#define SDBUF_D4L		0x30990
#define SDBUF_D4H		0x30992
#define SDBUF_D5L		0x30994
#define SDBUF_D5H		0x30996
#define SDBUF_D6L		0x30998
#define SDBUF_D6H		0x3099A
#define SDBUF_D7L		0x3099C
#define SDBUF_D7H		0x3099E
#define SDBUF_AD1		0x309A0
#define SDBUF_AD2		0x309A2
#define SDBUF_CTL		0x309A4
#define SDMODE			0x309A6
#define REFCTL			0x309A8
#define SDPRTY1			0x309AA
#define SDPRTY2			0x309AC
#define SDPRTY3			0x309AE
#define SDPRTY4			0x309B0
#define SDPRTY5			0x309B2
#define SDPRTY6			0x309B4
#define SDPRTY7			0x309B6
#define SDPRTY8			0x309B8
#define	SDPRTY9			0x309BA
#define	SDPRTY10		0x309BC
#define PRTYON			0x309BE
#define PRTYON_TEST		0x309C0


/* External Memory Interface Register Define */
#define	CS1CTRL1A		0x30A04
#define	CS1CTRL1B		0x30A06
#define	CS1CTRL2		0x30A08
#define	CFCTRL1			0x30A1A
#define	CFCTRL2			0x30A1C

#define	CS0CTRL1		0x30A00
#define	CS3CTRL1		0x30A0E
#define	CS4CTRL1		0x30A12
#define	CS2CTRL1		0x30A0A
#define	CS0CTRL2		0x30A02
#define	CS2CTRL2		0x30A0C
#define	CS3CTRL2		0x30A10
#define	CS4CTRL2		0x30A14
#define	BUSCTRL			0x30A16
#define	BUSRLS			0x30A18
#define	SMCTRL			0x30A1E
#define	BUSINTEN		0x30A20
#define	BUSINTSTS		0x30A22
#define	BUSSTS			0x30A24
#define	BUSWAITMD		0x30A26
#define	ECC1CP			0x30A28
#define	ECC1LP			0x30A2A
#define	ECC2CP			0x30A2C
#define	ECC2LP			0x30A2E
#define	ECCCLR			0x30A30
#define	PAGESZ			0x30A32
#define	VIFCDSPDEST		0x30A34
#define	PRIORCTL		0x30A36
#define	SURCEADD1		0x30A38
#define	SURCEADD2		0x30A3A
#define	DESTADD1		0x30A3C
#define	DESTADD2		0x30A3E
#define	DMASIZE			0x30A40
#define	DMADEVSEL		0x30A42
#define	EMIFDMACTL		0x30A44
#define	VIFCDSPADD1		0x30A46
#define	VIFCDSPADD2		0x30A48
#define	DPSTR0			0x30A4A
#define	DPSTR1			0x30A4C
#define	DPSTR2			0x30A4E
#define	DPSTR3			0x30A50
#define	DPSTR4			0x30A52
#define	DPSTR5			0x30A54
#define	EMIF_TEST		0x30A56

#define SDRAM_BASE			    (inw(DPSTR0))*(ONE_MB)
#define EXMEM1_BASE			    (inw(DPSTR1))*(ONE_MB)

static inline void gio_dir_input(int nGIO)
{
  if(nGIO > 31)
    outw(inw((unsigned int)DIR2)|(1 << (nGIO - 32)),DIR2);
  else if(nGIO > 15)
    outw(inw((unsigned int)DIR1)|(1 << (nGIO - 16)),DIR1);
  else
   outw(inw((unsigned int) DIR0)|(1 << nGIO),DIR0);
}

static inline void gio_dir_output(int nGIO)
{
  if(nGIO > 31)
    outw(inw((unsigned int )DIR2) & (~(1 << (nGIO - 32))),DIR2);
  else if(nGIO > 15)
    outw(inw((unsigned int )DIR1) & (~(1 << (nGIO - 16))),DIR1);
  else 
    outw(inw((unsigned int )DIR0)& (~(1 << nGIO)),DIR0);
}

static inline void gio_bit_clear(int nGIO)
{
  if(nGIO > 31)
    outw((1 << (nGIO - 32)),BITCLR2);
  else if(nGIO > 15)
    outw((1 << (nGIO - 16)),BITCLR1);
  else
    outw((1 << nGIO),BITCLR0);
}

static inline void gio_bit_set(int nGIO)
{
  if(nGIO > 31)
    outw((1 << (nGIO - 32)),BITSET2);
  else if(nGIO > 15)
    outw((1 << (nGIO - 16)),BITSET1);
  else
    outw((1 << nGIO),BITSET0);
}

static inline int gio_bit_isset(int nGIO)
{
  if (nGIO > 32)
    return ((inw((unsigned int)BITSET2) & (0x0001 << (nGIO - 32))) != 0x0000);
  else if (nGIO > 15)
    return ((inw((unsigned int)BITSET1) & (0x0001 << (nGIO - 16))) != 0x0000);
  else
    return ((inw((unsigned int)BITSET0) & (0x0001 << nGIO)) != 0x0000);
}

#endif



