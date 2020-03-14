#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include "touch.h"

uint8_t		t_btn[4];
unsigned int t_func_no = 0;

int		t_fd[2];
const int32_t	t_lay[][3]= {
	{ 0x1<<0,	0x1<<7,	-1 },
	{ 0x1<<1,	-1,	1 },
	{ 0x1<<2,	0x1<<4,-1 },
	{ 0x1<<3,	0x1<<5,-1 },
	{ 0x1<<4,	0x1<<6,-1 },
	{ 0x1<<8,	-1,	2 },
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

void t_init(void);

void t_reg_dump(void)
{
  int i, addr = 0;

  t_init();
  printf("touch sensor reg. dump\n");
  for (i = 0; i < 2; i++){
    printf("dev:%d\n", i);
    for (addr=0; addr < 0x8C; addr++){
      if (0 == addr){
        printf("%02X: ", addr);
      }else if (0 == (addr % 0x10) ){
        printf("\n%02X: ", addr);
      } else if (0 == (addr % 0x4) ){
        printf("- ");
      }
      printf("%02x ", t_readReg(t_fd[i], addr) );
    }
    printf("\n");
  }
}
void t_set_prm(int i)
{
  t_init();
  printf("t_set_prm: dev:%d\n", i);

  	// Device setup
	t_enterSetupMode(t_fd[i] );
  	t_restoreFactoryDefault(t_fd[i] );

  	// I/O port setup
	  usleep(150*1000);
  	t_setupGPIO(t_fd[i], 0x0, 0x0); // disable both GPIO ports
	// enable capsense for GP0[0:4] and GP1[0:4]
  	t_setupCapSense(t_fd[i], 0b00011111, 0b00011111);

  	t_writeCommand(t_fd[i], CS_NOISE_TH, 0x10);  // noise threshold
  	t_writeCommand(t_fd[i], CS_BL_UPD_TH, 0x0F); // bucket
  
  	t_writeCommand(t_fd[i], CS_FINGER_TH_00, 0x32);
  	t_writeCommand(t_fd[i], CS_FINGER_TH_01, 0x32);
  	t_writeCommand(t_fd[i], CS_FINGER_TH_02, 0x32);
  	t_writeCommand(t_fd[i], CS_FINGER_TH_03, 0x32);
  	t_writeCommand(t_fd[i], CS_FINGER_TH_04, 0x32);
 	t_writeCommand(t_fd[i], CS_FINGER_TH_10, 0x32);
  	t_writeCommand(t_fd[i], CS_FINGER_TH_11, 0x32);
  	t_writeCommand(t_fd[i], CS_FINGER_TH_12, 0x32);
  	t_writeCommand(t_fd[i], CS_FINGER_TH_13, 0x32);
  	t_writeCommand(t_fd[i], CS_FINGER_TH_14, 0x32);

  	t_saveToFlash(t_fd[i]);
  	usleep(120*1000);
  	t_softwareReset(t_fd[i]);
  	usleep(50*1000);
	t_enterNormalMode(t_fd[i]);
	usleep(100*1000);
}
int pre_t_proc(void)
{
	int ret = 1;

	switch(t_func_no){
	case 1:
	  t_reg_dump();
          break;
	case 90:
	  t_set_prm(0);
          break;
	case 91:
	  t_set_prm(1);
          break;
	default:
	  ret = 0;
	  break;
	}
	return ret;
}
void get_t_val(uint16_t *pkey16)
{
  uint32_t ret[2] = {0, 0};
  int i;
  uint32_t val;
  uint16_t	key = 0;
  uint8_t	btn[4] = {0,0,0,0};

  for (i=0; i<2; i++){
      ret[i] =  0b0001111100011111 & t_readReg16(t_fd[i], INPUT_PORT0);
  }
  val = ret[0] | (ret[1] << 16);

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
    t_btn[i] = btn[i];
  }
}

void t_init(void)
{
  int i;
  static const int addr[]={T_ADDR0, T_ADDR1};

  for (i=0; i<2; i++){
    t_fd[i] = t_i2cinit(addr[i]);
  }
#if 0
  while(1){
    usleep(2000*1000);
    get_t_val();
    printf("key:0x%04x, btn:%1d,%1d,%1d,%1d\n",
	key16, t_btn[0], t_btn[1], t_btn[2], t_btn[3]);
  }
#endif

}
