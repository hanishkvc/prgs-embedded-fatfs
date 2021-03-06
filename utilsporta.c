/*
 * utilsporta.c - portability utility functions
 * v04Apr2005-1550
 * C Hanish Menon <hanishkvc>, 28Aug2004
 */

#include <utilsporta.h>
#include <errorporta.h>

#ifdef OS_LINUX
#include <time.h>
#endif

#ifdef OS_WINDOWS
#include <windows.h>
extern HWND listboxMSG;
#define PRINTSTR_LEN 1024
char sPrint[PRINTSTR_LEN];
int posSPrint;
#endif

#include <stdio.h>
#define na_fprintf fprintf
#define na_printstr printf

#ifdef OS_WINDOWS
int pa_windows_update_printstr(const char *str)
{
  int consumed, iRet;
    
  iRet=pa_strncpyEx(&sPrint[posSPrint],str,PRINTSTR_LEN-posSPrint,&consumed);
  posSPrint+=consumed;
  if(iRet < 0)
    iRet = 0;
  else if(sPrint[posSPrint-1] == '\n')
    iRet = 0;
  else 
    iRet = -1;
  if(iRet == 0)
    posSPrint = 0;
  return iRet;  
}    
#endif

int pa_printstr(const char *str)
{
#ifdef OS_WINDOWS
  if(pa_windows_update_printstr(str) == 0)
    SendMessage(listboxMSG,LB_ADDSTRING,0,(long int)sPrint);
  return 0;
#else
  return na_printstr(str);
#endif  
}

int pa_printstrErr(const char *str)
{
#ifdef OS_WINDOWS	
  if(pa_windows_update_printstr(str) == 0)
    SendMessage(listboxMSG,LB_ADDSTRING,0,(long int)sPrint);
  return 0;
#else  
  return na_fprintf(stderr,str);
#endif
}

int pa_printint(const int data)
{
  char sInt[32];
  int iConsumed;
  pa_int32TOstr(sInt,32,data,&iConsumed);
  return pa_printstr(sInt);
}

int pa_printints(const int data, char *str)
{
  pa_printint(data); pa_printstr(str);
  return 0;
}

int pa_printuint(const unsigned int data)
{
  char sInt[32];
  int iConsumed;
  pa_uint32TOstr(sInt,32,data,&iConsumed);
  return pa_printstr(sInt);
}

int pa_printuints(const unsigned int data, char *str)
{
  pa_printuint(data); pa_printstr(str);
  return 0;
}

int pa_printhex(const unsigned int data)
{
  char sInt[32];
  pa_uint32TOhexstr(sInt,data);
  pa_printstr("0x");
  return pa_printstr(sInt);
}

int pa_printhexs(const unsigned int data, char *str)
{
  pa_printhex(data); pa_printstr(str);
  return 0;
}
	  
void pa_divmod(int num, int den, int* div, int* rem)
{
  int sign;

  sign = 1; *div = 0; *rem = 0;
  if(den == 0) return;
  if(num < 0) sign = sign * -1;
  if(den < 0) sign = sign * -1;

  while(num >= den)
  {
    num = num - den;
    *div = *div + 1;
  }
  *rem = num;

  *div = *div * sign;
  *rem = *rem * sign;
}

int pa_mod(int num, int den)
{
  int div, rem;
  pa_divmod(num,den,&div,&rem);
  return rem;
}

int pa_div(int num, int den)
{
  int div, rem;
  pa_divmod(num,den,&div,&rem);
  return div;
}

void pa_swapstring(char *dest, int destLen)
{
  int iTemp,iCur,iLen,iEnd;
 
  iLen = destLen >> 1;
  iEnd = destLen-1;
  for(iCur=0;iCur<iLen;iCur++)
  {
    iTemp = dest[iCur];
    dest[iCur] = dest[iEnd-iCur];
    dest[iEnd-iCur] = iTemp;    
  }
}

void pa_memset(void *vdest, int ival, uint32 len)
{
  uint8 *dest,val;
  int iCur;
  int iNum8s;
  int iRemains;

  dest = vdest; val = ival;
  iNum8s = len >> 3;
  iRemains = len - (iNum8s << 3);
  for(iCur = 0; iCur < iNum8s; iCur++)
  {
    *dest = val; dest++;
    *dest = val; dest++;
    *dest = val; dest++;
    *dest = val; dest++;

    *dest = val; dest++;
    *dest = val; dest++;
    *dest = val; dest++;
    *dest = val; dest++;
  }
  for(iCur = 0; iCur < iRemains; iCur++)
  {
    *dest = val; dest++;
  }
}

