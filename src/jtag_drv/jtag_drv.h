#include <stdint.h>

#include "../../include/status.h"
#include "../../include/main.h"
#include "Arduino.h"

typedef enum TapState
{
    TEST_LOGIC_RESET, RUN_TEST_IDLE,
    SELECT_DR, CAPTURE_DR, SHIFT_DR, EXIT1_DR, PAUSE_DR, EXIT2_DR, UPDATE_DR,
    SELECT_IR, CAPTURE_IR, SHIFT_IR, EXIT1_IR, PAUSE_IR, EXIT2_IR, UPDATE_IR
} tap_state;

// Global Variables
extern tap_state current_state;

/**
 * 
 * @brief Return to TEST LOGIC RESET state of the TAP FSM.
 * Apply 5 TCK cycles accompanied with TMS logic state 1.
 */
void reset_tap();

/**
 * @brief Detects the the existence of a chain and checks the ir length.
 * @param out_ir_len An integer that represents the length of the instructions.
 * Most certainly less than 255 bits.
 * @param out_id_code An integer that represents the idcode that is
 * available in the currently active TAP (device) between JTDI and JTDO.
 */
status_t detect_chain(uint32_t* out_ir_len, uint32_t* out_idcode);

/**
*	@brief Insert data of length ir_len to IR, and end the interaction
*	in the state end_state which can be one of the following:
*	TLR, RTI, SelectDR.
*	@param ir_in Pointer to the input data array. (bytes array)
*	@param ir_out Pointer to the output data array. (bytes array)
*	@param ir_len Length of the register currently connected between tdi and tdo.
*	@param end_state TAP state after dr inseration.
*/
void insert_ir(uint8_t* ir_in, uint8_t* ir_out, uint32_t ir_len, uint8_t end_state);

/**
*	@brief Insert data of length dr_len to DR, and end the interaction
*	in the state end_state which can be one of the following:
*	TLR, RTI.
*	@param dr_in Pointer to the input data array. (bytes array)
*	@param dr_out Pointer to the output data array. (bytes array)
*	@param dr_len Length of the register currently connected between tdi and tdo.
*	@param end_state TAP state after dr inseration.
*/
void insert_dr(uint8_t* dr_in, uint8_t* dr_out, uint32_t dr_len, uint8_t end_state);

/**
 * @brief Find out the dr length of a specific instruction.
 * Make sure that current state is TLR prior this calling this function.
 * @param instruction Pointer to the bytes array that contains the instruction.
 * @param ir_len The length of the IR. (Needs to be know prior to function call).
 * @param process_ticks Number of TCK ticks to wait for the inserted instruction to "process in".
 * @return Counter that represents the size of the DR. Or 0 if didn't find
 * a valid size. (DR may not be implemented or some other reason).
 */
uint32_t detect_dr_len(uint8_t* instruction, uint32_t ir_len, uint32_t process_ticks);

/**
 * @brief Similarly to discovery command in urjtag, performs a brute force search
 * of each possible values of the IR register to get its corresponding DR leght in bits.
 * Test Logic Reset (TLR) state is being reached after each instruction.
 * @param first ir value to begin with.
 * @param last Usually 2 to the power of (ir_len) - 1.
 * @param max_dr_len Maximum data register allowed.
 * @param ir_len Length of the IR.
 * @param ir_in Pointer to ir_in register.
*/
status_t discovery(uint32_t first, uint32_t last, uint32_t max_dr_len, uint32_t ir_len,  uint8_t* ir_in);

/**
*	@brief Advance the TAP machine 1 state ahead according to the current state 
*	and next state of the IEEE 1149.1 standard.
*	@param next_state The next state to advance to.
*/
status_t advance_tap_state(uint8_t next_state);
