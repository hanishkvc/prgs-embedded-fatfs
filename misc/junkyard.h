
/* Moved from fatfs.h - Sep2004
 * These defines provide a fast access method to the corresponding boot sector
 * filed, however they are not portable as many fields in the bootsector
 * are not properly aligned based on their size so on processors like ARM
 * Mips etc where the h/w doesn't provide implicit unaligned access support
 * this will cause a problem
 */
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

/*
 * Moved from bdfile.c - Oct2004
 * Found that _llseek isnot popular, and also not defined implicitly as
 * of now,  so moved it out.
 */
#ifdef BDFILE_FILE__LLSEEK
  {
  long long fOffset;
  unsigned long fOffsetH, fOffsetL;
  fOffset = (long long)sec*bd->secSize;
  fOffsetH = (fOffset >> 32) & 0xFFFFFFFF;
  fOffsetL = fOffset & 0xFFFFFFFF;
  res = _llseek((int)bd->u1,fOffsetH,fOffsetL,&fOffset,SEEK_SET);
  }
#endif

/*
 * One of the modifications to support MemCARD3PCtlr with Dm270 on PHIDMR
 *
 */
static inline int bdhdd_checkstatus(bdkT *bd,uint8 cmd,int *iStatus,int *iError,int waitCnt)
{
  int ret;

  ret=bdhdd_altstat_waitifbitset(bd,BDHDD_STATUS_BSYBIT,iStatus,waitCnt);
#if DEBUG_PRINT_BDHDD > 25
  printf("INFO:[altstatus=0x%x]\n",*iStatus);
#endif
  if(ret!=0)
  {
    fprintf(stderr,"ERR:BDHDD:checkstatus:0: Cmd [0x%x] timeout\n", cmd);
    return -ERROR_TIMEOUT;
  }
  if(bd->grpId == BDHDD_GRPID_DM270CF_MEMCARD3PCTLR)
  {
    ret=bdhdd_altstat_waitifbitclear(bd,BDHDD_STATUS_DRQBIT,iStatus,waitCnt);
#if DEBUG_PRINT_BDHDD > 25
  printf("INFO:[altstatus=0x%x]\n",*iStatus);
#endif
    if(ret!=0)
    {
    fprintf(stderr,"ERR:BDHDD:checkstatus:1: Cmd [0x%x] timeout\n", cmd);
    return -ERROR_TIMEOUT;
    }
  }
  *iStatus = BDHDD_READ8(BDHDD_CMDBR_STATUS);
#if DEBUG_PRINT_BDHDD > 25
  printf("INFO:[status=0x%x]\n",*iStatus);
#endif
  if(*iStatus & BDHDD_STATUS_ERRBIT)
  {
    *iError = BDHDD_READ8(BDHDD_CMDBR_ERROR);
    fprintf(stderr,"ERR: Cmd [0x%x] returned error [0x%x]\n",cmd,*iError);
    return -ERROR_FAILED;
  }
  return 0;
}

#ifdef PRG_MODE_DM270_MEMCARD3PCTLR

static inline int bdhdd_checkstatus(bdkT *bd,uint8 cmd,int *iStatus,int *iError,int waitCnt)
{
  int iCur,iTest;

  waitCnt=waitCnt*4;
  do{
    iCur = BDHDD_READ8(BDHDD_CMDBR_STATUS);
    iTest = iCur & 0x58;
    waitCnt--;
  }while((iTest != 0x58) && waitCnt);
  *iStatus = BDHDD_READ8(BDHDD_CMDBR_STATUS);
#if DEBUG_PRINT_BDHDD > 25
  printf("INFO:MemCARD3PCtlr:[status=0x%x]\n",*iStatus);
#endif
  if(*iStatus & BDHDD_STATUS_ERRBIT)
  {
    *iError = BDHDD_READ8(BDHDD_CMDBR_ERROR);
    fprintf(stderr,"ERR: Cmd [0x%x] returned error [0x%x]\n",cmd,*iError);
    return -ERROR_FAILED;
  }
  if(waitCnt == 0)
  {
    fprintf(stderr,"ERR:BDHDD:checkstatus:0: Cmd [0x%x] timeout\n", cmd);
    return -ERROR_TIMEOUT;
  }
  return 0;
}

#else

