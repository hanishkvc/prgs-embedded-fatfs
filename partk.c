/*
 * part.c - library for working with partition table
 * v16july2004-1000
 * C Hanish Menon <hanishkvc>, 16july2004
 * 
 */

#include <inall.h>
#include <bdfile.h>
#include <part.h>

int part_get(struct TPartInfo *pInfo)
{
  uint8 pBuf[512], *pCur;
  uint16 t1, iPart, tVerify;
  uint16 pOffsets[4] 
    = { PART1_OFFSET, PART2_OFFSET, PART3_OFFSET, PART4_OFFSET };

  bd_get_sectors(0, 1, pBuf); 
  pCur = pBuf+PARTEXECMARK_OFFSET;
  tVerify = buffer_read_uint16_le(&pCur);
  if(tVerify != 0xaa55)
  {
    printf("part:get:ERROR: 0xaa55 missing from offset %d\n", PARTEXECMARK_OFFSET);
    return -1;
  }

  for(iPart = 0; iPart < 4; iPart++)
  {
    pCur = pBuf+pOffsets[iPart];
    pInfo->active[iPart] = buffer_read_uint8_le(&pCur);
    pInfo->sHead[iPart] = buffer_read_uint8_le(&pCur);
    t1 = buffer_read_uint16_le(&pCur);
    pInfo->sCyl[iPart] = ((t1 & 0xff00) >> 8) | ((t1 & 0xc0) << 2);
    pInfo->sSec[iPart] = t1 & 0x3f;
    pInfo->type[iPart] = buffer_read_uint8_le(&pCur);
    pInfo->eHead[iPart] = buffer_read_uint8_le(&pCur);
    t1 = buffer_read_uint16_le(&pCur);
    pInfo->eCyl[iPart] = ((t1 & 0xff00) >> 8) | ((t1 & 0xc0) << 2);
    pInfo->eSec[iPart] = t1 & 0x3f;
    pInfo->fLSec[iPart] = buffer_read_uint32_le(&pCur);
    pInfo->nLSec[iPart] = buffer_read_uint32_le(&pCur);
#if (DEBUG_PRINT_PART > 10)
    printf("part[%d]: active[0x%x] type [0x%x] fLSec [%ld] nLSec[%ld]\n",
      iPart, pInfo->active[iPart], pInfo->type[iPart], 
      pInfo->fLSec[iPart], pInfo->nLSec[iPart]);
    printf("part[%d]: seHead[%d:%d] seCyl[%d:%d] seSec[%d:%d]\n",
      iPart, pInfo->sHead[iPart], pInfo->eHead[iPart], 
      pInfo->sCyl[iPart], pInfo->eCyl[iPart], 
      pInfo->sSec[iPart], pInfo->eSec[iPart]);
#endif
  }
  return 0;
}

