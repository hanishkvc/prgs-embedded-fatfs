/*
 * dm270utils.h - library for working on dm270 
 * v07Feb2005_2159
 * C Hanish Menon <hanishkvc>, 14july2004
 * Originaly written for a earlier linux port to dm270
 */

#ifndef _DM270UTILS_H_
#define _DM270UTILS_H_

#include <bdk.h>
#include <bdhdd.h>
#include <bdh8b16.h>

#define ONE_MB	0x100000
/* GIO Register Define */
#define DM270_DIR0			0x30580
#define DM270_DIR1			0x30582
#define DM270_DIR2			0x30584
#define DM270_INV0			0x30586
#define DM270_INV1			0x30588
#define DM270_INV2			0x3058A
#define DM270_BITSET0			0x3058C
#define DM270_BITSET1			0x3058E
#define DM270_BITSET2			0x30590
#define DM270_BITCLR0			0x30592
#define DM270_BITCLR1			0x30594
#define DM270_BITCLR2			0x30596
#define DM270_IRQPORT			0x30598
#define DM270_IRQEDGE			0x3059A
#define DM270_CHAT0			0x3059C
#define DM270_CHAT1			0x3059E
#define DM270_CHAT2			0x305A0
#define DM270_NCHAT			0x305A2
#define DM270_FSEL0			0x305A4
#define DM270_FSEL1			0x305A6

/* SDRAM Controller Register Define */
#define DM270_SDBUF_D0L		0x30980
#define DM270_SDBUF_D0H		0x30982
#define DM270_SDBUF_D1L		0x30984
#define DM270_SDBUF_D1H		0x30986
#define DM270_SDBUF_D2L		0x30988
#define DM270_SDBUF_D2H		0x3098A
#define DM270_SDBUF_D3L		0x3098C
#define DM270_SDBUF_D3H		0x3098E
#define DM270_SDBUF_D4L		0x30990
#define DM270_SDBUF_D4H		0x30992
#define DM270_SDBUF_D5L		0x30994
#define DM270_SDBUF_D5H		0x30996
#define DM270_SDBUF_D6L		0x30998
#define DM270_SDBUF_D6H		0x3099A
#define DM270_SDBUF_D7L		0x3099C
#define DM270_SDBUF_D7H		0x3099E
#define DM270_SDBUF_AD1		0x309A0
#define DM270_SDBUF_AD2		0x309A2
#define DM270_SDBUF_CTL		0x309A4
#define DM270_SDMODE			0x309A6
#define DM270_REFCTL			0x309A8
#define DM270_SDPRTY1			0x309AA
#define DM270_SDPRTY2			0x309AC
#define DM270_SDPRTY3			0x309AE
#define DM270_SDPRTY4			0x309B0
#define DM270_SDPRTY5			0x309B2
#define DM270_SDPRTY6			0x309B4
#define DM270_SDPRTY7			0x309B6
#define DM270_SDPRTY8			0x309B8
#define DM270_SDPRTY9			0x309BA
#define DM270_SDPRTY10			0x309BC
#define DM270_PRTYON			0x309BE
#define DM270_PRTYON_TEST		0x309C0


/* External Memory Interface Register Define */
#define DM270_CS1CTRL1A			0x30A04
#define DM270_CS1CTRL1B			0x30A06
#define DM270_CS1CTRL2			0x30A08
#define DM270_CFCTRL1			0x30A1A
#define DM270_CFCTRL2			0x30A1C

#define DM270_CS0CTRL1			0x30A00
#define DM270_CS3CTRL1			0x30A0E
#define DM270_CS4CTRL1			0x30A12
#define DM270_CS2CTRL1			0x30A0A
#define DM270_CS0CTRL2			0x30A02
#define DM270_CS2CTRL2			0x30A0C
#define DM270_CS3CTRL2			0x30A10
#define DM270_CS4CTRL2			0x30A14
#define DM270_BUSCTRL			0x30A16
#define DM270_BUSRLS			0x30A18
#define DM270_SMCTRL			0x30A1E
#define DM270_BUSINTEN			0x30A20
#define DM270_BUSINTSTS			0x30A22
#define DM270_BUSSTS			0x30A24
#define DM270_BUSWAITMD			0x30A26
#define DM270_ECC1CP			0x30A28
#define DM270_ECC1LP			0x30A2A
#define DM270_ECC2CP			0x30A2C
#define DM270_ECC2LP			0x30A2E
#define DM270_ECCCLR			0x30A30
#define DM270_PAGESZ			0x30A32
#define DM270_VIFCDSPDEST		0x30A34
#define DM270_PRIORCTL			0x30A36
#define DM270_SOURCEADDH		0x30A38
#define DM270_SOURCEADDL		0x30A3A
#define DM270_DESTADDH			0x30A3C
#define DM270_DESTADDL			0x30A3E
#define DM270_DMASIZE			0x30A40
#define DM270_DMADEVSEL			0x30A42
#define DM270_DMACTL			0x30A44
#define DM270_VIFCDSPADD1		0x30A46
#define DM270_VIFCDSPADD2		0x30A48
#define DM270_DPSTR0			0x30A4A
#define DM270_DPSTR1			0x30A4C
#define DM270_DPSTR2			0x30A4E
#define DM270_DPSTR3			0x30A50
#define DM270_DPSTR4			0x30A52
#define DM270_DPSTR5			0x30A54
#define DM270_EMIF_TEST			0x30A56

