#ifndef __JTAGGER__H__
#define __JTAGGER__H__

#include "Arduino.h"

/**  
 * If you don't wish to see debug info such as TAP state transitions put 0.
 * Otherwise assign 1.
 */
#define DEBUGTAP 0

/** 
 * If you don't wish to see debug info regarding user input via serial port put 0.
 * Otherwise assign 1.
 */
#define DEBUGSERIAL 1

/**
 * If 1 then print each time TAP is reset
 */
#define PRINT_RESET_TAP 0

/*
 * Define JTAG pins as you wish
 */
#define TCK 7
#define TMS 8
#define TDI 9
#define TDO 10
#define TRST 11

/**
 * Sizes of global arrays to store
 * content of IR and DR
 */
#define MAX_IR_LEN 128
#define MAX_DR_LEN 1024

/**
 * Number of 1s to insert into IR
 * to clear it from previous content.
 * Used to count the IR length
 */
#define MANY_ONES 100

/*	Choose a half-clock cycle delay	*/
// half clock cycle
// #define HC delay(1);
// or
#define DELAY_US 100 // delay in microseonds for a half-clock cycle (HC) to drive TCK.
#define HC delayMicroseconds(DELAY_US);

// A more precise way to delay a half clock cycle:
/*
#define HC \
{ \
    __asm__ __volatile__("nop \n\t \
                          nop \n\t \
                          nop \n\t \
                          nop \n\t \
                          " : :);  \
}
Notice, that you also need to override the digitalWrite and digitalRead
functions with an appropriate assembly in order to reach the desired JTAG speeds.
*/
 

/**
 * @brief Prints ASCII art
 */
void print_welcome();

/**
 * @brief Display all available commands.
 * Also, you can add you custom commands for a specific target.
 * For example the MAX10 FPGA commands are included.
 */
void print_main_menu();


#endif