void pa_memcpy(void *vDest, void *vSrc, uint32 len)
{
  int iCur;
  int iNum8s;
  int iRemains;
  uint8 *dest = vDest;
  uint8 *src = vSrc;

  iNum8s = len >> 3;
  iRemains = len - (iNum8s << 3);
  for(iCur = 0; iCur < iNum8s; iCur++)
  {
    *dest = *src; dest++; src++;
    *dest = *src; dest++; src++;
    *dest = *src; dest++; src++;
    *dest = *src; dest++; src++;

    *dest = *src; dest++; src++;
    *dest = *src; dest++; src++;
    *dest = *src; dest++; src++;
    *dest = *src; dest++; src++;
  }
  for(iCur = 0; iCur < iRemains; iCur++)
  {
    *dest = *src; dest++; src++;
  }
}

int pa_strnlen(const char *src, int maxlen)
{
  int iCur;

  for(iCur = 0; iCur < maxlen; iCur++)
  {
    if(*src == (char)NULL)
      break;
    src++;
  }
  return iCur;
}

int pa_strnlen_c16(const char16 *src, int maxlen)
{
  int iCur;

  for(iCur = 0; iCur < maxlen; iCur++)
  {
    if(*src == (char16)NULL)
      break;
    src++;
  }
  return iCur;
}

int pa_strncpyEx(char *dest, const char *src, uint32 len, int *iConsumed)
{
  uint32 iCur;

  for(iCur = 0; iCur < len; iCur++)
  {
    *dest = *src;
    if(*src != (char)NULL)
    {
      dest++; src++;
    }
    else
    {
      *iConsumed = iCur;
      return 0;
    }
  }
  return -ERR_INSUFFICIENTDEST;
}

int pa_strncpy(char *dest, char *src, uint32 len)
{
  int iTemp;
  return pa_strncpyEx(dest,src,len,&iTemp);
}

int pa_strncpyEx_c16(char16 *dest, char16 *src, uint32 len, int *iConsumed)
{
  uint32 iCur;

  for(iCur = 0; iCur < len; iCur++)
  {
    *dest = *src;
    if(*src != (char16)NULL)
    {
      dest++; src++;
    }
    else
    {
      *iConsumed = iCur;
      return 0;
    }
  }
  return -ERR_INSUFFICIENTDEST;
}

int pa_strncmp(char *dest, char *src, uint32 len)
{
  uint32 iCur;

  for(iCur = 0; iCur < len; iCur++)
  {
    if((*dest == (char)NULL) || (*src == (char)NULL))
    {
       if((*dest == (char)NULL) && (*src == (char)NULL))
         return 0;
       else
         return -1;
    }
    if(*src != *dest)
      return -1;
    dest++; src++;
  }
  return 0;
}

int pa_strncmp_c16(char16 *dest, char16 *src, uint32 len)
{
  uint32 iCur;

  for(iCur = 0; iCur < len; iCur++)
  {
    if((*dest == (char16)NULL) || (*src == (char16)NULL))
    {
       if((*dest == (char16)NULL) && (*src == (char16)NULL))
         return 0;
       else
         return -1;
    }
    if(*src != *dest)
      return -1;
    dest++; src++;
  }
  return 0;
}

void pa_toupper(char *str)
{
  while(*str)
  {
    if((*str >= 'a') && (*str <= 'z'))
      *str = *str + ('A' -'a');
    str++;
  }
}

unsigned long int pa_strtoul(const char *nptr, char **endptr, int base)
{
  unsigned long int iCur, pCur;

  iCur = 0; pCur = 0;
  while(*nptr == ' ') nptr++;
  if(*nptr == '-') nptr++;
  if(base == 10)
  {
    while(*nptr != (char)NULL)
    {
      if((*nptr < '0') || (*nptr > '9'))
        break;
      else
        iCur = iCur*10+((int)*nptr-'0'); /* 10 or base */
      if(iCur < pCur)
      {
        iCur = ERR_MAXUINT;
        break;
      }
      pCur = iCur;
      nptr++;
    }
  }
  else
  {
    iCur = ERR_MAXUINT;
    pa_printstrErr("ERR: unsupported base \n");
  }
  if(endptr != NULL)
    *endptr = (char*)nptr;
  return iCur;
}

long int pa_strtol(const char *nptr, char **endptr, int base)
{
  long int iCur, iSign;

  while(*nptr == ' ') nptr++;
  iSign = 1;
  if(*nptr == '-')
  {
    iSign = -1;
    nptr++;
  }
  iCur = pa_strtoul(nptr,endptr,base);
  if(iCur < 0)
    return ERR_MAXINT*iSign;
  return iCur*iSign;
}

char pa_intTOhexchar[16] = { '0','1','2','3','4','5','6','7','8','9',
                         'a','b','c','d','e','f' };

