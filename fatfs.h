/*
 * fatfs.h - library for working with fat filesystem
 * v12Oct2004_1810
 * C Hanish Menon <hanishkvc>, 14july2004
 * 
 */

#ifndef _FATFS_H_
#define _FATFS_H_

#define FATFS_LIBVER "v12Oct2004_1811"

#include <rwporta.h>
#include <bdk.h>

#ifndef DEBUG_PRINT_FATFS
#define DEBUG_PRINT_FATFS 11
#endif

#ifndef FATFS_FAT_PARTLYMAPPED 
#define FATFS_FAT_FULLYMAPPED
#endif

/* Define to simulate error during FATFS_FAT_PARTLYMAPPED */
#undef FATFS_SIMULATE_ERROR  

#define FATSEC_MAXSIZE 4096
#define FATBOOTSEC_MAXSIZE FATSEC_MAXSIZE
#ifdef FATFS_FAT_PARTLYMAPPED
#define FATFAT_MAXSIZE (1024*1024)
#endif
#ifdef FATFS_FAT_FULLYMAPPED
#define FATFAT_MAXSIZE (8192*1024)
#endif
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

#define FAT16_CLEANSHUTBIT 0x8000
#define FAT16_HARDERRBIT 0x4000
#define FAT32_CLEANSHUTBIT 0x08000000
#define FAT32_HARDERRBIT   0x04000000

#define FAT16_EOF 0xFFF8
#define FAT16_BADCLUSTER 0xFFF7
#define FAT32_EOF 0x0FFFFFF8
#define FAT32_BADCLUSTER 0x0FFFFFF7
#define FATFS_FREECLUSTER 0x0

#define FAT32_EXTFLAGS_ACTIVEFAT(N) ((N)&0xF)
#define FAT32_FSVER 0x0000

#define fatfs_firstsecofclus(fat, N) (((N-2)*fat->bs.secPerClus)+fat->bs.firstDataSec)
/* assumes FAT is a uint8 array */
#define FAT16OffsetInBytes(N) (N*2)
#define FAT32OffsetInBytes(N) (N*4) 
#define FATFS16_FatEntryInSec(fat, N) (fat->bs.rsvdSecCnt+(FAT16OffsetInBytes(N)/fat->bs.bytPerSec))
#define FATFS16_FatEntryAtOffset(fat, N) (FAT16OffsetInBytes(N)%fat->bs.bytPerSec)
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
  uint32 baseSec, totSecs;
  uint8 *BBuf, *FBuf, *RDBuf;
  uint32 rdSize;
  struct TFatBootSector bs;
  uint32 curFatNo, curFatStartEntry, curFatEndEntry, maxFatEntry;
  /* Functions */
  int (*loadrootdir)(struct TFat *fat);
  int (*getfatentry)(struct TFat *fat, uint32 iEntry,
          uint32 *iValue, uint32 *iActual);
  int (*checkfatbeginok)(struct TFat *fat);
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

int fatfs_init(struct TFat *fat, struct TFatBuffers *fBufs, 
      bdkT *bd, int baseSec, int totSecs);
int fatfs_loadbootsector(struct TFat *fat);
int fatfs_loadfat(struct TFat *fat, int fatNo, int startEntry);
int fatfs16_checkfatbeginok(struct TFat *fat);
int fatfs32_checkfatbeginok(struct TFat *fat);
int fatfs16_loadrootdir(struct TFat *fat);
int fatfs32_loadrootdir(struct TFat *fat);
int fatfs_getfileinfo_fromdir(char *cFile, uint8 *dirBuf, uint16 dirBufSize, 
      struct TFileInfo *fInfo, uint32 *prevPos);
int fatfs16_getfatentry(struct TFat *fat, uint32 iEntry,
      uint32 *iValue, uint32 *iActual);
int fatfs32_getfatentry(struct TFat *fat, uint32 iEntry, 
      uint32 *iValue, uint32 *iActual);
int fatfs_getopticluslist_usefileinfo(struct TFat *fat, 
      struct TFileInfo *fInfo, struct TClusList *cl, int *clSize, 
      uint32 *prevClus);
int fatfs_loadfileallsec_usefileinfo(struct TFat *fat, 
      struct TFileInfo *fInfo, uint8 *buf, int32 bufLen, uint32 *totSecsRead);
/* Call this before calling loadfileclus to verify if any 
   empty space can occur at end of the given buffer */
int fatfs_checkbuf_forloadfileclus(struct TFat *fat, uint32 bufLen);
int fatfs_loadfileclus_usefileinfo(struct TFat *fat, struct TFileInfo *fInfo, 
      uint8 *buf, uint32 bufLen, uint32 *totalClusRead, uint32 *prevClus);
int fatfs_cleanup(struct TFat *fat);

/* From fsutils */
/* partNo starts from 0 */
int fsutils_mount(bdkT *bd, int bdGrpId, int bdDevId,
      struct TFat *fat, struct TFatBuffers *fatBuffers, int partNo);
int fsutils_umount(bdkT *bd, struct TFat *fat);

/* Notes
 * * THREADED prgs:  A given user context shouldn't be actively 
 *   used by fatuc_xxx functions at the same time
 * * ch[ange]dir: user giving wrong path won't affect the uc
 * * updatetempdirbufinfo: Always call this before using tempDirBuf 
 *   of uc as tempDirBuf could point to TFat.RDBuf in some cases
 */
void fatuc_updatetempdirbufinfo(struct TFatFsUserContext *uc);
int fatuc_changedir(struct TFatFsUserContext *uc, char *fDirName,
  int bUpdateCurDir);
int fatuc_init(struct TFatFsUserContext *uc, struct TFat *fat);
int fatuc_chdir(struct TFatFsUserContext *uc, char *fDirName);
int fatuc_getfileinfo(struct TFatFsUserContext *uc, char *cFile,  
  struct TFileInfo *fInfo, uint32 *prevPos);

#endif

