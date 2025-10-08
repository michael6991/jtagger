#include "jtag_drv.h"
#include "../../include/utils.h"

void reset_tap()
{
#if PRINT_RESET_TAP
    Serial.print("\nResetting TAP\n");
#endif
    for (uint8_t i = 0; i < 5; ++i)
    {
        digitalWrite(TMS, 1);
        digitalWrite(TCK, 0); HC;
        digitalWrite(TCK, 1); HC;
    }
    current_state = TEST_LOGIC_RESET;
}

int detect_chain(uint32_t* out_ir_len, uint32_t* out_idcode)
{
    uint8_t id_bits[32] = {0};
    uint32_t idcode = 0;
    uint32_t i = 0;
    uint8_t counter = 0;
    int rc = OK;

    reset_tap();
    Serial.println("Attempting to detect active chain");

    // try to read IDCODE first and then detect the IR length
    advance_tap_state(RUN_TEST_IDLE);
    advance_tap_state(SELECT_DR);
    advance_tap_state(CAPTURE_DR);
    advance_tap_state(SHIFT_DR);
    
    // shift out the IDCODE from the id code register
    // assumed that the IDCODE IR is the default IR after power up.
    // first bit to read out is the LSB of the IDCODE.
    for (i = 0; i < 32; i++)
    {
        advance_tap_state(SHIFT_DR);
        id_bits[i] = digitalRead(TDO);
    }
    advance_tap_state(EXIT1_DR);

    // LSB of IDCODE must be 1.
    if (id_bits[0] != 1)
    {
        Serial.println("\n\nBad IDCODE or not implemented, LSB = 0");
        return -ERR_BAD_IDCODE;
    }

    // turn IDCODE bits into unsigned integer
    rc = bin_array_to_uint32(id_bits, 32, &idcode);
    if (rc != OK) {
        Serial.println("\nCould not convert IDCODE");
        *out_ir_len = 0;
        *out_idcode = 0;
        return rc;
    }

    Serial.print("\nFound IDCODE: ");
    print_array(id_bits, 32); Serial.print(" (0x");
    Serial.print(idcode, HEX); Serial.print(")");

    // find ir length.
    Serial.print("\nAttempting to find IR length of target ...\n");
    reset_tap();
    advance_tap_state(RUN_TEST_IDLE);
    advance_tap_state(SELECT_DR);
    advance_tap_state(SELECT_IR);
    advance_tap_state(CAPTURE_IR);
    advance_tap_state(SHIFT_IR);
    
    // shift in MANY_ONES amount of ones into TDI to clear the register
    // from its previos content. then shift a single zero followed by
    // a bunch of ones and cout the amount of clock cycles from inserting zero
    // till we read it back in TDO.
    digitalWrite(TDI, 1);
    for (i = 0; i < MANY_ONES; ++i) 
    {
        advance_tap_state(SHIFT_IR);
    }

    digitalWrite(TDI, 0);
    advance_tap_state(SHIFT_IR);

    digitalWrite(TDI, 1);
    for (i = 0; i < MANY_ONES; ++i)
    {
        advance_tap_state(SHIFT_IR);

        if (digitalRead(TDO) == 0)
        {
            counter++;
            *out_ir_len = counter;
            *out_idcode = idcode;
            Serial.print("\nIR length: "); Serial.println(cur_tap->ir_len, DEC);
            return OK;
        }
        counter++;
    }

    advance_tap_state(EXIT1_IR);
    advance_tap_state(UPDATE_IR);
    advance_tap_state(RUN_TEST_IDLE);

    *out_ir_len = 0;
    *out_idcode = 0;
    Serial.println("\nDidn't find valid IR length");

    return -ERR_INVALID_IR_OR_DR_LEN;
}

void insert_ir(uint8_t* ir_in, uint8_t* ir_out, uint32_t ir_len, uint8_t end_state)
{
    // make sure that current state is TLR (test logic reset)
    uint8_t i = 0;

    advance_tap_state(RUN_TEST_IDLE);
    advance_tap_state(SELECT_DR);
    advance_tap_state(SELECT_IR);
    advance_tap_state(CAPTURE_IR);
    advance_tap_state(SHIFT_IR);

    // shift data bits into the IR. make sure that first bit is LSB
    for (i = 0; i < ir_len - 1; i++)
    {
        digitalWrite(TDI, ir_in[i]);
        digitalWrite(TCK, 0); HC;
        digitalWrite(TCK, 1); HC;
        ir_out[i] = digitalRead(TDO);  // read the shifted out bits. LSB first
    }

    // read and write the last IR bit and continue to the end state
    digitalWrite(TDI, ir_in[i]);
    advance_tap_state(EXIT1_IR);
    ir_out[i] = digitalRead(TDO);

    advance_tap_state(UPDATE_IR);

    if (end_state == RUN_TEST_IDLE){	
        advance_tap_state(RUN_TEST_IDLE);
    }
    else if (end_state == SELECT_IR){
        advance_tap_state(SELECT_IR);
    }
    else if (end_state == TEST_LOGIC_RESET){
        reset_tap();
    }
}