#define DM270_SDRAM_BASE		((PA_MEMREAD16(DM270_DPSTR0))*(ONE_MB))
#define DM270_EXMEM1_BASE		((PA_MEMREAD16(DM270_DPSTR1))*(ONE_MB))

static inline void gio_dir_input(unsigned short int nGIO)
{
  if(nGIO > 31)
    PA_MEMWRITE16(DM270_DIR2,PA_MEMREAD16(DM270_DIR2)|(1 << (nGIO - 32)));
  else if(nGIO > 15)
    PA_MEMWRITE16(DM270_DIR1,PA_MEMREAD16(DM270_DIR1)|(1 << (nGIO - 16)));
  else
    PA_MEMWRITE16(DM270_DIR0,PA_MEMREAD16(DM270_DIR0)|(1 << nGIO));
}

static inline void gio_dir_output(unsigned int nGIO)
{
  if(nGIO > 31)
    PA_MEMWRITE16(DM270_DIR2,PA_MEMREAD16(DM270_DIR2) & (~(1 << (nGIO - 32))));
  else if(nGIO > 15)
    PA_MEMWRITE16(DM270_DIR1,PA_MEMREAD16(DM270_DIR1) & (~(1 << (nGIO - 16))));
  else 
    PA_MEMWRITE16(DM270_DIR0,PA_MEMREAD16(DM270_DIR0) & (~(1 << nGIO)));
}

static inline void gio_bit_clear(int nGIO)
{
  if(nGIO > 31)
    PA_MEMWRITE16(DM270_BITCLR2,(1 << (nGIO - 32)));
  else if(nGIO > 15)
    PA_MEMWRITE16(DM270_BITCLR1,(1 << (nGIO - 16)));
  else
    PA_MEMWRITE16(DM270_BITCLR0,(1 << nGIO));
}

static inline void gio_bit_set(int nGIO)
{
  if(nGIO > 31)
    PA_MEMWRITE16(DM270_BITSET2,(1 << (nGIO - 32)));
  else if(nGIO > 15)
    PA_MEMWRITE16(DM270_BITSET1,(1 << (nGIO - 16)));
  else
    PA_MEMWRITE16(DM270_BITSET0,(1 << nGIO));
}

static inline int gio_bit_isset(int nGIO)
{
  if (nGIO > 32)
    return ((PA_MEMREAD16(DM270_BITSET2) & (0x1 << (nGIO - 32))) != 0x0);
  else if (nGIO > 15)
    return ((PA_MEMREAD16(DM270_BITSET1) & (0x1 << (nGIO - 16))) != 0x0);
  else
    return ((PA_MEMREAD16(DM270_BITSET0) & (0x1 << nGIO)) != 0x0);
}

inline void bdh8b16_inswk_dm270dma(uint32 port, uint16* buf, int count);
inline void bdh8b16_outswk_dm270dma(uint32 port, uint16* buf, int count);
int bdh8b16_init_grpid_dm270ide_h8b16(bdkT *bd, int grpId, int devId, int bdkFlags);

inline void bdhdd_inswk_dm270dma(uint32 port, uint16* buf, int count);
inline void bdhdd_outswk_dm270dma(uint32 port, uint16* buf, int count);
int bdhdd_init_grpid_dm270cf_fpmc(bdkT *bd, int grpId, int devId, int bdkFlags);
int bdhdd_init_grpid_dm270cf_MemCARD3PCtlr(bdkT *bd, int grpId, int devId, int bdkFlags);

#endif


