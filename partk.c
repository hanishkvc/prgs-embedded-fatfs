/*
 * partk.c - library for working with partition table
 * v30Sep2004-2145
 * C Hanish Menon <hanishkvc>, 16july2004
 * 
 */

#include <inall.h>
#include <bdfile.h>
#include <partk.h>

int partk_get(pikT *pi, bdkT *bd)
{
  uint8 pBuf[BDK_SECSIZE], *pCur;
  uint16 t1, iPart, tVerify;
  uint16 pOffsets[PARTK_NUMPARTS] 
    = { PARTK1_OFFSET, PARTK2_OFFSET, PARTK3_OFFSET, PARTK4_OFFSET };

  bd->get_sectors(bd,0,1,pBuf); 
  pCur = pBuf+PARTKEXECMARK_OFFSET;
  tVerify = buffer_read_uint16_le(&pCur);
  if(tVerify != 0xaa55)
  {
    printf("partk:get:ERROR: 0xaa55 missing from offset %d\n", PARTKEXECMARK_OFFSET);
    return -1;
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