void insert_dr(uint8_t* dr_in, uint8_t dr_len, uint8_t end_state, uint8_t* dr_out)
{
    // make sure that current state is TLR
    uint32_t i = 0;

    advance_tap_state(RUN_TEST_IDLE);
    advance_tap_state(SELECT_DR);
    advance_tap_state(CAPTURE_DR);
    advance_tap_state(SHIFT_DR);

    // shift data bits into DR. make sure that first bit is LSB
    for (i = 0; i < dr_len - 1; i++)
    {
        digitalWrite(TDI, dr_in[i]);
        digitalWrite(TCK, 0); HC;
        digitalWrite(TCK, 1); HC;
        dr_out[i] = digitalRead(TDO);  // read the shifted out bits . LSB first
    }

    // read and write the last DR bit and continue to the end state
    digitalWrite(TDI, dr_in[i]);
    advance_tap_state(EXIT1_DR);
    dr_out[i] = digitalRead(TDO); 

    advance_tap_state(UPDATE_DR);

    if (end_state == RUN_TEST_IDLE){	
        advance_tap_state(RUN_TEST_IDLE);
    }
    else if (end_state == SELECT_DR){
        advance_tap_state(SELECT_DR);
    }
    else if (end_state == TEST_LOGIC_RESET){
        reset_tap();
    }
}

void flush_ir_dr(uint8_t* ir_reg, uint8_t* dr_reg, uint32_t ir_len, uint32_t dr_len)
{
    clear_reg(ir_reg, ir_len);
    clear_reg(dr_reg, dr_len);
}

uint32_t detect_dr_len(uint8_t* instruction, uint8_t ir_len, uint32_t process_ticks)
{	
    // make sure that current state is TLR prior this calling this function.

    // temporary array to strore the shifted out bits from IR
    uint8_t tmp[ir_len];
    uint32_t i = 0;
    uint32_t counter = 0;

    // insert the instruction we wish to check into ir
    insert_ir(instruction, tmp, ir_len, RUN_TEST_IDLE);
    
    // a couple of clock cycles to process the instruction
    for (i = 0; i < process_ticks; i++)
    {
        HC; HC;
    }

    /* 
        check the length of the DR register between TDI and TDO
        by inserting many ones (MAX_DR_LEN) to clean the register.
        afterwards, insert a single zero and start counting the amount
        of TCK clock cycles till the appearence of that zero in TDO.
    */
    advance_tap_state(SELECT_DR);
    advance_tap_state(CAPTURE_DR);
    advance_tap_state(SHIFT_DR);

    digitalWrite(TDI, 1);
    for (i = 0; i < MAX_DR_LEN; ++i)
    {
        advance_tap_state(SHIFT_DR);
    }

    digitalWrite(TDI, 0);
    advance_tap_state(SHIFT_DR);

    digitalWrite(TDI, 1);
    for (i = 0; i < MAX_DR_LEN; ++i)
    {
        advance_tap_state(SHIFT_DR);

        if (digitalRead(TDO) == 0){
            ++counter;
            return counter;
        }
        counter++;
    }

    return 0;
}

int discovery(uint32_t first, uint32_t last, uin32_t max_dr_len, uint8_t* ir_in, uint8_t* ir_out)
{
    uint32_t instruction, len = 0;
    int rc = OK;

    // discover all dr lengths corresponding to their ir.
    Serial.print("\n\nDiscovery of instructions from 0x"); Serial.print(first, HEX);
    Serial.print(" to 0x"); Serial.println(last, HEX);

    for (instruction=first; instruction <= last; instruction++)
    {
        // reset tap
        reset_tap();
        len = 0;

        // prepare to shift instruction
        int_to_bin_array(ir_in, instruction, ir_len);

        Serial.print("\nDetecting DR length for IR: ");
        print_array(ir_in, ir_len);
        Serial.print(" (0x"); Serial.print(instruction, HEX); Serial.print(")");
        Serial.flush();

        len = detect_dr_len(ir_in, ir_len, 4);
        if (len == max_dr_len)
        {
            Serial.println("\nDiscovery: TDO is stuck at 1");
            rc = -ERR_TDO_STUCK_AT_1;
            break;
        }

        Serial.print(" ... "); Serial.print(len, DEC);
        Serial.flush();
    }

    reset_tap();
    Serial.println("\n\n   Done");
    return rc;
}

