
/* Moved from fatfs.h
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

