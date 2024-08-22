/*
 * i2c.c
 *
 *  Created on: Aug 19, 2024
 *      Author: Jim Merkle
 */
#include "debug.h"
#include "i2c.h"

/*********************************************************************
 * @fn      IIC_Init
 *
 * @brief   Initializes the IIC peripheral.
 *
 * @return  none
 */
void IIC_Init(u32 bound, u16 address)
{
    GPIO_InitTypeDef GPIO_InitStructure={0};
    I2C_InitTypeDef I2C_InitTStructure={0};

    RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE );
    RCC_APB1PeriphClockCmd( RCC_APB1Periph_I2C1, ENABLE );

    // Init pin for SCL (PC2)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init( GPIOC, &GPIO_InitStructure );

    // Init pin for SDA (PC1)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init( GPIOC, &GPIO_InitStructure );

    I2C_InitTStructure.I2C_ClockSpeed = bound;
    I2C_InitTStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitTStructure.I2C_DutyCycle = I2C_DutyCycle_16_9;
    I2C_InitTStructure.I2C_OwnAddress1 = address;
    I2C_InitTStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitTStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_Init( I2C1, &I2C_InitTStructure );

    I2C_Cmd( I2C1, ENABLE );

}

// Spin, waiting for I2C module to become Not Busy (Both SCL and SDA are high)
// Return I2C_ERR_SUCCESS or I2C_ERR_BUSY
// Using loop counter for time-out (a bit of a hack, but doesn't use a timer)
int i2c_wait_not_busy(void)
{
    uint32_t count = 0;
    while ( I2C_GetFlagStatus( I2C1, I2C_FLAG_BUSY ) != RESET) {
        if(++count >= I2C_BUSY_LOOPS)
            return I2C_ERROR_BUSY;
    }
    return I2C_ERROR_SUCCESS;
}

// Spin, waiting for I2C module to enter master mode
// Return I2C_ERR_SUCCESS or I2C_ERROR_TIME_OUT
// Using loop counter for time-out (a bit of a hack, but doesn't use a timer)
int i2c_wait_master_mode(void)
{
    uint32_t count = 0;
    while( !I2C_CheckEvent( I2C1, I2C_EVENT_MASTER_MODE_SELECT ) ) {
        if(++count >= I2C_MASTER_MODE_LOOPS)
            return I2C_ERROR_TIME_OUT;
    }
    return I2C_ERROR_SUCCESS;
}

// Spin, waiting for I2C module to complete sending byte (either address byte or data byte)
// Return I2C_ERR_SUCCESS or I2C_ERROR_TIME_OUT
// Using loop counter for time-out (a bit of a hack, but doesn't use a timer)
// I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED : address sent
// I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED : address sent (transition to receive)
// I2C_EVENT_MASTER_BYTE_TRANSMITTED : data byte sent
int i2c_wait_transmit_complete(void)
{
    uint32_t count = 0;
    while(1) {
        uint32_t i2c_status = I2C_GetLastEvent(I2C1);
        if(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED == i2c_status) break;
        if(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED == i2c_status) break;
        if(I2C_EVENT_MASTER_BYTE_TRANSMITTED == i2c_status) break;
        if(++count >= I2C_TRANSMIT_COMPLETE_LOOPS) return I2C_ERROR_TIME_OUT;
    }
    return I2C_ERROR_SUCCESS;
}

// Spin, waiting for I2C module to receive a byte
// Return I2C_ERR_SUCCESS or I2C_ERROR_TIME_OUT
// Using loop counter for time-out (a bit of a hack, but doesn't use a timer)
int i2c_wait_master_receiver_mode(void)
{
    uint32_t count = 0;
    while( !I2C_CheckEvent( I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED ) ) {
        if(++count >= I2C_MASTER_RECEIVER_LOOPS)
            return I2C_ERROR_TIME_OUT;
    }
    return I2C_ERROR_SUCCESS;
}


// Spin, waiting for I2C module's transmitter to empty
// Return I2C_ERR_SUCCESS or I2C_ERROR_TIME_OUT
// Using loop counter for time-out (a bit of a hack, but doesn't use a timer)
int i2c_wait_transmit_empty(void)
{
    uint32_t count = 0;
    while( I2C_GetFlagStatus( I2C1, I2C_FLAG_TXE ) ==  RESET ) {
        if(++count >= I2C_TRANSMIT_EMPTY_LOOPS)
            return I2C_ERROR_TIME_OUT;
    }
    return I2C_ERROR_SUCCESS;
}


// Assuming I2C transmitter is ready to transmit, send a byte and wait
// for transmitter to empty, allowing another byte to follow
int i2c_send_byte(uint8_t data)
{
    //printf("send 0x%02X\n",data);
    I2C1->DATAR = data;

    //printf("Wait for master transmitter mode\n");
    I2C_ERROR rc = i2c_wait_transmit_complete();
    if(I2C_ERROR_SUCCESS != rc) {
        //uint32_t status = I2C_GetLastEvent(I2C1);
        //printf("%s i2c_wait_transmit_complete() 0x%06X, %d\n",__func__, status, rc);
        return rc;
    }
    return I2C_ERROR_SUCCESS;
}

// Assuming I2C receiver is ready to receive, begin receiving a byte and wait
// for receiver to fill, then transfer byte to buffer
int i2c_read_byte(uint8_t * data)
{
    //printf("Wait for master receiver mode\n");
    I2C_ERROR rc = i2c_wait_master_receiver_mode();
    if(I2C_ERROR_SUCCESS != rc) {
        uint32_t status = I2C_GetLastEvent(I2C1);
        printf("%s i2c_wait_master_receiver_mode() 0x%06X, %d\n",__func__, status, rc);
        return rc;
    }
    // Transfer byte to buffer
    *data = I2C1->DATAR;
    //printf("%s, 0x%02X\n",__func__, *data);

    return I2C_ERROR_SUCCESS;
}

