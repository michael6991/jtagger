#ifndef __UTILS__
#define __UTILS__

#include <stdint.h>

#include "Arduino.h"
#include "status.h"

// Global Variables
extern String digits;

/**
 * @brief Fill the register with zeros
 * @param reg Pointer to the register to flush.
 * @param len number of cells to clear.
 */
void clear_reg(uint8_t* reg, int len);

/**
 * @brief Clear the remaining artifacts from previous operation
 * no matter what it was.
 * Cleaning this buffer is sometimes esssetial for correct operration
 * of the serial interface. (And is generaly a good practice).
 */
void clear_serial_rx_buf();

/**
 * @brief Busy waiting with a while loop for a user input.
 * Prior to loop, the function clears the rx serial buffer
 * and notifys the user that we are ready for input.
 */
void notify_input_and_busy_wait_for_serial_input(const char* message);

/**
 * @brief Sends the bytes of an array/buffer via the serial port.
 * @param buf Pointer to the buffer of data to be sent.
 * @param chunk_size Number of bytes to send to host.
 */
void send_data_to_host(uint8_t* buf, uint16_t chunk_size);

/**
 * @brief Waits for the incoming of a special character to Serial.
 * @return The input char.
*/
char serial_event(char character);

/**
 * @brief Used for various tasks where an input character needs to be received
 * from the user via the serial port.
 * @param message Message for the user.
 * @return char input from user.
 */
char get_character(const char* message);

/**
 * @brief Get string from the user via the serial port.
 * @return String input from user.
 */
String get_string(const char* message);

/**
 * @brief Used for various tasks where a number needs to be received
 * from the user via the serial port.
 */
void fetch_number(const char* message);

/**
 * @brief Used for various tasks where a hexadecimal number needs to be received
 * from the user via the serial port.
 * @param num_bytes The amount of hexadecimal characters to receive.
 * @return uint32_t representation of the hexadecimal number from user.
 */
uint32_t get_tnteger(int num_bytes);

/**
 *
 * @brief Receive a number from the user in different formats: 0x, 0b, or decimal.
 * With an option to return the fetched number in a uint32 format.
 * @param message A message for the user.
 * @param dest Destination array. Will contain user's input value.
 * @param size Size (in bytes) of the destination array.
 * @param out The constructed number.
 */
status_t parse_number(uint8_t* dest, uint32_t size, const char* message, uint32_t* out);

/**
 * @brief Convert char into a hexadecimal number
 * @param ch Character to convert.
 * @param out Reference to output result.
 * @return Status code OK if not out of bounds.
 */
status_t chr_to_hex(char ch, uint8_t* out);

/**
 * @brief Convert the content of a String object into an integer number,
 * where every byte (char) represents a single bit. 
 * Together the string represents a binary number.
 * First element of the string is the LSB.
 * Note: Max length of the given string is 32.
 * @param str Pointer to the array of bits.
 * @param out An unsigned integer that represents the the value of the array bits.
 */
status_t bin_string_to_uint32(String str, uint32_t* out);

/**
 * @brief Convert array of bytes into an integer number, where every byte
 * represents a single bit. Together the array represents a binary number.
 * First element of array is the LSB. 
 * Note: Max length of the given array is 32.
 * @param arr Pointer to the array of bits.
 * @param len Integer that represents the length of the binary array.
 * @param out Pointer to result integer that represents the the value of the bits array.
 */
status_t bin_array_to_uint32(uint8_t* arr, int len, uint32_t* out);

/**
 * @brief Convert a binary string into a bytes array arr that will represent
 * the binary value just as string. each element (byte) in arr represents a bit.
 * First element is the LSB.
 * @param arr Pointer to the output array with the binary values.
 * @param arrSize Length of the output array in bytes.
 * @param str String that represents the binary digits. (LSB is the first char of string).
 * @param strSize Length of the string object.
 * @return ok or error code.
 */
status_t bin_str_to_bin_array(uint8_t* arr, int arrSize, String str ,int strSize);

/**
 * @brief Convert a hexadecimal string into a bytes array arr that will represent
 * the binary value of the hexadecimal array. each element (byte) in arr represents a bit.
 * First element is the LSB.
 * @param arr Pointer to the output array with the binary values.
 * @param arrSize Length of the output array in bytes.
 * @param str String that represents the hexadecimal digits.
 * (LSB is the first char of string).
 * @param strSize Length of the string object.
 * @return ok or error code
 */
status_t hex_str_to_bin_array(uint8_t* arr, int arrSize, String str, int strSize);

/**
 * @brief Convert base 10 decimal string into a bytes array arr that will represent
 * the binary value of the decimal array. each element (byte) in arr represents a bit.
 * First element is the LSB.
 * @param arr Pointer to the output array with the binary values.
 * @param arrSize Length of the output array in bytes.
 * @param str String that represents the decimal digits.
 * (LSB is the first char of string).
 * @param strSize Length of the string object.
 */
status_t dec_str_to_bin_array(uint8_t* arr, int arrSize, String str, int strSize);

/**
 * @brief Convert an integer number n into a bytes array arr that will represent
 * the binary value of n. each element (byte) in arr represent a bit.
 * First element is the LSB. Largest number is a 32 bit number.
 * @param arr Pointer to the output array with the binary values.
 * @param n The integer to convert.
 * @param len Length of the output array in bytes. (max size 32)
 */
status_t int_to_bin_array(uint8_t* arr, uint32_t n, uint32_t len);

/**
 * @brief Prints the given array from last element to first.
 * @param arr Pointer to array.
 * @param len Number of cells to print from that array.
*/
void print_array(uint8_t* arr, uint32_t len);

#endif