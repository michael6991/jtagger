/** @file main.ino
 *
 * @brief Basic Jtagger, built for simple purposes such as:
 * detecting existance of a scan chain. read idcode, insert ir and dr,
 * and other simple or complex implementations of custom made operations.
 *
 * Can be simply modified for your requirements.
 *
 * @author Michael Vigdorchik
 */
#include "Arduino.h"

#include "include/art.h"
#include "include/main.h"
#include "include/status.h"
#include "include/utils.h"
#include "src/jtag_drv/jtag_drv.h"
#include "src/chain/chain.h"
#include "src/max10/max10_funcs.h"

// DR content to input into chain's real DR
uint8_t dr_out[MAX_DR_LEN];
// DR content to store from output of chain's real DR
uint8_t dr_in[MAX_DR_LEN];
// IR content to input into chain's real IR
uint8_t ir_in[MAX_IR_LEN];
// IR content to store from output of chain's real IR
uint8_t ir_out[MAX_IR_LEN];

tap_t taps[MAX_ALLOWED_TAPS];


void print_main_menu()
{
    Serial.flush();	
    Serial.print("\n---------\nMain Menu\n\n");
    Serial.print("\tAll numerical parameters should be passed in the format: {0x || 0b || decimal}\n\n");
    Serial.print("a - Add new TAP device to chain\n");
    Serial.print("b - Activate TAP device in chain\n");
    Serial.print("c - Connect to chain\n");
    Serial.print("d - Discovery\n");
    Serial.print("i - Insert IR\n");
    Serial.print("l - Detect DR length\n");
    Serial.print("p - Print active TAP devices in chain\n");
    Serial.print("r - Insert DR\n");
    Serial.print("s - Select active TAP device to work on\n");
    Serial.print("t - Reset TAP state machine\n");
    Serial.print("q - Toggle TRST line\n");
    Serial.print("h - Show this menu\n");
    Serial.print("z - Exit\n");
    Serial.flush();
}

void setup()
{
    // initialize mode for standard IEEE 1149.1 JTAG pins
    pinMode(TCK, OUTPUT);
    pinMode(TMS, OUTPUT);
    pinMode(TDI, OUTPUT);
    pinMode(TDO, INPUT_PULLUP);
    pinMode(TRST, OUTPUT);

    // initialize pins state
    digitalWrite(TCK, 0);
    digitalWrite(TMS, 1);
    digitalWrite(TDI, 1);
    digitalWrite(TRST, 1);

    // initialize possible TAPs in chain
    chain_taps_init(taps);
    clear_reg(ir_in, MAX_IR_LEN);
    clear_reg(ir_out, MAX_IR_LEN);
    clear_reg(dr_in, MAX_DR_LEN);
    clear_reg(dr_out, MAX_DR_LEN);

    // initialize serial communication
    Serial.begin(115200);
    while (!Serial) { }
    Serial.setTimeout(500); // set timeout for various serial R/W funcs
}

