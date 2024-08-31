# CH32V003_command_line

Example Command Line project for the CH32V003

### Getting Started

        The base code provided for the nanoCH32V003 provides a good
        starting point for a project.
        To better use this base code, the developer needs to define
        their clock source and if appropriate, their USART pin
        configuration.
        1) File: system_ch32v00x.c, define clock source by uncommenting
        the appropriate #define.
        In my case, since my development board had no external crystal,
        I chose the following for HSI, 48MHz:
        
        //#define SYSCLK_FREQ_8MHz_HSI    8000000
        //#define SYSCLK_FREQ_24MHZ_HSI   HSI_VALUE
        #define SYSCLK_FREQ_48MHZ_HSI   48000000
        //#define SYSCLK_FREQ_8MHz_HSE    8000000
        //#define SYSCLK_FREQ_24MHz_HSE   HSE_VALUE
        //#define SYSCLK_FREQ_48MHz_HSE   48000000
        
        2) File: debug.h, Define debug USART pins
        I chose no remapping
        /* UART Printf Definition */
        #define DEBUG_UART1_NoRemap   1  //Tx-PD5  Rx-PD6
        #define DEBUG_UART1_Remap1    2  //Tx-PD0  Rx-PD1
        #define DEBUG_UART1_Remap2    3  //Tx-PD6  Rx-PD5
        #define DEBUG_UART1_Remap3    4  //Tx-PC0  Rx-PC1
        
        /* DEBUG USART Definition */
        #ifndef DEBUG
        #define DEBUG   DEBUG_UART1_NoRemap
        #endif

### debug.c, USART_Printf_Init() required modifications for USART receive:

        // See debug2.c for modified version, USART_Printf_Init2()
        // USART RX - input pin: D6
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
        GPIO_Init(GPIOD, &GPIO_InitStructure);
        // Need BOTH TX and RX:
        USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;

### Requires UART read support, debug2.c

        __attribute__((used))
        int __io_getchar(void)
        {
            if( USART_GetFlagStatus(USART1, USART_FLAG_RXNE))
            return USART_ReceiveData(USART1);
            else
                return EOF;
        }
        

### Supplies the following command list:

        Command Line parser, Aug 13 2024
        Enter "help" or "?" for list of commands
        >help
        Help - command list
        Command     Comment
        ?           display help menu
        help        display help menu
        add         add <number> <number>
        id          unique ID
        info        processor info
        read        read <address>, display 32-bit value
        clocks      display clock control registers
        reset       reset processor
        resetcause  display reset cause flag
        i2cscan     scan I2C1, showing active devices
        temp        access external DS3231, read temperature
        
        >

### Example "clocks: command output:

        >clocks
        RCC->CTLR : 03003F83
        PLL clock ready
        PLL enable
        HSI CAL: 3F
        HSI TRIM: 10
        HSI ready
        HSI enable
        RCC->CFGR0: 0000000A
        System clock: PLL
        
        >

### Scan I2C1 Bus, displaying peripherals present
            
        >i2cscan
             0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
        00:          -- -- -- -- -- -- -- -- -- -- -- -- --
        10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
        20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
        30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
        40: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
        50: -- -- -- -- -- -- -- 57 -- -- -- -- -- -- -- --
        60: -- -- -- -- -- -- -- -- 68 -- -- -- -- -- -- --
        70: -- -- -- -- -- -- -- --
        
        >
