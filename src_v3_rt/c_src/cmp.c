/* cmp.c */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include "cmp.h"
#include "log.h"

/* exp(log(tone_num)/1.45) */
const double cmp_cnv[] = {
	1, 1, 1.61, 2.13, 2.60, 3.03,
	3.44, 3.83, 4.20, 4.55, 4.89 }; 
double log_cnv[17] = {0,};

void cmp_init(void)
{
	log_prt("end:cmp_init\n");
}

int compress(int fac, int tone_num, int in)
{
	int i;
#if 0
	int fac_mod = fac;

	if (fac > 768 && tone_num > 5){
		fac_mod = 768;
	}
	if (tone_num > 10){
		tone_num = 10;
	}
	return (double)fac_mod / 768 
		* cmp_cnv[tone_num] / tone_num * in;
#else
	/* initialize */
	if (0 == log_cnv[0])
	{
		for (i=0; i<CMP_FAC+1; i++){
			log_cnv[i] = 1;
		}
		for (; i<16+1; i++){
			log_cnv[i] = log(i) / log(CMP_FAC);
		}
	}
	return in / log_cnv[tone_num];
#endif
}
