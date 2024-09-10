// Copyright Jim Merkle, 2/17/2020
// Module: command_line.c
//
// Command Line Parser
//
// Using serial interface, receive commands with parameters.
// Parse the command and parameters, look up the command in a table, execute the command.
// Since the command/argument buffer is global with global pointers, each command will parse its own arguments.
// Since no arguments are passed in the function call, all commands will have int command_name(void) prototype.

// Notes:
// The stdio library's stdout stream is buffered by default.  This makes printf() and putchar() work strangely
// for character I/O.  Buffering needs to be disabled for this code module.  See setvbuf() in cl_setup().

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h> // uint8_t
#include "command_line.h"
#include "i2c.h"
#include "core_riscv.h"

// Typedefs
typedef struct {
  char * command;
  char * comment;
  int arg_cnt; // count of arguments plus command
  int (*function)(void); // pointer to command function
} COMMAND_ITEM;

const COMMAND_ITEM cmd_table[] = {
    {"?",         "display help menu",                            1, cl_help},
    {"help",      "display help menu",                            1, cl_help},
    {"add",       "add <number> <number>",                        3, cl_add},
    {"id",        "unique ID",                                    1, cl_id},
    {"info",      "processor info",                               1, cl_info},
    {"read",      "read <address>, display 32-bit value",         2, cl_read},
    {"clocks",    "display clock control registers",              1, cl_clocks},
    {"reset",     "reset processor",                              1, cl_reset},
    {"resetcause","display reset cause flag",                     1, cl_reset_cause},
    {"servo",     "0.8ms, 1.5ms, 2.2ms pulse widths",             1, cl_servo},
    {"i2cscan",   "scan I2C1, showing active devices",            1, cl_i2cscan},
    {"temp",      "access external DS3231, read temperature",     1, cl_ds3231_temperature},
    {NULL,NULL,0,NULL}, /* end of table */
};

// Globals:
char buffer[MAXSERIALBUF]; // holds command strings from user
char * argv[MAXWORDS]; // pointers into buffer
int argc; // number of words (command & arguments)

void cl_setup(void) {
    // The STM32 development environment's stdio library provides buffering of stdout stream by default.  Turn it off!
    setvbuf(stdout, NULL, _IONBF, 0);
    // Turn on yellow text, print greeting, reset attributes
    printf("\n" COLOR_YELLOW "Command Line parser, %s" COLOR_RESET "\n",__DATE__);
    printf(COLOR_YELLOW "Enter \"help\" or \"?\" for list of commands" COLOR_RESET "\n");
    putchar('>'); // initial prompt
}

// Externals
extern int USART_ReadByte(void);   // debug2.c

// Check for data available from USART interface.  If none present, just return.
// If data available, process it (add it to character buffer if appropriate)
void cl_loop(void)
{
    static int index = 0; // index into global buffer
    int c;

    // Spin, reading characters until EOF character is received (no data), buffer is full, or
    // a <line feed> character is received.  Null terminate the global string, don't return the <LF>
    while(1) {
      c = USART_ReadByte();
      switch(c) {
          case EOF:
              return; // non-blocking - return
          case _CR:
          case _LF:
            buffer[index] = 0; // null terminate
            if(index) {
                putchar(_LF); // newline
                cl_process_buffer(); // process the null terminated buffer
            }
            printf("\n>");
            index = 0; // reset buffer index
            return;
          case _BS:
            if(index<1) continue;
            printf("\b \b"); // remove the previous character from the screen and buffer
            index--;
            break;
          default:
            if(index<(MAXSERIALBUF - 1) && c >= ' ' && c <= '~') {
                putchar(c); // write character to terminal
                buffer[index] = (char) c;
                index++;
            }
      } // switch
  } // while(1)
  return;
} // cl_loop()

void cl_process_buffer(void)
{
    argc = cl_parseArgcArgv(buffer, argv, MAXWORDS);
    // Display each of the "words" / command and arguments
    //for(int i=0;i<argc;i++)
    //  printf("%d >%s<\n",i,argv[i]);
    if (argc) {
        // At least one "word" / argument found
        // See if command has a match in the command table
        // If null function pointer found, exit for-loop
        int cmdIndex;
        for (cmdIndex = 0; cmd_table[cmdIndex].function; cmdIndex++) {
            if (strcmp(argv[0], cmd_table[cmdIndex].command) == 0) {
                // We found a match in the table
                // Enough arguments?
                if (argc < cmd_table[cmdIndex].arg_cnt) {
                    printf("\r\nInvalid Arg cnt: %d Expected: %d\n", argc - 1,
                            cmd_table[cmdIndex].arg_cnt - 1);
                    break;
                }
                // Call the function associated with the command
                (*cmd_table[cmdIndex].function)();
                break; // exit for-loop
            }
        } // for-loop
          // If we compared all the command strings and didn't find the command, or we want to fake that event
        if (!cmd_table[cmdIndex].command) {
            printf("Command \"%s\" not found\r\n", argv[0]);
        }
    } // At least one "word" / argument found
}

