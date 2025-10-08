/* --------------------------------------------------------------------------------------- */
/* ------------------- Custom functions for MAX10 FPGA project ----------------------------*/
/* --------------------------------------------------------------------------------------- */
#include <stdint.h>

#include "max10_ir.h"
#include "../jtag_drv/jtag_drv.h"
#include "../../include/main.h"
#include "../../include/utils.h"

/**
 * @brief Read user defined 32 bit code of MAX10 FPGA.
 * @param ir_in ir_in
 * @param ir_out ir_out
 * @param dr_in dr_in
 * @param dr_out dr_out
 * @return 32 bit integer that represents the user code.
 */
uint32_t max10_read_user_code(const uint8_t ir_len, uint8_t* ir_in, uint8_t* ir_out, uint8_t* dr_in, uint8_t* dr_out)
{
    uint32_t res = 0;

    clear_reg(dr_out, MAX_DR_LEN);
    int_to_bin_array(ir_in, USERCODE, ir_len);
    insert_ir(ir_in, ir_out, ir_len, RUN_TEST_IDLE);
    insert_dr(dr_in, dr_out, 32, RUN_TEST_IDLE);    
    bin_array_to_uint32(dr_out, 32, &res);

    return res;
}

/**
 * @brief Perform read flash operation on the MAX10 FPGA, by getting an address range and 
 * incrementing the given address in each iteration with ISC_ADDRESS_SHIFT, before invoking ISC_READ.
 * @param ir_in Pointer to the input data array. Bytes array, where each
 * @param ir_out Pointer to the output data array. (bytes array)
 * @param dr_in Pointer to the input data array. (bytes array)
 * @param dr_out Pointer to the output data array. (bytes array)
 * @param start Address from which to start the flash reading.
 * @param num Amount of 32 bit words to read, starting from the start address.
*/
void max10_read_ufm_range(const uint8_t ir_len, uint8_t* ir_in, uint8_t* ir_out, uint8_t* dr_in, uint8_t* dr_out, const uint32_t start, const uint32_t num)
{
    uint32_t res = 0;

    Serial.println("\nReading flash in address iteration fashion");
    int_to_bin_array(ir_in, ir_len, ISC_ENABLE);
    insert_ir(ir_in, ir_out, ir_len, RUN_TEST_IDLE);

    // delay between ISC_Enable and read attenpt.(may be shortened)
    delay(15);
    
    for (uint32_t j=start; j < (start + num); j += 4)
    {
        // shift address instruction
        int_to_bin_array(ir_in, ir_len, ISC_ADDRESS_SHIFT);
        insert_ir(ir_in, ir_out, ir_len, RUN_TEST_IDLE);
        
        // shift address value
        clear_reg(dr_in, 32);
        int_to_bin_array(dr_in, j, 23);
        insert_dr(dr_in, dr_out, 23, RUN_TEST_IDLE);
        
        // shift read instruction
        int_to_bin_array(ir_in, ir_len, ISC_READ);
        insert_ir(ir_in, ir_out, ir_len, RUN_TEST_IDLE);

        // read data
        clear_reg(dr_in, 32);
        insert_dr(dr_in, dr_out, 32, RUN_TEST_IDLE);

        // print address and corresponding data
        int_to_bin_array(dr_out, res, 32);
        Serial.print("\n0x"); Serial.print(j, HEX);
        Serial.print(": 0x"); Serial.print(res, HEX);
        Serial.flush();
    }
}

/**
 * @brief Perform read flash operation on the MAX10 FPGA, by getting an address range and 
 * incrementing the given address in each iteration with ISC_ADDRESS_SHIFT, before invoking ISC_READ.
 * @param ir_in Pointer to the input data array.  (bytes array)
 * @param ir_out Pointer to the output data array. (bytes array)
 * @param dr_in Pointer to the input data array. (bytes array)
 * @param dr_out Pointer to the output data array. (bytes array)
 * @param start Address from which to start the flash reading.
 * @param num Amount of 32 bit words to read, starting from the start address.
*/
void max10_read_ufm_range_burst(const uint8_t ir_len, uint8_t* ir_in, uint8_t* ir_out, uint8_t* dr_in, uint8_t* dr_out, const uint32_t start, const uint32_t num)
{
    uint32_t res = 0;

    Serial.println("\nReading flash in burst fashion");    
    int_to_bin_array(ir_in, ir_len, ISC_ENABLE);
    insert_ir(ir_in, ir_out, ir_len, RUN_TEST_IDLE);

    // delay between ISC_Enable and read attenpt.(may be shortened)
    delay(15);

    // shift address instruction
    int_to_bin_array(ir_in, ir_len, ISC_ADDRESS_SHIFT);
    insert_ir(ir_in, ir_out, ir_len, RUN_TEST_IDLE);

    // shift address value
    clear_reg(dr_in, 32);
    int_to_bin_array(dr_in, start, 23);
    insert_dr(dr_in, dr_out, 23, RUN_TEST_IDLE);

    // shift read instruction
    int_to_bin_array(ir_in, ir_len, ISC_READ);
    insert_ir(ir_in, ir_out, ir_len, RUN_TEST_IDLE);

    clear_reg(dr_in, 32);

    for (uint32_t j=start ; j < (start + num); j += 4)
    {
        // read data in burst fashion
        insert_dr(dr_in, dr_out, 32, RUN_TEST_IDLE);

        // print address and corresponding data
        bin_array_to_uint32(dr_out, 32, &res);
        Serial.print("\n0x"); Serial.print(j, HEX);
        Serial.print(": 0x"); Serial.print(res, HEX);
        Serial.flush();
    }
}

