/*
 * fatfs.h - library for working with fat filesystem
 * v17Mar2005_1303
 * C Hanish Menon <hanishkvc>, 14july2004
 * 
 */

#ifndef _FATFS_H_
#define _FATFS_H_

#define FATFS_LIBVER "v17Mar2005_2008"

#include <rwporta.h>
#include <bdk.h>
#include <utilsporta.h>

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

#define FATFS_BS_STARTBYTE_T0 0xeb
#define FATFS_BS_STARTBYTE_T1 0xe9

#define FATDIRENTRY_SIZE 32
#define FATDIRENTRYNAME_SIZE 11
#define FATDIRENTRYLFN_SIZE 255
#define FATFSNAME_SIZE (FATDIRENTRYNAME_SIZE+1)
#define FATFSLFN_SIZE (FATDIRENTRYLFN_SIZE+1)
#define FATDIRENTRYLFN_PARTIALCHARS 13
#define FATFSPATHNAME_MAXSIZE (10*FATFSLFN_SIZE)

#define FATFSCLUS_MAXSIZE (64*1024)
#define FATFSUSERCONTEXTDIRBUF_MAXSIZE (2048*FATDIRENTRY_SIZE)
#define FATFSUSERCONTEXT_NUMFILES 4
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
#define FATFS_EOF FAT32_EOF
#define FATFS_BADCLUSTER FAT32_BADCLUSTER

#define FAT32_EXTFLAGS_ACTIVEFAT(N) ((N)&0xF)
#define FAT32_FSVER 0x0000

#define FATUC_USED 1
#define FATUC_FREE 0

#define FATUC_FOPEN_OPEN 1
#define FATUC_FOPEN_CREATE 2

#define FATUC_FOPEN_CREATE_DENTRIESPERFILE 4

#define FATUC_SEEKSET 1
#define FATUC_SEEKCUR 2
#define FATUC_SEEKEND 3

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

#define FINFO_UPDATED_NONE 0x0
#define FINFO_UPDATED_SIZE 0x1
#define FINFO_UPDATED_NAME 0x2
#define FINFO_UPDATED_FIRSTCLUS 0x4
#define FINFO_UPDATED_ALL 0x8
struct TFileInfo 
{
  uint8 name[FATFSNAME_SIZE], attr, ntRes, crtTimeTenth;
  char16 lfn[FATFSLFN_SIZE];
  uint16 crtTime, crtDate, lastAccDate, wrtTime, wrtDate;
  uint32 firstClus, fileSize;
  uint32 newFileSize;
  int updated;
  int fDEntryPos, lDEntryPos;
};

struct TFatBuffers
{
  /* FATBOOTSEC_MAXSIZE/4 , would align to 4byte boundry */
  uint32 FBBuf[FATBOOTSEC_MAXSIZE/4];
  uint32 FFBuf[FATFAT_MAXSIZE/4]; 
  uint32 FRDBuf[FATROOTDIR_MAXSIZE/4];
};

struct TClusList
{
  uint32 baseClus;
  uint32 adjClusCnt;
};

#define FATFS_FREECLUSLIST_SIZE 1024

struct TFatFreeClusters
{
  struct TClusList cl[FATFS_FREECLUSLIST_SIZE];
  int clSize, clIndex;
  uint32 curMinAdjClusCnt, fromClus;
};

struct TFat
{
  bdkT *bd;	
  uint32 baseSec, totSecs;
  uint8 *BBuf, *FBuf, *RDBuf;
  uint32 rdSize;
  struct TFatBootSector bs;
  uint32 curFatNo, curFatStartEntry, curFatEndEntry, maxFatEntry;
  uint32 curFatPartStartSecAbs, curFatEndSecAbs;
  uint32 cntDataClus,clusSize;
  int RDUpdated, fatUpdated;
  struct TFatFreeClusters freeCl;
  /* Functions */
  int (*getfatentry)(struct TFat *fat, uint32 iEntry,
          uint32 *iValue, uint32 *iActual);
  int (*setfatentry)(struct TFat *fat, uint32 iEntry, uint32 iValue);
  int (*loadrootdir)(struct TFat *fat);
  int (*storerootdir)(struct TFat *fat);
  int (*checkfatbeginok)(struct TFat *fat);
};

struct TFatFile
{
	struct TFileInfo fInfo;
	uint32 fPos;
	int state;
	uint32 fromClus,offInFromClus;
	struct TFileInfo dInfo;
};

struct TFatFsUserContext
{
  struct TFat *fat;
  uint8 *curDirBuf;
  uint32 curDirBufLen;
  struct TFileInfo curDirInfo;
  uint8 *tempDirBuf;
  uint32 tempDirBufLen;
  struct TFileInfo tempDirInfo;
  
