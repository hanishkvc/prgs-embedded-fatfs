/*
 * partk.c - library for working with partition table
 * v14Oct2004_0935
 * C Hanish Menon <hanishkvc>, 16july2004
 * 
 */

#include <inall.h>
#include <bdfile.h>
#include <partk.h>

int partk_get(pikT *pi, bdkT *bd, char *pBuf, int forceMbr)
{
  uint8 *pCur;
  uint16 t1, iPart, tVerify;
  uint16 pOffsets[PARTK_NUMPARTS] 
    = { PARTK1_OFFSET, PARTK2_OFFSET, PARTK3_OFFSET, PARTK4_OFFSET };

  if(bd->get_sectors(bd,0,1,pBuf) != 0)
  {
    fprintf(stderr,"ERR:partk: getting 0th sector\n");
    return -ERROR_FAILED;
  }
  if(forceMbr == 0)
  {
    pCur = pBuf+PARTKEXECMARK_OFFSET;
    tVerify = buffer_read_uint16_le(&pCur);
    if(tVerify != 0xaa55)
    {
      fprintf(stderr,"ERR:partk:NoMBR: offset %d != 0xaa55, but [0x%x]\n",
        PARTKEXECMARK_OFFSET,tVerify);
      return -ERROR_PARTK_NOMBR;
    }
    pCur=pBuf;
    tVerify=buffer_read_uint8_le(&pCur);
    if((tVerify==PARTK_BS_STARTBYTE_T0) || (tVerify==PARTK_BS_STARTBYTE_T1))
    {
      fprintf(stderr,"ERR:partk:NoMBR: BootSec byte found at byte0\n");
      fprintf(stderr,"INFO:partk:Bootloaders like grub in MBR, can trigger this, If so forceMbr\n");
      return -ERROR_PARTK_NOMBR;
    }
#ifdef PARTK_VERIFY_MBRSTARTBYTE  
    if(tVerify != PARTK_MBR_STARTBYTE_T0)
    {
      fprintf(stderr,"ERR:partk:NoMBR: MBRs byte not found at byte0\n");
      fprintf(stderr,"INFO:partk:Non booting hdd/CF can trigger this, if so forceMbr\n");
      return -ERROR_PARTK_NOMBR;
    }
#endif
  }

  for(iPart = 0; iPart < PARTK_NUMPARTS; iPart++)
  {
    pCur = pBuf+pOffsets[iPart];
    pi->active[iPart] = buffer_read_uint8_le(&pCur);
    pi->sHead[iPart] = buffer_read_uint8_le(&pCur);
    t1 = buffer_read_uint16_le(&pCur);
    pi->sCyl[iPart] = ((t1 & 0xff00) >> 8) | ((t1 & 0xc0) << 2);
    pi->sSec[iPart] = t1 & 0x3f;
    pi->type[iPart] = buffer_read_uint8_le(&pCur);
    pi->eHead[iPart] = buffer_read_uint8_le(&pCur);
    t1 = buffer_read_uint16_le(&pCur);
    pi->eCyl[iPart] = ((t1 & 0xff00) >> 8) | ((t1 & 0xc0) << 2);
    pi->eSec[iPart] = t1 & 0x3f;
    pi->fLSec[iPart] = buffer_read_uint32_le(&pCur);
    pi->nLSec[iPart] = buffer_read_uint32_le(&pCur);
#if (DEBUG_PRINT_PARTK > 10)
    printf("partk[%d]: active[0x%x] type [0x%x] fLSec [%ld] nLSec[%ld]\n",
      iPart, pi->active[iPart], pi->type[iPart], 
      pi->fLSec[iPart], pi->nLSec[iPart]);
    printf("partk[%d]: seHead[%d:%d] seCyl[%d:%d] seSec[%d:%d]\n",
      iPart, pi->sHead[iPart], pi->eHead[iPart], 
      pi->sCyl[iPart], pi->eCyl[iPart], 
      pi->sSec[iPart], pi->eSec[iPart]);
#endif
  }
  return 0;
}