// Given 7-bit address, pointer to data, and count, send data to I2C peripheral
// Returns I2C_ERROR code
int i2c_write(uint16_t i2c_address, uint8_t * data, uint8_t count)
{
    //printf("Wait for Not Busy\n");
    I2C_ERROR rc = i2c_wait_not_busy();
    if(I2C_ERROR_SUCCESS != rc) return rc;

    //printf("Start\n");
    I2C_GenerateSTART( I2C1, ENABLE );

    //printf("Wait for master mode\r\n");
    rc = i2c_wait_master_mode();
    if(I2C_ERROR_SUCCESS != rc) return rc;

    //printf("Send Address\n");
    rc = i2c_send_byte(i2c_address<<1); // R/W bit clear
    if(I2C_ERROR_SUCCESS != rc) return rc;

    // Send data bytes
    while(count) {
        rc = i2c_send_byte(*data);
        if(I2C_ERROR_SUCCESS != rc) return rc;
        data++;
        count--;
    } // while

    //printf("Stop\r\n");
    I2C_GenerateSTOP( I2C1, ENABLE );

    //printf("%s Done\r\n",__func__);
    return I2C_ERROR_SUCCESS;
} // i2c_write()

// Given 7-bit address, pointer for data, and count, read data from I2C peripheral
// Returns I2C_ERROR code
// Added ACK bit support to NAK last byte
int i2c_read(uint16_t i2c_address, uint8_t * data, uint8_t count)
{
    I2C1->CTLR1 |= (1<<10); // Set ACK bit
    //printf("Wait for Not Busy\n");
    I2C_ERROR rc = i2c_wait_not_busy();
    if(I2C_ERROR_SUCCESS != rc) return rc;

    //printf("Start\n");
    I2C_GenerateSTART( I2C1, ENABLE );

    //printf("Wait for master mode\r\n");
    rc = i2c_wait_master_mode();
    if(I2C_ERROR_SUCCESS != rc) return rc;

    //printf("Send Address\n");
    rc = i2c_send_byte((i2c_address<<1) | 1); // R/W bit set
    if(I2C_ERROR_SUCCESS != rc) return rc;

    // Read data bytes
    while(count) {
        // If this is the last byte, NAK it
        if(count == 1) I2C1->CTLR1 &= ~(1<<10); // Clear ACK bit before data is read from DATAR
        rc = i2c_read_byte(data);
        if(I2C_ERROR_SUCCESS != rc) return rc;
        data++;
        count--;
    } // while

    //printf("Stop\n");
    I2C_GenerateSTOP( I2C1, ENABLE );

    //printf("%s Done\n",__func__);
    return I2C_ERROR_SUCCESS;
} // i2c_read()

// Given a 7-bit I2C address, initiate a READ from device, looking ACK in response
// to the device's address.  If device is present and provides ACK, return I2C_ERROR_SUCCESS,
// else return I2C_ERROR_ACK.
int i2c_device_detect(uint16_t i2c_address)
{
    I2C1->CTLR1 |= (1<<10); // Set ACK bit
    //printf("%s: Wait for Not Busy\n",__func__);
    I2C_ERROR rc = i2c_wait_not_busy();
    if(I2C_ERROR_SUCCESS != rc) return rc;

    //printf("Start\n");
    I2C_GenerateSTART( I2C1, ENABLE );

    //printf("Wait for master mode\r\n");
    rc = i2c_wait_master_mode();
    if(I2C_ERROR_SUCCESS != rc) return rc;

    // i2c_send_byte() will error if address isn't ACK'ed
    //printf("Send Address\n");
    rc = i2c_send_byte((i2c_address<<1) | 1); // R/W bit set

    // If Ack Failure bit set...
    uint16_t star1 = I2C1->STAR1;
    if(star1 & I2C_FLAG_AF) {
        star1 &= ~I2C_FLAG_AF;
        I2C1->STAR1 = star1; // clear AF flag
        rc = I2C_ERROR_ACK;
    }

    I2C1->CTLR1 &= ~(1<<10); // Clear ACK bit (REQUIRED!)

    //printf("%s: Stop\n",__func__);
    I2C_GenerateSTOP( I2C1, ENABLE );

    //printf("%s Done\r\n",__func__);
    return rc == I2C_ERROR_SUCCESS ? I2C_ERROR_SUCCESS : I2C_ERROR_ACK;
} // i2c_device_detect()

// Create console display showing I2C devices present via an address map,
// similar to the following produced by Linux's i2cdetect command:
//      0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
// 00:          -- -- -- -- -- -- -- -- -- -- -- -- --
// 10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
// 20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
// 30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
// 40: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
// 50: -- -- -- -- -- -- 56 -- -- -- -- -- -- -- -- --
// 60: -- -- -- -- -- -- -- -- 68 -- -- -- -- -- -- --
// 70: -- -- -- -- -- -- -- ¡ª-
void i2c_scan(void)
{
    printf("     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");
    printf("00:          ");
    for(unsigned address=3; address<=0x77; address++) {
        // print row header, starting row address
        if((address % 0x10) == 0) printf("\n%02X: ",address);
        if(I2C_ERROR_SUCCESS == i2c_device_detect(address))
            printf("%02X ",address);
        else
            printf("-- ");

    } // for-loop
    printf("\n");
}