void loop()
{
    status_t rc = OK;
    uint32_t num, dr_len = 0;
    uint32_t nbits, first_ir, final_ir, max_dr_len = 0;
    uint32_t chain_ir_len, chain_idcode = 0;
    uint32_t which_tap = 0;

    uint32_t tmp_idcode = 0;
    uint32_t tmp_ir_len = 0;

    String str;
    str.reserve(32);
    memset((void*)str.c_str(), '\0', 32);

    tap_t* cur_tap = nullptr;
    current_state = TEST_LOGIC_RESET;
    char command = '0';

    // to begin session
    String start = get_string("Insert 'start' > ");
    if (start != "start") {
        Serial.println("\nInvalid 'start' response from host");
        goto inf_loop;
    }

    print_welcome();

    // try to detect the existance of a chain and set a single device to the chain
    chain_tap_add(taps, which_tap, "device 0", 0, 2);
    cur_tap = &taps[0];

    // detect chain and read idcode.
    // at the initial detection we reference the first tap device: taps[0].
    rc = detect_chain(&cur_tap->ir_len, &cur_tap->idcode);
    if (rc != OK) {
        goto inf_loop;
    }

    // ir len cannot be too large and idcode's LSB must be 1
    // TODO: device can be without a default IDCODE connected between TDI and TDO
    if ((cur_tap->ir_len > MAX_IR_LEN) || !(cur_tap->idcode & 0x01)) {
        Serial.println("\nInvalid IDCODE value or IR length");
        goto inf_loop;
    }

    // successfuly found active device in chain so activate the initial tap in chain
    chain_tap_activate(taps, which_tap);

    reset_tap();
    print_main_menu();

    while (true)
    {
        num = 0;
        command = get_character("\ncmd > ");

        switch (command)
        {
        // add new TAP device to chain
        case 'a':
            chain_print_taps(taps);

            rc = parse_number(nullptr, 32, "\nIndex in chain > ", &which_tap);
            if (rc != OK) break;

            str = get_string("\nName of device (31 chars) > ");
            // TODO check str

            rc = parse_number(nullptr, 32, "\nIdcode > ", &tmp_idcode);
            if (rc != OK) break;
            
            rc = parse_number(nullptr, 32, "\nIR length > ", &tmp_ir_len);            if (rc != OK) break;

            rc = chain_tap_add(taps, which_tap, str.c_str(), tmp_idcode, tmp_ir_len);
            if (rc != OK) break;

            chain_print_taps(taps);
            memset((void*)str.c_str(), '\0', 32);
            break;

        // activate TAP device in chain
        case 'b':
            chain_print_taps(taps);
            rc = parse_number(nullptr, 32, "Select which TAP device index you want to activate (Decimal) > ", &which_tap);
            if (rc != OK) {
                Serial.println("\nCould not get valid TAP device index");
                break;
            }

            rc = chain_tap_activate(taps, which_tap);
            if (rc != OK) {
                Serial.print("\nError selecting tap device: "); Serial.print(which_tap, DEC);
                Serial.println("TAP device is inactive or was not discovered properly");
                break;
            }

            Serial.print("\nSelected TAP device: "); Serial.println(which_tap, DEC);
            break;

        // attempt to connect to chain and read idcode
        case 'c':
            rc = detect_chain(&chain_ir_len, &chain_idcode);
            if (rc != OK) break;
            Serial.print("Chain IR length: "); Serial.print(chain_ir_len);
            break;

        // discovery of existing IRs
        case 'd':
            rc = parse_number(nullptr, 20, "First IR > ", &first_ir);
            if (rc != OK) break;
            rc = parse_number(nullptr, 20, "Final IR > ", &final_ir);
            if (rc != OK) break;
            rc = parse_number(nullptr, 20, "Max allowed DR length > ", &max_dr_len);
            if (rc != OK) break;

            discovery(first_ir, final_ir, max_dr_len, cur_tap->ir_len, ir_in);
            break;

        // insert ir
        case 'i':
            // TODO: debug the following line
            rc = parse_number(&ir_in[cur_tap->ir_in_idx], cur_tap->ir_len, "\nShift IR > ", &num);
            if (rc != OK) break;

            Serial.print("\nIR  in: ");
            print_array(ir_in, cur_tap->ir_len);
            if (get_character("\ncontinue (y/n)? > ") == 'n')
                break;
            
            // insert the existing binary value in the ir_in global register
            // and save the output to the ir_out global register
            insert_ir(ir_in, ir_out, cur_tap->ir_len, RUN_TEST_IDLE);

            // print the hex value if length is not to large
            if (cur_tap->ir_len <= 32) 
            {
                bin_array_to_uint32(ir_in, cur_tap->ir_len, &num);
                Serial.print(" | 0x"); Serial.print(num, HEX);
            }

            Serial.print("\nIR out: ");
            print_array(ir_out, cur_tap->ir_len);

            // print the hex value if length is not to large
            if (cur_tap->ir_len <= 32)
            {
                bin_array_to_uint32(ir_out, cur_tap->ir_len, &num);
                Serial.print(" | 0x"); Serial.print(num, HEX);
            }
            break;

        // detect current dr length
        case 'l':
            dr_len = detect_dr_len(ir_in, cur_tap->ir_len, 4);
            if (dr_len == 0) {
                Serial.println("\nDidn't find the current DR length, TDO is stuck");
            }
            else {
                Serial.print("\nDR length: ");
                Serial.print(dr_len);
            }
            break;

        // print active TAPs chain
        case 'p':
            chain_print_taps(taps);
            break;

        // insert dr
        case 'r':
            rc = parse_number(nullptr, 32, "Enter amount of bits to shift > ", &nbits);
            if (nbits == 0 || rc != OK)
                break;

            rc = parse_number(dr_in, nbits, "\nShift DR > ", &nbits);
            if (rc != OK) break;

            insert_dr(dr_in, dr_out, nbits, RUN_TEST_IDLE);

            Serial.print("\nDR  in: ");
            print_array(dr_in, nbits);
            
            // print the hex value if lenght is not large enough
            if (nbits <= 32)
            {
                bin_array_to_uint32(dr_in, nbits, &num); // TODO needed after parseNum ?
                Serial.print(" | 0x"); Serial.print(num, HEX);
            }
            
            Serial.print("\nDR out: ");
            print_array(dr_out, nbits);
            
            // print the hex value if lenght is not large enough
            if (nbits <= 32)
            {
                bin_array_to_uint32(dr_out, nbits, &num);  // TODO same as above
                Serial.print(" | 0x"); Serial.print(num, HEX);
            }
            break;

        case 's':
            // select active tap to work on
            chain_print_taps(taps);
            rc = parse_number(nullptr, 32, "Selecet the TAP device index (Decimal) > ", &which_tap);
            if (rc != OK) {
                Serial.println("\nCould not get valid TAP device index");
                break;
            }
            
            rc = chain_tap_selector(taps, which_tap, cur_tap, ir_in, ir_out);
            if (rc != OK) {
                Serial.print("\nError selecting tap device: "); Serial.print(which_tap, DEC);
                Serial.println("TAP device is inactive or was not discovered properly");
                break;
            }

            Serial.print("\nSelected TAP device: "); Serial.println(which_tap, DEC);
            break;

        case 't':
            Serial.println("Resetting TAP");
            reset_tap();
            break;
        
        case 'q':
            Serial.println("Toggling TRST line");
            digitalWrite(TRST, 0);
            HC; HC; HC; HC; HC; HC; HC; HC;
            digitalWrite(TRST, 1);
            break;

        case 'h':
            print_main_menu();
            break;

        case 'z':
            Serial.print("\nExiting...\nReset Arduino to start again");
            reset_tap();
            goto inf_loop;

        default:
            Serial.println("Invalid Command");
            break;
        }
    }

    reset_tap();

inf_loop:
    Serial.println("\n[!] You must reset the Arduino at this point [!]");
    Serial.end();
    while(1); // loop in place
}
