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
//#include "main.h"   // HAL functions and defines

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
    //{"reset",     "reset processor",                              1, cl_reset},
    //{"timer",     "timer test - testing 50ms delay",              1, cl_timer},
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
int __io_getchar(void);   // main.c
//int __io_putchar(int ch); // main.c

// Check for data available from USART interface.  If none present, just return.
// If data available, process it (add it to character buffer if appropriate)
void cl_loop(void)
{
    static int index = 0; // index into global buffer
    int c;

    // Spin, reading characters until EOF character is received (no data), buffer is full, or
    // a <line feed> character is received.  Null terminate the global string, don't return the <LF>
    while(1) {
      c = __io_getchar();
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

#if 0
// Return appropriate error code string
// Yes, we could just return the HAL strings vs copy them...
char * PrintHalStatus(int status)
{
    static char s_status[20];
    switch(status) {
        case HAL_OK:
          sprintf(s_status,"HAL_OK");
          break;
        case HAL_ERROR:
          sprintf(s_status,"HAL_ERROR");
          break;
        case HAL_BUSY:
          sprintf(s_status,"HAL_BUSY");
          break;
        case HAL_TIMEOUT:
          sprintf(s_status,"HAL_TIMEOUT");
          break;
        default:
          sprintf(s_status,"%d",status);
          break;
    } // switch
    return s_status;
} // PrintHalStatus()

// Reset the processor
int cl_reset(void) {
    NVIC_SystemReset(); // CMSIS Cortex-M3 function - see Drivers/CMSIS/Include/core_cm3.h
    while (1) ; // wait here until reset completes

    return 0;
}

// Perform a timer2 test.
// Timer2 is a 16-bit free-running timer with pre-scale counter.
// Is the us timer tracking System Ticks?
// Timer2 is configured to update each micro-second
// Alternatively, if used a GPIO, we could toggle a pin after X micro-seconds
int cl_timer(void)
{
    printf("%s(), Timing HAL_Delay(50)\n",__func__);
    volatile TIM_TypeDef *TIMx = TIM2; // Use timer 2
    uint32_t start_ticks = HAL_GetTick();
    uint32_t start_us = TIMx->CNT; // read us hardware timer
    HAL_Delay(50); // delay 50us
    uint32_t stop_us = TIMx->CNT; // read us hardware timer
    uint32_t stop_ticks = HAL_GetTick();
    // Report results
    printf("HAL_GetTick() time: %lu ms\n",stop_ticks-start_ticks);
    if(stop_us < start_us) stop_us += 1<<16; // roll-over, add 16-bit roll-over offset
    printf("TIMx->CNT time: %lu us\n",stop_us - start_us);
    return 0;
}
#endif

