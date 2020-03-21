/*
 * TouchCY8C201A0 - Library for CY8C201A0 CapSense Controller
 *
 * This program is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 */
#include <stdio.h>
#include <unistd.h>
#include <wiringPiI2C.h>
#include <i2c/smbus.h>
#include "touch.h"

int t_i2cinit(int addr)
{
  return wiringPiI2CSetup(addr);
}
  
// Low level commands
int t_writeRegister(int fd, uint8_t reg)
{
  return wiringPiI2CReadReg8(fd, reg);
}

int t_writeCommand(int fd, uint8_t reg, uint8_t command)
{
  return wiringPiI2CWriteReg8 (fd, reg, command); 
}

int t_readReg16(int fd, uint8_t reg)
{
  return wiringPiI2CReadReg16(fd, reg); 
}

void t_getAllReg(int fd)
{
  int i;

  usleep(10*1000);
  for (i=0; i<0x7f; i++){
    printf("%02x ", wiringPiI2CReadReg8(fd, i) );
    switch(i % 16){
      case 7:
	printf(" ");
        break;
      case 15:
	printf("\n");
        break;
    }
  }
  printf("\n");
}

uint8_t t_getDeviceId(int fd )
{
  return (uint8_t)wiringPiI2CReadReg8 (fd, DEVICE_ID);
}

uint8_t t_getDeviceStatus(int fd )
{
  return (uint8_t)wiringPiI2CReadReg8 (fd, DEVICE_STATUS);
}

uint8_t t_getI2cDriveMode(int fd )
{
  return (uint8_t)wiringPiI2CReadReg8 (fd, I2C_ADDR_DM);
}

uint8_t t_getCsEnb0(int fd )
{
  return (uint8_t)wiringPiI2CReadReg8 (fd, CS_ENABLE0);
}

uint8_t t_getCsEnb1(int fd )
{
  return (uint8_t)wiringPiI2CReadReg8 (fd, CS_ENABLE1);
}

uint8_t t_getGpioEnb0(int fd )
{
  return (uint8_t)wiringPiI2CReadReg8 (fd, GPIO_ENABLE0);
}

uint8_t t_getGpioEnb1(int fd )
{
  return (uint8_t)wiringPiI2CReadReg8 (fd, GPIO_ENABLE1);
}
void t_checkUnlock(int fd)
{
  const unsigned char unlock[] = {0x3C, 0xA5, 0x69};

  // unlock
  printf("write ret:%d\n",
    i2c_smbus_write_i2c_block_data(fd, I2C_DEV_LOCK, 0x3, unlock) );
  usleep(200);
  printf("I2C_DEV_LOCK:0x%02x\n", 
  	wiringPiI2CReadReg8(fd, I2C_DEV_LOCK) );
} 
void t_checkLock(int fd)
{
  const unsigned char lock[3] = {0x96, 0x5A, 0xC3};

  // lock
  printf("write ret:%d\n",
    i2c_smbus_write_i2c_block_data(fd, I2C_DEV_LOCK, 0x3, lock) );
  usleep(200);
  printf("I2C_DEV_LOCK:0x%02x\n", 
  	wiringPiI2CReadReg8(fd, I2C_DEV_LOCK) );
} 
void t_changeI2CAddress(int fd, uint8_t old_addr, uint8_t new_addr)
{
  printf("Before:I2C_ADDR_DM:0x%02x\n", 
  	wiringPiI2CReadReg8(fd, I2C_ADDR_DM) );

  // unlock
  t_checkUnlock(fd);

  // change address and i2c_open_drain
  t_writeCommand(fd, I2C_ADDR_DM, (0b10000000|new_addr));
  usleep(1000);

  // lock
  t_checkLock(fd);

  printf("after:I2C_ADDR_DM:0x%02x\n", 
  	wiringPiI2CReadReg8(fd, I2C_ADDR_DM) );
}

void t_enterSetupMode(int fd )
{
  t_writeCommand(fd, COMMAND_REG, 0x08); // enter setup mode
  usleep(2*1000);
}

void t_enterNormalMode(int fd )
{
  t_writeCommand(fd, COMMAND_REG, 0x07); // enter normal mode  
}

void t_restoreFactoryDefault(int fd )
{
  t_writeCommand(fd, COMMAND_REG, 0x02); // restore factory defaults
  usleep(120*1000);
}

void t_softwareReset(int fd )
{
  t_writeCommand(fd, COMMAND_REG, 0x08); // enter setup mode
  t_writeCommand(fd, COMMAND_REG, 0x06); // software reset
}

void t_saveToFlash(int fd )
{
  t_writeCommand(fd, COMMAND_REG, 0x01); // save to flash
}

void t_setupInversion(int fd, int inv0, int inv1)
{
  t_writeCommand(fd, INVERSION_MASK0, inv0);
  t_writeCommand(fd, INVERSION_MASK1, inv1);
}

void t_setupGPIO(int fd, int gpio0, int gpio1)
{
  t_writeCommand(fd, GPIO_ENABLE0, gpio0);
  usleep(11*1000);
  t_writeCommand(fd, GPIO_ENABLE1, gpio1);
  usleep(11*1000);
}

void t_setupCapSense(int fd, uint8_t cs0, uint8_t cs1)
{
#if 0
  //uint8_t cmd[]= { cs0, cs1 };
  //t_writeCommandSequence(fd, CS_ENABLE0, cmd, 2);
#else
  t_writeCommand(fd, CS_ENABLE0, cs0);
  usleep(12*1000);
  t_writeCommand(fd, CS_ENABLE1, cs1); 
  usleep(12*1000);
#endif
}

void t_selectBtn(int fd, uint8_t port, uint8_t gpx)
{
  t_writeCommand(fd, CS_READ_BUTTON, ((port & 0x1)<<7)| (0x1<< gpx) );
}

uint8_t t_readReg(int fd, uint8_t reg)
{
  return (uint8_t)t_writeRegister(fd, reg);    
}

uint8_t t_readStatus(int fd, uint8_t port)
{
  uint8_t ret;

  if (port == 0)
    ret = t_writeRegister(fd, CS_READ_STATUS0);
  else
    ret = t_writeRegister(fd, CS_READ_STATUS1);
    
  return ret;
}