void pa_uint8TOhexstr(char *cDest, uint8 iInt)
{
  cDest[0] = pa_intTOhexchar[(iInt & 0xf0) >> 4];
  cDest[1] = pa_intTOhexchar[(iInt & 0xf)];
  cDest[2] = (char)PA_NULL;
}

void pa_uint32TOhexstr(char *cDest, uint32 iInt)
{
  cDest[0] = pa_intTOhexchar[(iInt & 0xf0000000) >> 28];
  cDest[1] = pa_intTOhexchar[(iInt & 0x0f000000) >> 24];
  cDest[2] = pa_intTOhexchar[(iInt & 0x00f00000) >> 20];
  cDest[3] = pa_intTOhexchar[(iInt & 0x000f0000) >> 16];
  cDest[4] = pa_intTOhexchar[(iInt & 0x0000f000) >> 12];
  cDest[5] = pa_intTOhexchar[(iInt & 0x00000f00) >> 8];
  cDest[6] = pa_intTOhexchar[(iInt & 0x000000f0) >> 4];
  cDest[7] = pa_intTOhexchar[(iInt & 0x0000000f)];
  cDest[8] = (char)PA_NULL;
}

int pa_uint32TOstr(char *cDest, int destLen, uint32 iInt, int *iConsumed)
{
  int iCur,iTemp,retVal;
  
  iCur=0; retVal=0;
  do
  {
    if(iCur == destLen)
    {
      retVal = -ERR_INSUFFICIENTDEST;
      break;
    }
    iTemp = iInt % 10;
    iInt = iInt / 10;
    cDest[iCur++] = pa_intTOhexchar[(iTemp & 0xf)];
  }while(iInt);
  if(iCur >= destLen)
    iCur = destLen-1;
  if(iCur >= 0)
  {
    cDest[iCur] = (char)PA_NULL;
    pa_swapstring(cDest,iCur);
  }
  *iConsumed = iCur;
  return retVal;
}

int pa_int32TOstr(char *cDest, int destLen, int32 iInt, int *iConsumed)
{
  int iCur = 0, tConsumed;
  int retVal = 0;
  uint32 uInt;

  *iConsumed = 0;
  if(iInt < 0)
  {
    if(destLen < 3)
      return -ERR_INSUFFICIENTDEST;
    cDest[iCur] = '-';
    iCur++;
    destLen--;
    *iConsumed = 1;
    uInt = iInt * -1;
  }
  else
    uInt = iInt;
  retVal = pa_uint32TOstr(&cDest[iCur], destLen, uInt, &tConsumed);
  *iConsumed = *iConsumed + tConsumed;
  return retVal;
}

int pa_bufferTOhexstr(char *dest, int destLen, char *src, int srcLen)
{
  int iCur;
  
  if(destLen < (srcLen*2+1))
    return -ERR_INSUFFICIENTDEST;
  for(iCur=0;iCur<srcLen;iCur++)
    pa_uint8TOhexstr(&dest[iCur*2],(uint8)src[iCur]);
  return 0;
}

int pa_strc16Tostr_nolen(char *dest, char16 *src)
{
  return pa_strc16Tostr_len(dest,src,ERR_MAXINT);
}

int pa_strc16Tostr_len(char *dest, char16 *src, int destLen)
{
  int dCur,sCur;
  
  for(sCur=0,dCur=0;dCur<destLen;)
  {
    dest[dCur++] = (char)(src[sCur]&0xff);
    if(src[sCur++] == (char16)NULL)
      break;
  }
  dest[dCur-1] = (char)PA_NULL;
  return 0;
}

int pa_strTostrc16_len(char16 *dest, char *src, int destLen)
{
  int dCur,sCur;
  
  for(sCur=0,dCur=0;dCur<destLen;)
  {
    dest[dCur++] = (char16)(src[sCur]);
    if(src[sCur++] == (char)NULL)
      break;
  }
  dest[dCur-1] = (char16)NULL;
  return 0;
}