/**
 * @brief User interface with the various flash reading functions.
 * @param ir_in  Pointer to the input data array.  (bytes array)
 * @param ir_out Pointer to the output data array. (bytes array)
 * @param dr_in Pointer to the input data array. (bytes array)
 * @param dr_out Pointer to the output data array. (bytes array)
*/
void max10_read_flash_session(const uint8_t ir_len, uint8_t* ir_in, uint8_t* ir_out, uint8_t* dr_in, uint8_t* dr_out)
{
    uint32_t startAddr = 0;
    uint32_t numToRead = 0;

    Serial.print("\nReading flash address range");
    
    while (1)
    {
        clear_reg(dr_in, MAX_DR_LEN);
        clear_reg(dr_out, MAX_DR_LEN);
        
        reset_tap();
        
        parse_number(NULL, 16, "\nInsert start addr > ", &startAddr);
        parse_number(NULL, 16, "\nInsert amount of words to read > ", &numToRead);
        max10_read_ufm_range_burst(ir_len, ir_in, ir_out, dr_in, dr_out, startAddr, numToRead);
            
        if (get_character("\nInput 'q' to quit loop, else to continue > ") == 'q'){
            Serial.println("Exiting...");
            break;
        }
    }
}


/**
 * According to MAX10 BSDL
 * 
 *   "FLOW_ERASE " &
    "INITIALIZE " &
        "(ISC_ADDRESS_SHIFT 23:000000 WAIT TCK 1)" &
      "(DSM_CLEAR                   WAIT 350.0e-3)," &
 *
 * @brief Erase the entire flash
 * @param ir_in Pointer to ir_in register.
 * @param ir_out Pointer to ir_out register.
 */
void max10_erase_device(const uint8_t ir_len, uint8_t* ir_in, uint8_t * ir_out, uint8_t* dr_in, uint8_t* dr_out)
{
    Serial.println("\nErasing device ...");

    clear_reg(ir_in, ir_len);
    clear_reg(dr_in, 32);

    int_to_bin_array(ir_in, ir_len, ISC_ENABLE);
    insert_ir(ir_in, ir_out, ir_len, RUN_TEST_IDLE);

    delay(1);

    int_to_bin_array(ir_in, ir_len, ISC_ADDRESS_SHIFT);
    insert_ir(ir_in, ir_out, ir_len, RUN_TEST_IDLE);
    
    int_to_bin_array(dr_in, 23, 0x00);
    insert_dr(dr_in, dr_out, 23, RUN_TEST_IDLE);

    delay(1);

    int_to_bin_array(ir_in, ir_len, DSM_CLEAR);
    insert_ir(ir_in, ir_out, ir_len, RUN_TEST_IDLE);

    delay(400);

    Serial.println("\nDone");
}

void max10_print_menu()
{
    Serial.flush();	
    Serial.print("\n\nMAX10 FPGA Menu:\n");
    Serial.print("a - Read flash\n");
    Serial.print("b - Read user code\n");
    Serial.print("c - Erase flash\n");
    Serial.print("z - Exit\n");
    Serial.flush();
}

/**
 * @brief Prompts the user to choose what to execute
 * from the available menu of max10 commands.
 */
void max10_main(const uint8_t ir_len, uint8_t* ir_in, uint8_t* ir_out, uint8_t* dr_in, uint8_t* dr_out)
{
    max10_print_menu();
    char command = get_character("\nmax10 > ");

    switch (command)
    {
    case 'a':
        // attempt to read address range from ufm
        max10_read_flash_session(ir_len, ir_in, ir_out, dr_in, dr_out);
        break;

    case 'b':
        // read user code
        Serial.print("\nUser Code: 0x"); 
        Serial.print(max10_read_user_code(ir_len, ir_in, ir_out, dr_in, dr_out), HEX);
        flush_ir_dr(ir_in, dr_out, ir_len, MAX_DR_LEN);
        break;

    case 'c':
        // erase the whole flash
        max10_erase_device(ir_len, ir_in, ir_out, dr_in, dr_out);
        break;

    case 'z':
        // quit max10 commands menu
        Serial.print("\nGoing back to main menu...");
        break;

    default:
        break;
    }
}
