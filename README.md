# CH32V003_command_line

Example Command Line project for the CH32V003

Modifications to debug.c, USART_Printf_Init():

        // USART RX - input pin: D6
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
        GPIO_Init(GPIOD, &GPIO_InitStructure);
        // Need BOTH TX and RX:
        USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;

Requires UART read support -

        __attribute__((used))
        int __io_getchar(void)
        {
            if( USART_GetFlagStatus(USART1, USART_FLAG_RXNE))
            return USART_ReceiveData(USART1);
            else
                return EOF;
        }
        

Supplies the following command list:

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
        
        
        >
