/*
 * TouchCY8C201A0 - Library for CY8C201A0 CapSense Controller
 * (int fd, C) 2013 Akafugu Corporation
 *
 * This program is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (int fd, at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 */
#ifndef __TOUCH_H__ 
#define __TOUCH_H__ 

#include <stdint.h>

#define T_ADDR0 0x0
#define T_ADDR1 0x1

enum CY8_RegisterMap
{
  INPUT_PORT0  = 0x00,
  INPUT_PORT1  = 0x01,
  STATUS_PORT0 = 0x02,
  STATUS_PORT1 = 0x03,
  OUTPUT_PORT0 = 0x04,
  OUTPUT_PORT1 = 0x05,
  CS_ENABLE0 = 0x06,
  CS_ENABLE1 = 0x07,
  GPIO_ENABLE0 = 0x08,
  GPIO_ENABLE1 = 0x09,
  INVERSION_MASK0 = 0xA,
  INVERSION_MASK1 = 0xB,
  // 0x0C~17
  // 0x1c-
    
  CS_NOISE_TH = 0x4E,
  CS_BL_UPD_TH = 0x4F,

  CS_SCAN_POS_00 = 0x57,
  CS_SCAN_POS_01 = 0x58,
  CS_SCAN_POS_02 = 0x59,
  CS_SCAN_POS_03 = 0x5A,
  CS_SCAN_POS_04 = 0x5B,
  CS_SCAN_POS_10 = 0x5C,
  CS_SCAN_POS_11 = 0x5D,
  CS_SCAN_POS_12 = 0x5E,
  CS_SCAN_POS_13 = 0x5F,
  CS_SCAN_POS_14 = 0x60,

  CS_FINGER_TH_00 = 0x61,
  CS_FINGER_TH_01 = 0x62,
  CS_FINGER_TH_02 = 0x63,
  CS_FINGER_TH_03 = 0x64,
  CS_FINGER_TH_04 = 0x65,
  CS_FINGER_TH_10 = 0x66,
  CS_FINGER_TH_11 = 0x67,
  CS_FINGER_TH_12 = 0x68,
  CS_FINGER_TH_13 = 0x69,
  CS_FINGER_TH_14 = 0x6A,

  DEVICE_ID = 0x7A,
  DEVICE_STATUS = 0x7B,
  I2C_ADDR_DM = 0x7C,

  CS_SLID_CONFIG = 0x75,
  I2C_DEV_LOCK = 0x79,

  CS_READ_BUTTON = 0x81,
  CS_READ_BLM = 0x82,
  CS_READ_BLL = 0x83,
  CS_READ_DIFFM = 0x84,
  CS_READ_DIFFL = 0x85,
  CS_READ_RAWM = 0x86,
  CS_READ_RAWL = 0x87,
  CS_READ_STATUS0 = 0x88,
  CS_READ_STATUS1 = 0x89,
  COMMAND_REG = 0xA0,
};
///

  // Low level commands
  int t_i2cinit(int fd);
  int t_writeRegister(int fd, uint8_t reg);
  int t_writeCommand(int fd, uint8_t reg, uint8_t command);
  int t_writeCommandSequence(int fd, uint8_t reg, int* commands, uint8_t n);
  uint8_t t_receiveDataByte(int fd);
  void t_receiveData(int fd, uint8_t* buffer, uint8_t n);
  
  // High-level commands
  uint8_t t_getDeviceId(int fd);
  uint8_t t_getDeviceStatus(int fd);
  uint8_t t_getI2cDriveMode(int fd);
  uint8_t t_getCsEnb0(int fd );
  uint8_t t_getCsEnb1(int fd );
  uint8_t t_getGpioEnb0(int fd );
  uint8_t t_getGpioEnb1(int fd );
  void    t_getAllReg(int fd );
  int t_readReg16(int fd, uint8_t reg);

  void t_changeI2CAddress(int fd, uint8_t old_addr, uint8_t new_addr);
  
  void t_enterSetupMode(int fd);
  void t_enterNormalMode(int fd);
  
  void t_restoreFactoryDefault(int fd);
  void t_softwareReset(int fd);
  void t_saveToFlash(int fd);

  
  // GPIO / buttons / sliders configuration
  void t_setupGPIO(int fd, int gpio0, int gpio1);
  void t_setupCapSense(int fd, uint8_t cs0, uint8_t cs1);
  void t_setupInversion(int fd, int inv0, int inv1);

  uint8_t t_readReg(int fd, uint8_t reg);
  void t_selectBtn(int fd, uint8_t port, uint8_t gpx);
  
  // Read status
  uint8_t t_readStatus(int fd, uint8_t port);

#endif // __TOUCH_H__
