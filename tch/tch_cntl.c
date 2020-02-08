#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include "touch.h"

extern uint16_t	key16;
uint8_t		t_btn[4];

int		t_fd[2];
const int32_t	t_lay[][3]= {
	{ 0x1<<0,	0x1<<7,	-1 },
	{ 0x1<<1,	-1,	2 },
	{ 0x1<<2,	0x1<<4,-1 },
	{ 0x1<<3,	0x1<<5,-1 },
	{ 0x1<<4,	0x1<<6,-1 },
	{ 0x1<<8,	-1,	1 },
	{ 0x1<<9,	-1,	0 },
	{ 0x1<<10,	0x1<<1,-1 },
	{ 0x1<<11,	0x1<<2,-1 },
	{ 0x1<<12,	0x1<<3,-1 },

	{ 0x1<<16,	0x1<<9,	-1 },
	{ 0x1<<17,	0x1,	-1 },
	{ 0x1<<18,	0x1<<12,-1 },
	{ 0x1<<19, 	0x1<<11,-1 },
	{ 0x1<<20,	0x1<<10,-1 },
	{ 0x1<<24,	0x1<<8,	-1 },
	{ 0x1<<25,	-1,	3 },
	{ 0x1<<26,	0x1<<15,-1 },
	{ 0x1<<27, 	0x1<<14,-1 },
	{ 0x1<<28,	0x1<<13,-1 },
};

void t_conv(uint32_t val)
{
  int i;
  uint16_t	key = 0;
  uint8_t	btn[4] = {0,0,0,0};

  for (i=0; i< 20; i++){
    if (t_lay[i][1] != -1){ /* KEY */
      if ( 0 != (t_lay[i][0] & val)){ /* set bit */
        key |= t_lay[i][1];
      }
    }else{ /* BTN */
      if ( 0 != (t_lay[i][0] & val)){ /* set bit */
        btn[t_lay[i][2]] = 1;
      }
    }
  }
  /* set global variable */
  key16 = key;
  for (i=0; i< 4; i++){
    t_btn[i] = btn[i];
  }
}

void get_t_val(uint16_t *pkey16, uint8_t *pbtn)
{
  uint32_t ret[2] = {0, 0};
  int i;
  uint32_t val;
  uint16_t	key = 0;
  uint8_t	btn[4] = {0,0,0,0};

  for (i=0; i<2; i++){
      ret[i] =  0b0001111100011111 & t_readReg16(t_fd[i], INPUT_PORT0);
  }
  val = t_conv(ret[0] | (ret[1] << 16));

  for (i=0; i< 20; i++){
    if (t_lay[i][1] != -1){ /* KEY */
      if ( 0 != (t_lay[i][0] & val)){ /* set bit */
        key |= t_lay[i][1];
      }
    }else{ /* BTN */
      if ( 0 != (t_lay[i][0] & val)){ /* set bit */
        btn[t_lay[i][2]] = 1;
      }
    }
  }
  /* set global variable */
  *pkey16 = key;
  for (i=0; i< 4; i++){
    pbtn[i] = btn[i];
  }
}

void t_init(void)
{
  int i;

  for (i=0; i<2; i++){
    t_fd[i] = t_i2cinit(i);
  }
#if 0
  while(1){
    usleep(2000*1000);
    get_t_val();
    printf("key:0x%04x, btn:%1d,%1d,%1d,%1d\n",
	key16, t_btn[0], t_btn[1], t_btn[2], t_btn[3]);
  }
#endif

  return 0;
}
