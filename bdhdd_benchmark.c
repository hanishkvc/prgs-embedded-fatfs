int bdhdd_get_sectors_benchmark(bdkT *bd, long sec, long count, char*buf)
{
  int iBuf, iStat, iError, ret;
  int iLoops,iCurSecs,iLoop,iSec;
  uint16 *buf16 = (uint16*)buf;

  iBuf=0;
  iLoops = (count/256);
  if((count%256) != 0)
    iLoops++;
  for(iLoop=0;iLoop<iLoops;iLoop++)
  {
    if(count < 256)
      iCurSecs = count;
    else
      iCurSecs = 0;

    if((ret=bdhdd_readyforcmd(bd,bd->devId,BDHDD_CFG_LBA,
              ((sec&0xf000000)>>24))) != 0) return ret;
    BDHDD_WRITE8(BDHDD_CMDBR_SECCNT,iCurSecs);
    BDHDD_WRITE8(BDHDD_CMDBR_LBA0,(sec&0xff));
    BDHDD_WRITE8(BDHDD_CMDBR_LBA8,((sec&0xff00)>>8));
    BDHDD_WRITE8(BDHDD_CMDBR_LBA16,((sec&0xff0000)>>16));
    if(bdhdd_sendcmd(bd,BDHDD_CMD_READSECTORS,&iStat,&iError,
        BDHDD_WAIT_CMDTIME) != 0)
    {
      bdhdd_printsignature(bd);
      fprintf(stderr,"ERR:BDHDD: During READSECTORS cmd\n");
      return -ERROR_FAILED;
    }
    if(iCurSecs == 0) iCurSecs = 256;
    for(iSec=0;iSec<iCurSecs;iSec++)
    {
      if(iSec > 0)
      {
        ret=bdhdd_checkstatus(bd,0xFF,&iStat,&iError,BDHDD_WAIT_CMDTIME);
        if(ret != 0)
        {
          fprintf(stderr,"ERR:BDHDD: middle of READSECTORS, iStat[0x%x] iLoop[%d] iSec[%d]\n",iStat,iLoop,iSec);
          return ret;
        }
      }
      if((iStat&BDHDD_STATUS_DRQBIT)==0)
      {
        fprintf(stderr,"ERR:BDHDD: READSECTORS DRQ not set, iStat[0x%x] iLoop[%d] iSec[%d]\n",iStat,iLoop,iSec);
        return -ERROR_FAILED;
      }
      BDHDD_READ16S(BDHDD_CMDBR_DATA,&buf16[iBuf],256);
      iBuf+=256;
      iBuf = 0;
    }
    count-=iCurSecs;
    sec+=iCurSecs;
  }
  return 0;
}