int advance_tap_state(uint8_t next_state)
{
    int rc = OK;

    switch ( current_state )
    {
        case TEST_LOGIC_RESET:
            if (next_state == RUN_TEST_IDLE) {
                // go to run test idle
                digitalWrite(TMS, 0);
                digitalWrite(TCK, 0); HC;
                digitalWrite(TCK, 1); HC;
                current_state = RUN_TEST_IDLE;
            }
            else if (next_state == TEST_LOGIC_RESET) {
                // stay in test logic reset
                digitalWrite(TMS, 1);
                digitalWrite(TCK, 0); HC;
                digitalWrite(TCK, 1); HC;
            }
            break;

        case RUN_TEST_IDLE:
            if (next_state == SELECT_DR) {
                // go to select dr
                digitalWrite(TMS, 1);
                digitalWrite(TCK, 0); HC;
                digitalWrite(TCK, 1); HC;
                current_state = SELECT_DR;
            }
            else if (next_state == RUN_TEST_IDLE) {
                // stay in run test idle
                digitalWrite(TMS, 0);
                digitalWrite(TCK, 0); HC;
                digitalWrite(TCK, 1); HC;
            }
            break;

        case SELECT_DR:
            if (next_state == CAPTURE_DR) {
                // go to capture dr
                digitalWrite(TMS, 0);
                digitalWrite(TCK, 0); HC;
                digitalWrite(TCK, 1); HC;
                current_state = CAPTURE_DR;
            }
            else if (next_state == SELECT_IR) { 
                // go to select ir
                digitalWrite(TMS, 1);
                digitalWrite(TCK, 0); HC;
                digitalWrite(TCK, 1); HC;
                current_state = SELECT_IR;
            }
            break;

        case CAPTURE_DR:
            if (next_state == SHIFT_DR) {
                // go to shift dr
                digitalWrite(TMS, 0);
                digitalWrite(TCK, 0); HC;
                digitalWrite(TCK, 1); HC;
                current_state = SHIFT_DR;
            }
            else if (next_state == EXIT1_DR) { 
                // go to exit1 dr
                digitalWrite(TMS, 1);
                digitalWrite(TCK, 0); HC;
                digitalWrite(TCK, 1); HC;
                current_state = EXIT1_DR;
            }
            break;

        case SHIFT_DR:
            if (next_state == SHIFT_DR) {
                // stay in shift dr
                digitalWrite(TMS, 0);
                digitalWrite(TCK, 0); HC;
                digitalWrite(TCK, 1); HC;
            }
            else if (next_state == EXIT1_DR) {
                // go to exit1 dr
                digitalWrite(TMS, 1);
                digitalWrite(TCK, 0); HC;
                digitalWrite(TCK, 1); HC;
                current_state = EXIT1_DR;
            }
            break;

        case EXIT1_DR:
            if (next_state == PAUSE_DR) {
                // go to pause dr
                digitalWrite(TMS, 0);
                digitalWrite(TCK, 0); HC;
                digitalWrite(TCK, 1); HC;
                current_state = PAUSE_DR;
            }
            else if (next_state == UPDATE_DR) {
                // go to update dr
                digitalWrite(TMS, 1);
                digitalWrite(TCK, 0); HC;
                digitalWrite(TCK, 1); HC;
                current_state = UPDATE_DR;
            }
            break;

        case PAUSE_DR:
            if (next_state == PAUSE_DR) {
                // stay in pause dr
                digitalWrite(TMS, 0);
                digitalWrite(TCK, 0); HC;
                digitalWrite(TCK, 1); HC;
            }
            else if (next_state == EXIT2_DR) {
                // go to exit2 dr
                digitalWrite(TMS, 1);
                digitalWrite(TCK, 0); HC;
                digitalWrite(TCK, 1); HC;
                current_state = EXIT2_DR;
            }
            break;

        case EXIT2_DR:
            if (next_state == SHIFT_DR) {
                // go to shift dr
                digitalWrite(TMS, 0);
                digitalWrite(TCK, 0); HC;
                digitalWrite(TCK, 1); HC;
                current_state = SHIFT_DR;
            }
            else if (next_state == UPDATE_DR) {
                // go to update dr
                digitalWrite(TMS, 1);
                digitalWrite(TCK, 0); HC;
                digitalWrite(TCK, 1); HC;
                current_state = UPDATE_DR;
            }
            break;

        case UPDATE_DR:
            if (next_state == RUN_TEST_IDLE) {
                // go to run test idle
                digitalWrite(TMS, 0);
                digitalWrite(TCK, 0); HC;
                digitalWrite(TCK, 1); HC;
                current_state = RUN_TEST_IDLE;
            }
            else if (next_state == SELECT_DR) {
                // go to select dr
                digitalWrite(TMS, 1);
                digitalWrite(TCK, 0); HC;
                digitalWrite(TCK, 1); HC;
                current_state = SELECT_DR;
            }
            break;

        case SELECT_IR:
            if (next_state == CAPTURE_IR) {
                // go to capture ir
                digitalWrite(TMS, 0);
                digitalWrite(TCK, 0); HC;
                digitalWrite(TCK, 1); HC;
                current_state = CAPTURE_IR;
            }
            else if (next_state == TEST_LOGIC_RESET) {
                // go to test logic reset
                digitalWrite(TMS, 1);
                digitalWrite(TCK, 0); HC;
                digitalWrite(TCK, 1); HC;
                current_state = TEST_LOGIC_RESET;
            }
            break;

        case CAPTURE_IR:
            if (next_state == SHIFT_IR) {
                // go to shift ir
                digitalWrite(TMS, 0);
                digitalWrite(TCK, 0); HC;
                digitalWrite(TCK, 1); HC;
                current_state = SHIFT_IR;
            }
            else if (next_state == EXIT1_IR) {
                // go to exit1 ir
                digitalWrite(TMS, 1);
                digitalWrite(TCK, 0); HC;
                digitalWrite(TCK, 1); HC;
                current_state = EXIT1_IR;
            }
            break;

        case SHIFT_IR:
            if (next_state == SHIFT_IR) {
                // stay in shift ir
                digitalWrite(TMS, 0);
                digitalWrite(TCK, 0); HC;
                digitalWrite(TCK, 1); HC;
            }
            else if (next_state == EXIT1_IR) {
                // go to exit1 ir
                digitalWrite(TMS, 1);
                digitalWrite(TCK, 0); HC;
                digitalWrite(TCK, 1); HC;
                current_state = EXIT1_IR;
            }
            break;

        case EXIT1_IR:
            if (next_state == PAUSE_IR) {
                // go to pause ir
                digitalWrite(TMS, 0);
                digitalWrite(TCK, 0); HC;
                digitalWrite(TCK, 1); HC;
                current_state = PAUSE_IR;
            }
            else if (next_state == UPDATE_IR) {
                // go to update ir
                digitalWrite(TMS, 1);
                digitalWrite(TCK, 0); HC;
                digitalWrite(TCK, 1); HC;
                current_state = UPDATE_IR;
            }
            break;

        case PAUSE_IR:
            if (next_state == PAUSE_IR) {
                // stay in pause ir
                digitalWrite(TMS, 0);
                digitalWrite(TCK, 0); HC;
                digitalWrite(TCK, 1); HC;
            }
            else if (next_state == EXIT2_IR) {
                // go to exit2 dr
                digitalWrite(TMS, 1);
                digitalWrite(TCK, 0); HC;
                digitalWrite(TCK, 1); HC;
                current_state = EXIT2_IR;
            }
            break;

        case EXIT2_IR:
            if (next_state == SHIFT_IR) {
                // go to shift ir
                digitalWrite(TMS, 0);
                digitalWrite(TCK, 0); HC;
                digitalWrite(TCK, 1); HC;
                current_state = SHIFT_IR;
            }
            else if (next_state == UPDATE_IR) {
                // go to update ir
                digitalWrite(TMS, 1);
                digitalWrite(TCK, 0); HC;
                digitalWrite(TCK, 1); HC;
                current_state = UPDATE_IR;
            }
            break;

        case UPDATE_IR:
            if (next_state == RUN_TEST_IDLE) {
                // go to run test idle
                digitalWrite(TMS, 0);
                digitalWrite(TCK, 0); HC;
                digitalWrite(TCK, 1); HC;
                current_state = RUN_TEST_IDLE;
            }
            else if (next_state == SELECT_DR) {
                // go to select dr
                digitalWrite(TMS, 1);
                digitalWrite(TCK, 0); HC;
                digitalWrite(TCK, 1); HC;
                current_state = SELECT_DR;
            }
            break;

        default:
            Serial.println("Error: incorrent TAP state !");
            rc = -ERR_BAD_TAP_STATE;
            break;
    }
#if DEBUGTAP
    Serial.print("\ntap state: ");
    Serial.print(current_state, HEX);
#endif
    return rc;
}
