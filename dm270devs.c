/*
 * Issues in FPMC board:
 * * IOIS16 most probably is a output from Device to Host to tell its doing
 *   a 16bit transfer or not and Not a input to Device as specified in FPMC
 *   by connecting A5 to IOIS16 of Device.
 * * In 16bit mode Addr on the Addr bus is shifted by 1bit, which doesn't
 *   seem to be accounted for. 
 *   * So if we set the BUS to 8bit then again as doing 16bit access on 8bit 
 *   would require use of EM_BEH and EM_BEL from DM270, which is not currently
 *   done, so we CANNT SET BUS TO 8Bit.
 *   * So if we set the BUS to 16bit then again as 1bit shift is not accounted
 *   we will have to use addresses which are shifted that is:
 *   Ata Addr 1 will be Addr 2
 *   Ata Addr 2 will be Addr 4
 *   Ata Addr 7 will be Addr 14
 * * Also as nATA_CS0 and nATA_CS1 are active low signals, in FPMC the 
 *   CNTBR registers come before CMDBR registers
 * Note regarding DM270
 * * CS3CTRL2 as bit to tell if EM_CS3 region is 8bit or 16bit which
 *   on reset is 8bit.
 * * BUSCTRL as EMWE3 to enable checking of nEM_WAIT signal for EM_CS3 region
 */
int bdhdd_init_grpid_dm270ide_fpmc(struct bdkt *bd, int grpId, int devId)
{

  if(grpId == BDHDD_GRPID_DM270IDE_FPMC)
  {
    bd->CNTBR = PA_MEMREAD16(0x30A50) * 0x100000;
    bd->CMDBR = PA_MEMREAD16(0x30A50) * 0x100000 + 0x10;
    fprintf(stderr,"INFO:BDHDD:DM270:FPMC IDE - devId [%d] \n",devId);

    /* Enable Buffer for IDE access from DM270 */
    gio_dir_output(29);
    gio_bit_clear(29);
    lu_nanosleeppaka(1,0);
    return 0;
  }
  return -1;
}

