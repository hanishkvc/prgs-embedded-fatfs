/*
 * fatfs.h - library for working with fat filesystem
 * v14july2004
 * C Hanish Menon, 2004
 * 
 */

#ifndef _FATFS_H_
#define _FATFS_H_

#include <rwporta.h>

#ifndef DEBUG_PRINT_FATFS
#define DEBUG_PRINT_FATFS 11
#endif

#define FATBOOTSEC_MAXSIZE 512
//#define FATFAT_MAXSIZE (1024*1024)
#define FATFAT_MAXSIZE (2048*1024)
#define FATROOTDIR_MAXSIZE (512*1024)
#define FATDIRENTRY_SIZE 32
#define FATDIRENTRYNAME_SIZE 11
#define FILEINFONAME_SIZE 12
#define FILEINFOLFN_SIZE 256
#define FATDIRENTRYLFN_PARTIALCHARS 13

#define FATATTR_READONLY 0x01
#define FATATTR_HIDDEN   0x02
#define FATATTR_SYSTEM   0x04
#define FATATTR_VOLUMEID 0x08
#define FATATTR_DIR      0x10
#define FATATTR_ARCHIVE  0x20
#define FATATTR_LONGNAME 0x0F

#define BS_OEMName_sz8(base) (char*)((long)base+3)
#define BPB_BytPerSec(base) (unsigned short*)((long)base+11)
#define BPB_SecPerClus(base) (unsigned char*)((long)base+13)
#define BPB_RsvdSecCnt(base) (unsigned short*)((long)base+14)
#define BPB_NumFats(base) (unsigned char*)((long)base+16)
#define BPB_RootEntCnt(base) (unsigned short*)((long)base+17)
#define BPB_TotSec16(base) (unsigned short*)((long)base+19)
#define BPB_Media(base) (unsigned char*)((long)base+21)
#define BPB_FatSz16(base) (unsigned short*)((long)base+22)
#define BPB_HiddSec(base) (unsigned long*)((long)base+28)
#define BPB_TotSec32(base) (unsigned long*)((long)base+32)
#define BS_16BootSig(base) (unsigned char*)((long)base+38)
#define BS_16VolID(base) (unsigned long*)((long)base+39)
#define BS_16VolLab_sz11(base) (unsigned char*)((long)base+43)
#define BS_16FilSysType_sz8(base) (unsigned char*)((long)base+54)
#define BPB_FatSz32(base) (unsigned long*)((long)base+36)
#define BPB_32ExtFlags(base) (unsigned short*)((long)base+40)
#define BPB_32FSVer(base) (unsigned short*)((long)base+42)
#define BPB_32RootClus(base) (unsigned long*)((long)base+44)
#define BPB_32FSInfo(base) (unsigned short*)((long)base+48)
#define BPB_32BkBootSec(base) (unsigned short*)((long)base+50)
#define BS_32BootSig(base) (unsigned char*)((long)base+66)
#define BS_32VolID(base) (unsigned long*)((long)base+67)
#define BS_32VolLab_sz11(base) (unsigned char*)((long)base+71)
#define BS_32FilSysType_sz8(base) (unsigned char*)((long)base+82)

#define FATFS_FirstSecOfClus(N) (((N-2)*gFB.secPerClus)+gFB.firstDataSec)
/* assumes FAT is a uint8 array */
#define FAT16Offset(N) (N*2)
#define FAT32Offset(N) (N*4) 
#define FATFS16_FatEntryInSec(N) (gFB.rsvdSecCnt+(FAT16Offset(N)/gFB.bytPerSec))
#define FATFS16_FatEntryAtOffset(N) (FAT16Offset(N)%gFB.bytPerSec)
/* end of assumption */

struct TFatBootSector
{
  uint16 bytPerSec, secPerClus, rsvdSecCnt, numFats, 
    rootEntCnt, totSec16, fatSz16, extFlags, fsVer, fsInfo;
  uint32 fatSz32, totSec32, rootClus, volID;

  uint32 rootDirSecs, fatSz, firstDataSec, totSec, dataSec;
  int isFat12, isFat16, isFat32;
};

struct TFileInfo 
{
  uint8 name[FILEINFONAME_SIZE], lfn[FILEINFOLFN_SIZE], 
    attr, ntRes, crtTimeTenth;
  uint16 crtTime, crtDate, lastAccDate, wrtTime, wrtDate;
  uint32 firstClus, fileSize;
};

struct TFat
{
  uint32 baseSec;
  uint8 *BBuf, *FBuf, *RDBuf;
  uint32 rdSize;
};

struct TClusList
{
  uint32 baseClus;
  uint32 adjClusCnt;
};

extern struct TFatBootSector gFB;
extern struct TFat gFat;

int fatfs_init(int baseSec);
int fatfs_loadbootsector();
int fatfs_loadfat();
int fatfs16_loadrootdir();
int fatfs_fileinfo_indir(char *cFile, uint8 *dirBuf, uint16 dirBufSize, 
  struct TFileInfo *fInfo, uint32 *prevPos);
int fatfs16_getfile_opticlusterlist(struct TFileInfo fInfo, struct TClusList *cl, int *clSize, uint32 *prevClus);
int fatfs_cleanup();

#endif

