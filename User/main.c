/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : Jim Merkle
 * Version            : V1.0.0
 * Date               : 2024/08/22
 * Description        : Initialize command line and loop for command line input
 * Copyright (c) 2024 Jim Merkle
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/

/*
 *@Note
  Using peripherals / pins:
  UART: TX: PD5, RX: PD6
  GPIO: PD0 - LED
  I2C, SCL: PC2, SDA: PC1

*/

#include "debug.h"
#include "command_line.h"
#include "i2c.h"

// Function Prototypes
extern void USART_Printf_Init2(uint32_t baudrate); // debug2.c

/* Defines */

/* Global Variable */

/*********************************************************************
 * @fn      GPIO_Toggle_INIT
 *
 * @brief   Initializes GPIOA.0
 *
 * @return  none
 */
void GPIO_Toggle_INIT(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;  // toggle PD0
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
}

/*********************************************************************
 * @fn      main
 *
 * @brief   Main program.
 *
 * @return  none
 */
int main(void)
{
    u8 i = 0;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    Delay_Init();
    USART_Printf_Init2(115200); // Use alternate init function that includes RX pin
    printf("SystemClk:%d\n", SystemCoreClock);

    //printf("IIC Host mode, 100Kbps\r\n");
    IIC_Init( 100000, I2C_SELF_ADDRESS); // 80000 creates a nice looking 80KHz, 100K looks good too

    //printf("init toggle LED\n");
    GPIO_Toggle_INIT();

    // Initialize command line module
    cl_setup();

    while(1)
    {
        cl_loop(); // command line, check for input character
        Delay_Ms(40);
        GPIO_WriteBit(GPIOD, GPIO_Pin_0, (i == 0) ? (i = Bit_SET) : (i = Bit_RESET)); // toggle PD0
    }
}
