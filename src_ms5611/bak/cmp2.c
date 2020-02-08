/* cmp2.c */
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "cmp2.h"

void benCompressor_tilde_new(t_benCompressor_tilde *x);
double benCompressor_tilde_perform(short in_arg);      

t_benCompressor_tilde bencomp;

void cmp_init(void)
{
	benCompressor_tilde_new(&bencomp);
}

t_float benCompressor_tilde_rms (double in)
{
	int n = CMP_BUF_SIZE;	
	t_float rms;
	t_int i;
	static t_float sum = 0.0;
	static t_sample blockCopy[CMP_BUF_SIZE];
	static int loc = 0;
	static int cnt = 0;

	/* calculate sum again */
	if (loc == 0 && cnt != 0){
		sum = 0;
		for (i=0; i<n; i++){
			sum += blockCopy[i];
		}
		sum -= blockCopy[loc];
	}else{ /* calculate from difference */
		if (cnt == CMP_BUF_SIZE){ /* after one loop */
			sum -= blockCopy[loc];
		}else{
		/* less than one loop of blockCopy */
			if (cnt == 0){
				memset(blockCopy, 0, sizeof(blockCopy));
			}
			cnt++;
			n = cnt;
		}
	}
	blockCopy[loc] = in * in;
	sum += blockCopy[loc];
	loc++;
	loc %= CMP_BUF_SIZE;
	
	rms = sum/(t_float)n;
	rms = sqrt(rms);	
	
	return(rms);
}

void benCompressor_tilde_print (t_benCompressor_tilde *x){

	post("THRESH: %f", x->f_thresh);
	post("RATIO: %f", x->f_ratio);
	post("ATTACK: %f", x->f_attack);
	post("RELEASE: %f", x->f_release);

}

void benCompressor_tilde_thresh (t_benCompressor_tilde *x, t_floatarg thresh){
	x->f_thresh = thresh;
}

void benCompressor_tilde_ratio (t_benCompressor_tilde *x, t_floatarg ratio){
	
	if(ratio < 1){
		ratio = 1;
	}
	
	x->f_ratio = ratio;
	post("ratio change: %f",x->f_ratio);
}

void benCompressor_tilde_attack (t_benCompressor_tilde *x, t_floatarg attack){
	
	if(attack < 1){
		attack = 1;
	}
	
	
	x->f_attack = attack;
	post("attack change: %f",x->f_attack);
}

void benCompressor_tilde_release (t_benCompressor_tilde *x, t_floatarg release){
	
	if (release < 1){
		release = 1;
	}
	
	x->f_release = release;
	post("release change: %f",x->f_release);
}

//Ramp function to be used in attack and release
void benCompressor_tilde_rampcmd (t_benCompressor_tilde *x, t_floatarg target, t_floatarg time){
	
	t_float ampdif;  //
	t_int fadesamps; //fadetime in samples

	x->f_gain_target = (target<0)?0.0:target;
	x->f_gain_target = (x->f_gain_target>1)?1.0:x->f_gain_target;

//	post("target: %f, time: %f", x->f_gain_target, time);

	ampdif = x->f_gain_target - x->f_coeff;
	fadesamps  = x->sr * (time/1000);
	
	x->f_gain_change = ampdif/(t_float)(fadesamps-1);
	x->i_fadesamps = fadesamps;
	
}

short compress(short in)
{
	double ret;

	ret = benCompressor_tilde_perform(in);          
	return ret;
}

// DSP Perform Routine
double benCompressor_tilde_perform(short in_arg )          
{

	t_benCompressor_tilde *x = &bencomp;
	t_sample	in_val = ((double)in_arg)/32768; /* normalize */
	t_sample	out_val;
	t_sample  *in =    &in_val;
	t_sample  *out =   &out_val;
	t_float current_rms = 0;
#if 0	
	t_int n = (int)(w[4]);
#else
	t_int n = 1;		/* for khaen */
#endif
	
	
	current_rms = benCompressor_tilde_rms(in_val);
//	post("RMS: %f", current_rms);

// calculate a target amp if we're over the threshold. use the current attack time provided by the user as the ramp duration

/*
if attackFlag == 0 && RMS > thresh,
	attackFlag =1;
	ramp gain down;
else if attackFlag ==1 && RMS > thresh,
	adjust gain;
else if RMS < thresh,
	attackFlag = 0;
	ramp gain back to unity
*/

	if(current_rms > x->f_thresh && x->attackFlag == 0)
	{
		t_float overage, targetAmp;
		
		x->attackFlag = 1;

		overage = current_rms - x->f_thresh;
		overage /= x->f_ratio;
		targetAmp = x->f_thresh + overage;

		// rampcmd function takes amplitude in RMS, time in ms
		benCompressor_tilde_rampcmd(x, targetAmp/current_rms, x->f_attack);
	}
	else if(current_rms > x->f_thresh && x->attackFlag == 1)
	{

		t_float overage, targetAmp;

		overage = current_rms - x->f_thresh;
		overage /= x->f_ratio;
		targetAmp = x->f_thresh + overage;

		// rampcmd function takes amplitude in RMS, time in ms
		benCompressor_tilde_rampcmd(x, targetAmp/current_rms, x->f_attack);
	}
	else if(current_rms < x->f_thresh)
	{

		// rampcmd function takes amplitude in RMS, time in ms
		benCompressor_tilde_rampcmd(x, 1.0, x->f_release);
		x->attackFlag = 0;
	}
	
	t_sample coeff = x->f_coeff;
	
	while (n--){
		if(x->i_fadesamps-- > 1)
			coeff = coeff + x->f_gain_change;
		else
		{
			x->i_fadesamps = 0;
			coeff = x->f_gain_target;
		}

		
		// makeup gain stage here	
		*out++ = (*in++) * coeff;		
	}
	
	x->f_coeff = coeff;

	return out_val;
}

#if 0
// Adds DSP perform routine
void benCompressor_tilde_dsp(t_benCompressor_tilde *x, t_signal **sp)   //(pointer to class-dataspace, pointer to an array of signals)
{
  //Adds dsp perform routine to the DSP-tree. dsp_add(dsp perform routine, # of following pointers, sp[0] is first in-signal, sp[1] is second in-signal,sp[3] points to out-signal)

	dsp_add(
		benCompressor_tilde_perform,
		4,
		x,
		sp[0]->s_vec,
		sp[1]->s_vec,
		sp[0]->s_n
	);
	x->sr = sp[0]->s_sr;
	x->n = sp[0]->s_n;

}
#endif

void benCompressor_tilde_new(t_benCompressor_tilde *x)
{
	x->sr = 48000;			/* sample rate */
	x->f_thresh = 0.05;		/* rms value */
	x->f_ratio = 10;		/* samples */

	x->f_attack = 10;		/* ms */
	x->f_release = 10;		/* ms */
	
	x->f_coeff = 1.0;
	x->f_gain_target = x->f_coeff;

	x->f_gain_change=0;
	x->attackFlag = 0;
}
