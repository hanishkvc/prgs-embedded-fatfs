/*
 * pa_rand.c - portable pseudo random number generator and etal.
 * v16Jan2005_2035
 * C Hanish Menon <hanishkvc>, 1999-2005
 */
#ifndef _RAND_H_
#define _RAND_H_

typedef struct randT_
{
	unsigned int rand;
	unsigned int randW,cnt;
	int decCnt1,decCnt2;
} randT;

#define PARAND_DECCNT1_INIT 39
#define PARAND_DECCNT1_MAX 93
#define PARAND_DECCNT2 3232

void pa_rand_init(randT *r, int init);
void pa_rand_get(randT *r);
void pa_rand_getclipped(randT *r, unsigned int clip);
void pa_rand_getstring(randT *r, char *rstr, int len);
#endif