int pa_vsnprintf(char *dest,int destLen,char *format,void *args[])
{
  int iCur, iArg, iTemp, iLengthMod;
  int retVal;

  iCur = 0; iArg = 0; retVal = 0;
  iLengthMod = 0;
  while(1)
  {
    if(iCur == destLen)
    {
      retVal = -ERR_INSUFFICIENTDEST;
      break;
    }
    if(*format == (char)NULL)
    {
      dest[iCur] = *format;
      return 0;
    }
    if(iLengthMod == 0)
    {
      if(*format != '%')
      {
        dest[iCur] = *format;
        iCur++; format++;
        continue;
      }
      else
        format++;
    }
#ifdef UTILSPORTA_OVERSAFE_MODE
    /* Don't enable cas if you have a integer value which is zero 
       corresponding to %x or %d or %u then it will fail */
    if(args[iArg] == NULL)
    {
      retVal = -ERR_INSUFFICIENTARGS;
      break;
    }
#endif
    if((*format == 'h') && (iLengthMod == 0))
    {
      format++;
      iLengthMod = 2;
      if(*format == 'h')
      {
        iLengthMod = 1;
	format++;
      }
      continue;
    }
    if((*format == 'l') && (iLengthMod == 0))
    {
      format++;
      iLengthMod = 4;
      continue;
    }
    if(*format == 'x')
    {
      if((iLengthMod == 0) || (iLengthMod == 4) || (iLengthMod == 2))
      {
        if((destLen-iCur) < 10)
        {
          retVal = -ERR_INSUFFICIENTDEST;
          break;
        }
        pa_uint32TOhexstr(&dest[iCur], (uint32)args[iArg]);
        iCur+=8;
      }
      else if(iLengthMod == 1)
      {
        if((destLen-iCur) < 4)
        {
          retVal = -ERR_INSUFFICIENTDEST;
          break;
        }
        pa_uint8TOhexstr(&dest[iCur], (uint8)args[iArg]);
        iCur+=2;
      }
      format++; iArg++;
      iLengthMod = 0;
      continue;
    }
    if(*format == 'u')
    {
      iLengthMod = 0;
      if(pa_uint32TOstr(&dest[iCur],(destLen-iCur),(uint32)args[iArg],&iTemp)!=0)
      {
        retVal = -ERR_INSUFFICIENTDEST;
        break;
      }
      iCur+= iTemp; format++; iArg++;
      continue;
    }
    if(*format == 'd')
    {
      iLengthMod = 0;
      if(pa_int32TOstr(&dest[iCur],(destLen-iCur),(int32)args[iArg],&iTemp)!=0)
      {
        retVal = -ERR_INSUFFICIENTDEST;
        break;
      }
      iCur+= iTemp; format++; iArg++;
      continue;
    }
    if(*format == 'n')
    {
      if(pa_int32TOstr(&dest[iCur],(destLen-iCur),*(int*)args[iArg],&iTemp)!=0)
      {
        retVal = -ERR_INSUFFICIENTDEST;
        break;
      }
      iCur+= iTemp; format++; iArg++;
      continue;
    }
    if(*format == 's')
    {
      if(iLengthMod == 2)
        pa_strc16Tostr_nolen((char*)args[iArg],(char16*)args[iArg]);
      if(pa_strncpyEx(&dest[iCur],(char*)args[iArg],(destLen-iCur),&iTemp) != 0)
      {
        retVal = -ERR_INSUFFICIENTDEST;
        break;
      }
      iCur+= iTemp; format++; iArg++;
      iLengthMod = 0;
      continue;
    }
    pa_printstrErr("pa_vsnprintf:ERR: unsupported format>>");
    pa_printstrErr(format);
    pa_printstrErr("<<\n");
    retVal = -ERR_UNSUPPORTEDFORMAT;
    break;
  }
  if(iCur >= destLen)
    iCur = destLen-1;
  if(iCur >= 0)
    dest[iCur] = (char)PA_NULL;
  return retVal;
}

int pa_vprintfEx(char *format,void *args[],char *dest,int destLen)
{
  int retVal;

  retVal = pa_vsnprintf(dest,destLen,format,args);
  if(retVal == 0)
    pa_printstr(dest);
  return retVal;
}

#ifndef PA_PERROR
int pa_perror(char *str)
{
  int iRet;
  iRet = pa_printstrErr(str);
  pa_printstrErr("\n");
  return iRet;
}
#endif

int pa_isspace(char iCur)
{
  if((iCur==' ')||(iCur=='\t')||(iCur=='\r')||(iCur=='\n'))
    return 1;
  return 0;
}

int pa_isdigit(char iCur)
{
  if((iCur>='0')&&(iCur<='9'))
    return 1;
  return 0;
}

void pa_getdatetime(int32* yearSince1900, uint8* month, uint8* day, 
       uint8* hr, uint8* min, uint8* sec)
{
  *yearSince1900 = 0; *month = 0; *day = 0;
  *hr = 0; *min = 0; *sec = 0;
#ifdef OS_LINUX
  {
  struct tm cTm;
  time_t curTime;
  time(&curTime);
  if(gmtime_r(&curTime,&cTm) != NULL)
  {
    *yearSince1900 = cTm.tm_year;
    *month = cTm.tm_mon+1;
    *day = cTm.tm_mday;
    *hr = cTm.tm_hour;
    *min = cTm.tm_min;
    *sec = cTm.tm_sec;
  }
  }
#endif
}