// Return true (non-zero) if character is a white space character
int cl_isWhiteSpace(char c) {
  if(c==' ' || c=='\t' ||  c=='\r' || c=='\n' )
    return 1;
  else
    return 0;
}

// Parse string into arguments/words, returning count
// Required an array of char pointers to store location of each word, and number of strings available
// "count" is the maximum number of words / parameters allowed
int cl_parseArgcArgv(char * inBuf,char **words, int count)
{
  int wordcount = 0;
  while(*inBuf) {
    // We have at least one character
    while(cl_isWhiteSpace(*inBuf)) inBuf++; // remove leading whitespace
    if(*inBuf) {// have a non-whitespace
      if(wordcount < count) {
        // If pointing at a double quote, need to remove/advance past the first " character
        // and find the second " character that goes with it, and remove/advance past that one too.
        if(*inBuf == '\"' && inBuf[1]) {
            // Manage double quoted word
            inBuf++; // advance past first double quote
            words[wordcount]=inBuf; // point at this new word
            wordcount++;
            while(*inBuf && *inBuf != '\"') inBuf++; // move to end of word (next double quote)
        } else {
            // normal - not double quoted string
            words[wordcount]=inBuf; // point at this new word
            wordcount++;
            while(*inBuf && !cl_isWhiteSpace(*inBuf)) inBuf++; // move to end of word
        }
        if(cl_isWhiteSpace(*inBuf) || *inBuf == '\"') { // null terminate this word
          *inBuf=0;
          inBuf++;
        }
      } // if(wordcount < count)
      else {
        *inBuf=0; // null terminate string
        break; // exit while-loop
      }
    }
  } // while(*inBuf)
  return wordcount;
} // parseArgcArgv()

#define COMMENT_START_COL  12  //Argument quantity displayed at column 12
// We may want to add a comment/description field to the table to describe each command
int cl_help(void) {
    printf("Help - command list\r\n");
    printf("Command     Comment\r\n");
    // Walk the command array, displaying each command
    // Continue until null function pointer found
    for (int i = 0; cmd_table[i].function; i++) {
        printf("%s", cmd_table[i].command);
        // insert space depending on length of command
        unsigned cmdlen = strlen(cmd_table[i].command);
        for (unsigned j = COMMENT_START_COL; j > cmdlen; j--)
            printf(" "); // variable space so comment fields line up
        printf("%s\r\n", cmd_table[i].comment);
    }
    printf("\n");
    return 0;
}

int cl_add(void) {
    printf("add..  A: %s  B: %s\n", argv[1], argv[2]);
    int A = (int) strtol(argv[1], NULL, 0); // allow user to use decimal or hex
    int B = (int) strtol(argv[2], NULL, 0);
    int ret = A + B;
    printf("returning %d\n\n", ret);
    return ret;
}


//Unique device ID register (96 bits)
//Base address: 0x1FFFF7E8, 0x1FFFF7EC, 0x1FFFF7F0 32 bits each address, 12 bytes/96 bits total
#define UUID_BASE 0x1FFFF7E8

int cl_id(void) {
    volatile uint8_t *p_id = (uint8_t*) UUID_BASE; // 0x1FFFF7E8
    printf("Unique ID: 0x");
    for (int i = 11; i >= 0; i--)
        printf("%02X", p_id[i]); // display bytes from high byte to low byte

    printf("\n");
    return 0;
}

//Memory size register
//30.1.1 Flash capacity register
//Base address:  0x1FFFF7E0
#define FLASHSIZE_BASE 0x1FFFF7E0
//Contains number of K bytes of FLASH, IE: 0x80 = 128K bytes flash

int cl_info(void) {
    volatile uint16_t *p_k_bytes = (uint16_t*) FLASHSIZE_BASE; // stm32f103xb.h
    //volatile uint32_t *p_dev_id = (uint32_t*) DBGMCU_BASE; // stm32f103xb.h
    printf("Processor FLASH: %uK bytes\n", *p_k_bytes);
    printf("Processor RAM: 2K bytes\n"); // Built-in 2KB SRAM, starting address 0x20000000")
    //printf("Processor ID Code: 0x%08lX\n", *p_dev_id);
    return 0;
}

