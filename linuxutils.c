/*
 * linuxutils.c - library for linux related support routines
 * v07Nov2004_1101
 * C Hanish Menon <hanishkvc>, 14july2004
 * 
 */

#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>

#include <inall.h>
#include <errs.h>


int lu_nanosleeppaka(int secs, int nanosecs)
{
  struct timespec req, rem;
  
  req.tv_sec = secs;
  req.tv_nsec = nanosecs;
  do{
    if(nanosleep(&req,&rem) != 0)
    {
      if(errno == EINTR)
      {
        req.tv_sec = rem.tv_sec;
	req.tv_nsec = rem.tv_nsec;
#if DEBUG_GENERIC > 25	
	printf("INFO: nanosleep interrupted, but as been recovered\n");
#endif	
      }
      else
      {
#if DEBUG_GENERIC > 25	
        fprintf(stderr,"ERR: nanosleep failed\n");
#endif	
	return -ERROR_FAILED;
      }
    }
    else
      return 0;
  }while(1);
  return -ERROR_INVALID;
}

void lu_starttime(struct timeval *tv1)
{
  gettimeofday(tv1, NULL);
}

void lu_stoptimedisp(struct timeval *tv1,struct timeval *tv2, 
       long int *timeInUSECS, char *sPrompt)
{
  gettimeofday(tv2, NULL);
  if((tv2->tv_sec-tv1->tv_sec) == 0)
  {
    *timeInUSECS =  tv2->tv_usec - tv1->tv_usec;
  }
  else
  {
    *timeInUSECS = 1000000-tv1->tv_usec;
    *timeInUSECS += tv2->tv_usec;
    *timeInUSECS += (tv2->tv_sec-tv1->tv_sec-1)*1000000;
  }
  if(sPrompt != NULL)
  {
  fprintf(stderr,"*** [%s] TimeSpent [%ld]USECS ***\n",sPrompt,*timeInUSECS);
#ifdef PRG_MODE_DM270
  fprintf(stderr,"DM270:03Nov2004-NNOOOOTTTTTTEEEEEE-CPU Time fast by ~2.5\n");
#endif
  fprintf(stderr,"tv1 [%ld sec: %ld usec] tv2 [%ld sec: %ld usec]\n", 
    tv1->tv_sec, tv1->tv_usec, tv2->tv_sec, tv2->tv_usec);
  }
}

