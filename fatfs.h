/*
 * fatfs.h - library for working with fat filesystem
 * v30Sep2004_2300
 * C Hanish Menon <hanishkvc>, 14july2004
 * 
 */

#ifndef _FATFS_H_
#define _FATFS_H_

#include <rwporta.h>
#include <bdk.h>

#ifndef DEBUG_PRINT_FATFS
#define DEBUG_PRINT_FATFS 11
#endif

#define FATBOOTSEC_MAXSIZE 512
#define FATFAT_MAXSIZE (2048*1024)
#define FATROOTDIR_MAXSIZE (512*1024)
#define FATDIRENTRY_SIZE 32
#define FATDIRENTRYNAME_SIZE 11
#define FATDIRENTRYLFN_SIZE 255
#define FATFSNAME_SIZE (FATDIRENTRYNAME_SIZE+1)
#define FATFSLFN_SIZE (FATDIRENTRYLFN_SIZE+1)
#define FATDIRENTRYLFN_PARTIALCHARS 13
#define FATFSPATHNAME_MAXSIZE (10*FATFSLFN_SIZE)

#define FATFSCLUS_MAXSIZE (64*1024)
#define FATFSUSERCONTEXTDIRBUF_MAXSIZE (2048*FATDIRENTRY_SIZE)
#define FATFS_DIRSEP '\\'

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

#define fatfs_firstsecofclus(fat, N) (((N-2)*fat->bs.secPerClus)+fat->bs.firstDataSec)
/* assumes FAT is a uint8 array */
#define FAT16Offset(N) (N*2)
#define FAT32Offset(N) (N*4) 
#define FATFS16_FatEntryInSec(fat, N) (fat->bs.rsvdSecCnt+(FAT16Offset(N)/fat->bs.bytPerSec))
#define FATFS16_FatEntryAtOffset(fat, N) (FAT16Offset(N)%fat->bs.bytPerSec)
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
  uint8 name[FATFSNAME_SIZE], lfn[FATFSLFN_SIZE], 
    attr, ntRes, crtTimeTenth;
  uint16 crtTime, crtDate, lastAccDate, wrtTime, wrtDate;
  uint32 firstClus, fileSize;
};

struct TFatBuffers
{
  /* FATBOOTSEC_MAXSIZE/4 , would align to 4byte boundry */
  uint32 FBBuf[FATBOOTSEC_MAXSIZE/4];
  uint32 FFBuf[FATFAT_MAXSIZE/4]; 
  uint32 FRDBuf[FATROOTDIR_MAXSIZE/4];
};

struct TFat
{
  bdkT *bd;	
  uint32 baseSec;
  uint8 *BBuf, *FBuf, *RDBuf;
  uint32 rdSize;
  struct TFatBootSector bs;
};

struct TClusList
{
  uint32 baseClus;
  uint32 adjClusCnt;
};

struct TFatFsUserContext
{
  struct TFat *fat;
  uint8 *curDirBuf;
  uint32 curDirBufLen;
  uint8 *tempDirBuf;
  uint32 tempDirBufLen;
  uint8 sCurDir[FATFSPATHNAME_MAXSIZE];
  uint8 sTempDir[FATFSPATHNAME_MAXSIZE];
  uint32 dirBuf1[FATFSUSERCONTEXTDIRBUF_MAXSIZE/4];
  uint32 dirBuf2[FATFSUSERCONTEXTDIRBUF_MAXSIZE/4];
  uint8 pName[FATFSPATHNAME_MAXSIZE];
  uint8 fName[FATFSLFN_SIZE];
};

extern struct TFat gFat;

int fatfs_init(struct TFat *fat, struct TFatBuffers *fBufs, 
      bdkT *bd, int baseSec);
int fatfs_loadbootsector(struct TFat *fat);
int fatfs_loadfat(struct TFat *fat);
int fatfs16_loadrootdir(struct TFat *fat);
int fatfs_getfileinfo_fromdir(char *cFile, uint8 *dirBuf, uint16 dirBufSize, 
      struct TFileInfo *fInfo, uint32 *prevPos);
int fatfs16_getopticluslist_usefileinfo(struct TFat *fat, 
      struct TFileInfo *fInfo, struct TClusList *cl, int *clSize, 
      uint32 *prevClus);
int fatfs_loadfileallsec_usefileinfo(struct TFat *fat, 
      struct TFileInfo *fInfo, uint8 *buf, int32 bufLen);
/* Call this before calling loadfileclus to verify if any 
   empty space can occur at end of the given buffer */
int fatfs_checkbuf_forloadfileclus(struct TFat *fat, uint32 bufLen);
int fatfs_loadfileclus_usefileinfo(struct TFat *fat, struct TFileInfo *fInfo, 
      uint8 *buf, uint32 bufLen, uint32 *totalClusRead, uint32 *prevClus);
int fatfs_cleanup(struct TFat *fat);

/* Notes
 * * THREADED prgs:  A given user context shouldn't be actively 
 *   used by fatuc_xxx functions at the same time
 * * ch[ange]dir: user giving wrong path won't affect the uc
 * * updatetempdirbufinfo: Always call this before using tempDirBuf 
 *   of uc as tempDirBuf could point to gFat.RDBuf in some cases
 */
void fatuc_updatetempdirbufinfo(struct TFatFsUserContext *uc);
int fatuc_changedir(struct TFatFsUserContext *uc, char *fDirName,
  int bUpdateCurDir);
int fatuc_init(struct TFatFsUserContext *uc, struct TFat *fat);
int fatuc_chdir(struct TFatFsUserContext *uc, char *fDirName);
int fatuc_getfileinfo(struct TFatFsUserContext *uc, char *cFile,  
  struct TFileInfo *fInfo, uint32 *prevPos);

#endif