// Other things to add...
// MCO on PC4 pin

// Read and display memory/register value
int cl_read(void)
{
    char *endptr;
    uint32_t address = (uint32_t) strtol(argv[1], &endptr, 16); // allow user to use decimal or hex
    if (endptr == argv[1]) {
        printf("Invalid hex address\n");
        return 1;
    }
    uint32_t value = *(uint32_t *)address;
    printf("[%08X]: %08X\n",address,value);

    return 0;
}

// Display values of clock control registers
int cl_clocks(void)
{
    printf(COLOR_GREEN "RCC->CTLR : %08X" COLOR_RESET "\n",RCC->CTLR);
    if(RCC->CTLR & RCC_PLLRDY)  printf("PLL clock ready\n");
    if(RCC->CTLR & RCC_PLLON)   printf("PLL enable\n");
    if(RCC->CTLR & RCC_CSSON)   printf("Clock Security System enable\n");
    if(RCC->CTLR & RCC_HSEBYP)  printf("HSE bypass\n");
    if(RCC->CTLR & RCC_HSERDY)  printf("HSE ready\n");
    if(RCC->CTLR & RCC_HSEON)   printf("HSE enable\n");

    uint32_t hsical = RCC->CTLR & RCC_HSICAL;
    if(hsical){
        hsical >>= 8;
        printf("HSI CAL: %02X\n",hsical); }

    uint32_t hsitrim = RCC->CTLR & RCC_HSITRIM;
        if(hsitrim){
            hsitrim >>= 3;
            printf("HSI TRIM: %02X\n",hsitrim); }

    if(RCC->CTLR & RCC_HSIRDY)  printf("HSI ready\n");
    if(RCC->CTLR & RCC_HSION)   printf("HSI enable\n");

    printf(COLOR_GREEN "RCC->CFGR0: %08X" COLOR_RESET "\n",RCC->CFGR0);
    uint32_t sws = RCC->CFGR0 & RCC_SWS;
    printf("System clock: ");
    if(RCC_SWS_HSI == sws ) printf("HSI\n");
    if(RCC_SWS_HSE == sws ) printf("HSE\n");
    if(RCC_SWS_PLL == sws ) printf("PLL\n");

    return 0;
}

// Reset the processor via software reset
//6.5.2.6 PFIC interrupt configuration register (PFIC_CFGR)
//Offset address: 0x48
//[31:16] KEYCODE WO
//KEY1 = 0xFA05.
//KEY2 = 0xBCAF.
//KEY3 = 0xBEEF.
//[15:8] Reserved RO Reserved 0
//7 RESETSYS WO System reset
int cl_reset(void) {
    printf("%s\n",__func__);
    Delay_Ms(10);
    PFIC->CFGR = NVIC_KEY3 | 0x80;
    return 0;
}

// Display reset cause
// Control/Status register (RCC_RSTSCKR)
// 31 LPWRRSTF RO, Low-power reset flag.
// 30 WWDGRSTF RO, Window watchdog reset flag.
// 29 IWDGRSTF RO, Independent watchdog reset flag.
// 28 SFTRSTF  RO, Software reset flag.
// 27 PORRSTF  RO, Power-up/power-down reset flag.
// 26 PINRSTF  RO, External manual reset (NRST pin) flag.
int cl_reset_cause(void)
{
    printf("Reset cause: ");
    if(RCC->RSTSCKR & RCC_LPWRRSTF) printf("LPWRRSTF\n");
    if(RCC->RSTSCKR & RCC_WWDGRSTF) printf("WWDGRSTF\n");
    if(RCC->RSTSCKR & RCC_IWDGRSTF) printf("IWDGRSTF\n");
    if(RCC->RSTSCKR & RCC_SFTRSTF)  printf("SFTRSTF\n");
    if(RCC->RSTSCKR & RCC_PORRSTF)  printf("PORRSTF\n");
    if(RCC->RSTSCKR & RCC_PINRSTF)  printf("PINRSTF\n");

    // Clear reset flags for next time
    RCC->RSTSCKR |= RCC_RMVF;
    return 0;
}

/*
 *@Note
 PWM_MODE1: ccp defines active high pulse width
 PWM_MODE2: ccp defines active low pulse width

 For servo control, MODE1 is preferred.
   Loading larger values for ccp creates larger active high pulse width
*/

/* PWM Output Mode Definition */
#define PWM_MODE1   0
#define PWM_MODE2   1

