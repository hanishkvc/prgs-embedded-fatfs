/*
 * pa_rand.c - portable pseudo random number generator and etal.
 * v17Jan2005_1225
 * C Hanish Menon <hanishkvc>, 1999-2005
 */
#include <stdio.h>
#include <rand.h>
#include <utilsporta.h>

#define PARAND_SUMCNT 7
int paRandSum[PARAND_SUMCNT] = { 0x55, 0xda, 0x3f, 0x12, 0xea, 0x7a, 0x15};

#define PARAND_INTTOSTRCNT 69
char paRandIntToStr[PARAND_INTTOSTRCNT] = { 
	'L','l','M','m',
	'a','9','B','A',
	'5','^','R','S',
	'X','W','z','3',
	'C','d','7','#',
	'E','f','G','h',
	'V','4','?','w',
	'I','J','i','j',
	'6','_','k','K',
	'n','o','P','Q',
	's','r','@','t',
	'e','F','g','H',
	'U','v','T','u',
	'x','2','Z','y',
	'Y','1','z','0',
	'8','c','b','D',
	'N','O','p','q',
	'&',
};

void pa_rand_init(randT *r, int init)
{
	r->randW = init;
	r->cnt=0;
	r->decCnt1 = PARAND_DECCNT1_INIT;
	r->decCnt2 = PARAND_DECCNT2;
	r->rand = r->randW;
}

void pa_rand_print(randT *r)
{
	int i;
	pa_printstr("RandT cnt["); pa_printint(r->cnt);
	pa_printstr("] decCnt1["); pa_printint(r->decCnt1);
	pa_printstr("] decCnt2["); pa_printint(r->decCnt2);
	pa_printstr("]\n");
	for(i=0;i<PARAND_SUMCNT;i++)
	{
		pa_printstr("paRandSum["); pa_printint(i);
		pa_printstr("]="); pa_printint(paRandSum[i]);
		pa_printstr("\n");
	}
}

void pa_rand_get(randT *r)
{
	int cur, curH, curL, cur2H, cur2L;
	int iCur;

	cur = r->randW;
	curH = (cur & 0xffff0000) >> 16;
	curL = (cur & 0x0000ffff);
	r->decCnt1--;
	r->decCnt2--;
	if(r->decCnt2 <= 0)
	{
		r->decCnt2 = curH % PARAND_DECCNT2;
		for(iCur=0;iCur<PARAND_SUMCNT;iCur++)
			paRandSum[iCur] += 1;
#ifdef PARAND_DEBUG		
		pa_rand_print(r);
#endif		
	}
	if(r->decCnt1 <= 0)
	{
		r->decCnt1 = curL % PARAND_DECCNT1_MAX;
		paRandSum[r->decCnt1%PARAND_SUMCNT] = curH%0xff;
		curH = curH-curL;
		curL = curL-curH;
	}
	else
	{
		cur = r->cnt%3;
		switch(cur)
		{
		case 0:
			curL = curL + paRandSum[r->cnt%PARAND_SUMCNT];
			break;
		case 1:
			curL = curL - paRandSum[r->cnt%PARAND_SUMCNT];
			curH = curH - paRandSum[r->cnt%PARAND_SUMCNT];
			break;
		case 2:
			cur2L = curL & 0x00ff;
			cur2H = (curL & 0xff00) >> 8;
			curL = (cur2L << 8) | cur2H;
			break;
		}
	}
	cur = (curL << 16) | curH;
	r->randW = cur;
	r->cnt++;
	r->rand = r->randW;
}

void pa_rand_getclipped(randT *r, unsigned int clip)
{
	pa_rand_get(r);
	r->rand = r->rand % clip;
}

void pa_rand_getstring(randT *r, char *rstr, int len)
{
	int iCur;
	
	for(iCur=0;iCur<len-1;iCur++)
	{
		pa_rand_getclipped(r,PARAND_INTTOSTRCNT);
		rstr[iCur] = paRandIntToStr[r->rand];
	}
	rstr[len-1] = (char)NULL;
}

