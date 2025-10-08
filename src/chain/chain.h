#ifndef __CHAIN__H__
#define __CHAIN__H__

#include <stdint.h>

#include "../../include/status.h"

/**
 *  The total number of exisitng TAPs/Devices in the system that
 *  can be registered.
 */
#define MAX_ALLOWED_TAPS 16

typedef struct
{
    char name[32]; // must be null terminated
    uint32_t idcode;
    uint32_t ir_len;
    uint32_t ir_in_idx;
    uint32_t ir_out_idx;
    bool active;
}__attribute__((packed)) tap_t;

uint32_t chain_get_active_devices();

uint32_t chain_get_total_ir_len();

/**
 * Initialize the TAPs array of tap_t structs.
 */
void chain_taps_init(tap_t* taps);

status_t chain_tap_add(tap_t* taps, int index, const char* name, const uint32_t idcode, const int ir_len);

status_t chain_tap_remove(tap_t* taps, int index);

/**
 *         Global IR register
 * 
 * example: array/register of 12 bits
 * 
 *    MSB                                LSB
 *    11 10  9  8  7   6  5  4  3    2  1  0
 *    |_||_||_||_||_| |_||_||_||_|  |_||_||_|
 *     ^           ^   ^        ^    ^     ^
 *     out        in   out     in    out  in
 *     { dev 0     }   { dev 1  }    { dev 2 }
 *     { len 5     }   { len 4  }    { len 3 }
 * 
 * When activating a tap device, we assign its "in" entrance index
 * into the global IR, as well as the "out" output point from the IR.
 * Knowing the entrance index allows us to prepare the appropriate
 * payload to insert into the global IR by taking into consideration
 * the offset relative to the first bit (MSB  bit 0).
 * 
 * Explanation according to the above example:
 *  device 0: in = 0, out = 4
 *  device 1: in = 5, out = 8
 *  device 2: in = 9, out = 11
 * 
 */
status_t chain_tap_activate(tap_t* taps, int index, const int ir_in_idx, const int ir_out_idx);

status_t chain_tap_deactivate(tap_t* taps, int index);

/**
 * Example of a system/board where 2 TAPs exist in a single SOC:
 * 
 * The STM32F4xx MCUs integrate two serially connected JTAG TAPs, the boundary scan
 * TAP (IR is 5-bit wide) and the Cortex®-M4 with FPU TAP (IR is 4-bit wide).
 * To access the TAP of the Cortex®-M4 with FPU for debug purposes:
 * 1. First, it is necessary to shift the BYPASS instruction of the boundary scan TAP.
 * 2. Then, for each IR shift, the scan chain contains 9 bits (=5+4) and the unused TAP
 * instruction must be shifted in using the BYPASS instruction.
 * 3. For each data shift, the unused TAP, which is in BYPASS mode, adds 1 extra data bit in
 * the data scan chain.
 * 
 *           ____________       ____________
 * JTDI --> |tdi      tdo|---->|tdi      tdo|--> JTDO
 *          |            |     |            |
 *          |boundry scan|     |cortex-m4   |
 *          | tap  5-bit |     | tap  4-bit |
 *          |____________|     |____________|
 * 
 */
status_t chain_tap_selector(tap_t* taps, int index, tap_t* out, uint8_t* ir_in, uint8_t* ir_out);

/**
 * Print all active TAP devices in TAPs chain array.
 */
void chain_print_active_taps(tap_t* taps);

#endif /* __CHAIN_H__ */