  struct TFatFile files[FATFSUSERCONTEXT_NUMFILES];
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
int fatfs16_storerootdir(struct TFat *fat);
int fatfs32_storerootdir(struct TFat *fat);
int fatfs_getfileinfo_fromdir(char *cFile, uint8 *dirBuf, uint32 dirBufSize, 
      struct TFileInfo *fInfo, uint32 *prevPos, int useLFN);
int fatfs16_getfatentry(struct TFat *fat, uint32 iEntry,
      uint32 *iValue, uint32 *iActual);
int fatfs16_setfatentry(struct TFat *fat, uint32 iEntry, uint32 iValue);
int fatfs32_getfatentry(struct TFat *fat, uint32 iEntry, 
      uint32 *iValue, uint32 *iActual);
int fatfs32_setfatentry(struct TFat *fat, uint32 iEntry, uint32 iValue);
int fatfs__getoptifreecluslist(struct TFat *fat, struct TClusList *cl,
      int *clSize, int minAdjClusCnt, uint32 *fromClus);
int fatfs_getopticluslist_usefileinfo(struct TFat *fat, 
      struct TFileInfo *fInfo, struct TClusList *cl, int *clSize, 
      uint32 *fromClus);
int fatfs_loadfileallsec_usefileinfo(struct TFat *fat, 
      struct TFileInfo *fInfo, uint8 *buf, int32 bufLen, uint32 *totSecsRead);
/* Call this before calling loadfileclus to verify if any 
   empty space can occur at end of the given buffer */
int fatfs_checkbuf_forloadfileclus(struct TFat *fat, uint32 bufLen);
int fatfs_loadfileclus_usefileinfo(struct TFat *fat, struct TFileInfo *fInfo, 
      uint8 *buf, uint32 bufLen, uint32 *totalClusRead, uint32 *fromClus);
int fatfs_update_freecluslist(struct TFat *fat);
int fatfs_getopti_freecluslist(struct TFat *fat, struct TClusList *cl,
      int *clSize, int32 clusRequired, uint32 *fromClusHint);
int fatfs__storefileclus_usefileinfo(struct TFat *fat, struct TFileInfo *fInfo, 
      uint8 *buf, uint32 bytesToWrite, uint32 *totalClusWriten, 
      uint32 *lastClusWriten, uint32 *fromClus);
int fatfs_cleanup(struct TFat *fat);

/* From fsutils */
/* partNo starts from 0 */
int fsutils_mount(bdkT *bd, int bdGrpId, int bdDevId, int partNo,
      struct TFat *fat, struct TFatBuffers *fatBuffers, 
      int forceMbr, int bdkFlags);
int fsutils_umount(bdkT *bd, struct TFat *fat);

/* Notes
 * * THREADED prgs:  A given user context shouldn't be actively 
 *   used by fatuc_xxx functions at the same time
 * * ch[ange]dir: user giving wrong path won't affect the uc
 * * updatetempdirbufinfo: Always call this before using tempDirBuf 
 *   of uc as tempDirBuf could point to TFat.RDBuf in some cases
 */
void fatuc_updatetempdirbufinfo(struct TFatFsUserContext *uc);
int fatuc__changedir(struct TFatFsUserContext *uc, char *fDirName,
      int bUpdateCurDir);
int fatuc_init(struct TFatFsUserContext *uc, struct TFat *fat);
int fatuc_chdir(struct TFatFsUserContext *uc, char *fDirName);
int fatuc_getfileinfo(struct TFatFsUserContext *uc, char *cFile,  
      struct TFileInfo *fInfo, uint32 *prevPos);
int fatuc_fopen(struct TFatFsUserContext *uc, char *cFile, int *fId,
      int flag);
int fatuc_mkdir(struct TFatFsUserContext *uc, char *dirName,
      uint8* tBuf, int tBufLen);
int fatuc_fseek(struct TFatFsUserContext *uc, int fId,
      int32 offset, int whence);
int fatuc_fread(struct TFatFsUserContext *uc, int fId,
      uint8 *buf, uint32 bufLen, uint32 bytesToRead, 
      uint8 **atBuf, uint32 *bytesRead);
int fatuc__deletefile(struct TFatFsUserContext *uc, int fId);
int fatuc_move_dentry(struct TFatFsUserContext *uc, char *src, 
      char* destPath, char16* u16FName, char* tBuf, int tBufLen);
int fatuc_fclose(struct TFatFsUserContext *uc, int fId);

#endif

