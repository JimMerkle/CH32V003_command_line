# CH32V003_command_line

Example Command Line project for the CH32V003

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