/* PWM Output Mode Selection */
#define PWM_MODE PWM_MODE1    //
//#define PWM_MODE PWM_MODE2

/*********************************************************************
 * @fn      TIM1_OutCompare_Init
 *
 * @brief   Initializes TIM1 output compare.
 *          Use PD2 for PWM output
 *
 * @param   arr - the period value.
 *          psc - the prescaler value.
 *          ccp - the pulse value.
 *
 * @return  none
 */
void TIM1_PWMOut_Init(u16 arr, u16 psc, u16 ccp)
{
    GPIO_InitTypeDef GPIO_InitStructure={0};
    TIM_OCInitTypeDef TIM_OCInitStructure={0};
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure={0};

    RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOD | RCC_APB2Periph_TIM1, ENABLE );

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init( GPIOD, &GPIO_InitStructure );

    TIM_TimeBaseInitStructure.TIM_Period = arr;
    TIM_TimeBaseInitStructure.TIM_Prescaler = psc;
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit( TIM1, &TIM_TimeBaseInitStructure);

#if (PWM_MODE == PWM_MODE1)
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;

#elif (PWM_MODE == PWM_MODE2)
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;

#endif

    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = ccp;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC1Init( TIM1, &TIM_OCInitStructure );

    TIM_CtrlPWMOutputs(TIM1, ENABLE );
    TIM_OC1PreloadConfig( TIM1, TIM_OCPreload_Disable );
    TIM_ARRPreloadConfig( TIM1, ENABLE );
    TIM_Cmd( TIM1, ENABLE );
}

// Create 50Hz (20.0ms) pulse train, with 1.0ms, 1.5ms, 2.0ms pulse width
int cl_servo(void)
{
    // Initialize PWM for 50Hz (20ms period) 0.8ms high PWM
    TIM1_PWMOut_Init( 20000, 48-1, 800); // 0.8ms (1us units for ccp)
    printf("TIM1->CH1CVR: %u\n",TIM1->CH1CVR);
    Delay_Ms(2000);

    TIM1->CH1CVR = 1500; // 1.5ms
    printf("TIM1->CH1CVR: %u\n",TIM1->CH1CVR);
    Delay_Ms(2000);

    TIM1->CH1CVR = 2200; // 2.2ms
    printf("TIM1->CH1CVR: %u\n",TIM1->CH1CVR);
    //Delay_Ms(2000);
    return 0;
}

// command line interface for i2c_scan()
int cl_i2cscan(void)
{
    i2c_scan();
    return 0;
}

#define I2C_ADDRESS_DS3231  0x68   // 7-bit I2C address for DS3231

// If DS3231 present, poll and display the temperature each second
int cl_ds3231_temperature(void)
{
    if(I2C_ERROR_SUCCESS != i2c_device_detect(I2C_ADDRESS_DS3231)) {
        printf("DS3231 Not Found !\n");
        return I2C_ERROR_ACK;
    }
    printf("%s, Continuously read DS3231 temperature until reset\n",__func__);
    while(1) {
        // Force a temperature conversion, write 0x3C to control register, 0x0E (set CONV bit, BIT5)
        uint8_t reg=0x0E;
        uint8_t control_reg[2] = {0x0E,0x3C};
        i2c_write(I2C_ADDRESS_DS3231,control_reg,sizeof(control_reg));

        // Read temperature registers, 0x11, 0x12
        reg=0x11; // Temperature, MSB (Celcius)
        uint8_t temp_reg[2] = {0,0};
        i2c_write(I2C_ADDRESS_DS3231,&reg,sizeof(reg));
        i2c_read(I2C_ADDRESS_DS3231, temp_reg, sizeof(temp_reg));
        //printf("temp_reg0: %02X, temp_reg1: %02X\n",temp_reg[0],temp_reg[1]);
        // Combine registers into int16_t
        uint16_t u_temp_c = ((uint16_t)temp_reg[0]<<8) + ((uint16_t)temp_reg[1]);
        int16_t temp_c = (int16_t)u_temp_c;
        //printf("u_temp_c: %04X\n",u_temp_c);
        temp_c /= 64; // convert to 1/4 degree C units
        printf("Temp: %d %d/4C\n",temp_c/4,temp_c%4); // This display method only works for positive temperature values

        // Convert to Fahrenheit
        //int16_t temp_f = (((int16_t)temp_msb * 18) / 10) + 32 ; // multiply by 1.8, add 32
        //printf("Temp: %dF\n",temp_f);
        Delay_Ms(1000);
    } // while
}

