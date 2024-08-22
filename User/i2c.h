/*
 * i2c.h
 *
 *  Created on: Aug 19, 2024
 *      Author: User
 */

#ifndef USER_I2C_H_
#define USER_I2C_H_

#include "ch32v00x_i2c.h"

#define I2C_SELF_ADDRESS  0x06   // For host mode, this isn't necessary.  Provided for APIs that want self address.

typedef enum {
    I2C_ERROR_SUCCESS  =  0,
    I2C_ERROR_BUSY     = -1,  // Expected not busy (both SCL and SDA high) - missing pull-ups?
    I2C_ERROR_ACK      = -2,  // Expected ACK, none received before timeout
    I2C_ERROR_TIME_OUT = -3,
} I2C_ERROR;

#define I2C_BUSY_LOOPS                  10000
#define I2C_MASTER_MODE_LOOPS           10000
#define I2C_TRANSMIT_COMPLETE_LOOPS     10000
#define I2C_TRANSMIT_EMPTY_LOOPS        10000
#define I2C_MASTER_RECEIVER_LOOPS       10000


void IIC_Init(u32 bound, u16 address);
int i2c_write(uint16_t i2c_address, uint8_t * data, uint8_t count);
int i2c_read(uint16_t i2c_address, uint8_t * data, uint8_t count);
int i2c_device_detect(uint16_t i2c_address);
void i2c_scan(void);

#endif /* USER_I2C_H_ */
