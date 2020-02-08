/* cmp2.h */

//benCompressor~ is a basic compression algorithm implemented as a puredata external.
//Ben Mangold

#ifndef __CMP2_H__
#define __CMP2_H__

#define	t_int		int
#define	t_intarg	int
#define	t_float		double
#define	t_floatarg	double
#define	post		printf
#define	t_sample	double

#define CMP_BUF_SIZE	32

typedef struct _benCompressor_tilde { //dataspace
  t_int sr; //samplerate
  
  t_float f_coeff;
//  t_sample f;
  t_float f_thresh;
  t_float f_ratio;
  t_float f_attack;
  t_float f_release;
  t_int attackFlag;
  t_float f_gain_change; // gain increment
  t_float f_gain_target;
  t_int i_fadesamps; 

} t_benCompressor_tilde;

void cmp_init(void);
short compress(short);

#endif /* __CMP2_H__ */
