#include <stdint.h>

#include "Arduino.h"
#include "../include/utils.h
#include "../include/status.h"


void clear_reg(uint8_t* reg, int len)
{
    for (int i = 0; i < len; i++)
        reg[i] = 0;
}

void clear_serial_rx_buf()
{
    while (Serial.available()) { Serial.read(); }
}

void notify_input_and_busy_wait_for_serial_input(const char* message)
{
    clear_serial_rx_buf(); // first, clean the input buffer
    Serial.print(message); // notify user to input a value
    Serial.flush();
    while (Serial.available() == 0) {}
}

void send_data_to_host(uint8_t* buf, uint16_t chunk_size)
{
    for (int i = 0; i < chunk_size; ++i)
        Serial.write(buf[i]);

    Serial.flush();
}

char serial_event(char character)
{
  char inChar = '\0';

  while (Serial.available() == 0)
  {
    // get the new byte:
    inChar = (char)Serial.read();
    // if the incoming character equals to the argument, 
    // break from while and proceed to main loop
    // do something about it:
    if (inChar == character) {
      break;
    }
  }

  Serial.flush();
  return inChar;
}

char get_character(const char* message)
{
    char inChar[1] = {0};

    notify_input_and_busy_wait_for_serial_input(message);
    Serial.readBytesUntil('\n', inChar, 1);
    char chr = inChar[0];

#if DEBUGSERIAL
    Serial.print("\nchar: "); Serial.println(chr);
    Serial.flush();
#endif
    return chr;
}

String get_string(const char* message)
{
    String str;

    notify_input_and_busy_wait_for_serial_input(message);
    str = Serial.readStringUntil('\n');

#if DEBUGSERIAL
    Serial.print("\nstring: ");	Serial.println(str);
    Serial.print("string length = "); Serial.println(str.length());
    Serial.flush();
#endif
    return str;
}

void fetch_number(const char* message)
{
    notify_input_and_busy_wait_for_serial_input(message);
    digits = Serial.readStringUntil('\n');

#if DEBUGSERIAL
    Serial.print("\ndigits: ");	Serial.println(digits);
    Serial.print("digits length = "); Serial.println(digits.length());
    Serial.flush();
#endif
}

uint32_t get_integer(int num_bytes, const char* message)
{
    char myData[num_bytes];

    size_t m = Serial.readBytesUntil('\n', myData, num_bytes);
    myData[m] = '\0';  // insert null charcater

#if DEBUGSERIAL
    // shows: the hexadecimal string from user
    Serial.print("myData: "); Serial.println(myData);
#endif
    // convert string to hexadeciaml value
    uint32_t z = strtol(myData, nullptr, 16);

#if DEBUGSERIAL
    Serial.print("received: 0x");
    Serial.println(z, HEX); // shows 12A3
    Serial.flush();
#endif
    return z;
}

int parse_number(uint8_t* dest, uint16_t size, const char* message, uint32_t* out)
{
    int rc = OK;
    char prefix = '0';
    
    // set a default parsed value
    *out = 0;

    // fetch the digits from the Serial interface
    fetch_number(message);
    
    // received hex or bin number with prefix or a decimal witout prefix
    if (digits.length() >= 2)
        prefix = digits[1];	
    // else, no prefix, just a decimal number with 1 digit

    switch (prefix)
    {
    // user sent hexadecimal format
    case 'x':
        // cut out the digits without the prefix
        digits = digits.substring(2);

        if (dest != nullptr) {
            rc = hex_str_to_bin_array(dest, size, digits, digits.length());
            if (rc != OK)
                goto exit;
        }

        // Prepare the character array (the buffer)
        char tmp[digits.length() + 1];  // with 1 extra char for '/0'
        // convert String to char array
        digits.toCharArray(tmp, digits.length() + 1);
        // add null terminator
        tmp[digits.length()] = '\0';
        // convert to unsigned int
        *out = strtoul(tmp, NULL, 16);
        break;

    // user sent binary format
    case 'b':
        // cut out the digits without the prefix
        digits = digits.substring(2);

        // convert binary to integer
        if (dest != nullptr) {
            rc = bin_str_to_bin_array(dest, size, digits, digits.length());
            if (rc != OK)
                goto exit;
        }

        rc = bin_string_to_uint32(digits, out);
        break;

    // user sent decimal format
    default:
        if (isDigit(prefix) && digits.length() > 0)
        {
            // construct a whole decimal from the string
            char tmp[digits.length() + 1];
            digits.toCharArray(tmp, digits.length() + 1);
            tmp[digits.length()] = '\0';
            *out = strtoul(tmp, NULL, 10);

            if (dest != nullptr) {
                rc = int_to_bin_array(dest, *out, size);
            }
        } else {
            Serial.println("\nBad prefix, didn't get number");
            rc = -ERR_BAD_PREFIX_OR_SUFFIX;
        }
        break;
    }

exit:
    return rc;
}

int chr_to_hex(char ch)
{
    if (ch >= 'a' && ch <= 'f')
        return ch - 0x57;

    if (ch >= 'A' && ch <= 'F')
        return ch - 0x37;

    if (ch >= '0' && ch <= '9')
        return ch - 0x30;

    return -ERR_BAD_CONVERSION;
}

int bin_array_to_uint32(uint8_t* arr, int len, uint32_t* out)
{	
    uint32_t integer = 0;
    uint32_t mask = 1;

    if (len > 32){
        Serial.print("\nbin_array_to_uint32: array size too large");
        Serial.println("\nBad conversion.");
        Serial.flush();
        return -ERR_BAD_CONVERSION;
    }

    for (int i = 0; i < len; i++) {
        if (arr[i]) {
            integer |= mask;
        }
        mask = mask << 1;
    }

    *out = integer;

    return OK;
}

int bin_string_to_uint32(String str, uint32_t* out)
{
    uint32_t integer = 0;
    uint32_t mask = 1;

    if (str.length() > 32){
        Serial.println("\nbin_string_to_uint32: string length too large");
        Serial.println("Bad conversion.");
        return -ERR_BAD_CONVERSION;
    }

    for (int i = str.length() - 1; i >= 0; i--)
    {
        if (str[i] == '1')
            integer |= mask;

        mask = mask << 1;
    }

    *out = integer;

    return OK;
}


int bin_str_to_bin_array(uint8_t* arr, int arrSize, String str ,int strSize)
{
    int i = 0;

    if (strSize > arrSize)
    {
        Serial.print("\nbin_str_to_bin_array: size of string is larger than destination array.");
        Serial.print("\nDestination array size: "); Serial.print(arrSize);
        Serial.print("\nString requires: "); Serial.print(strSize);
        Serial.println("\nBad Conversion");
        Serial.flush();
        return -ERR_BAD_CONVERSION;
    }

    clear_reg(arr, arrSize);

    // last digit in received string is the least significant
    for (i = strSize - 1; i >= 0; i--)
        arr[strSize - 1 - i] = str[i] - 0x30; // ascii to unsigned int    

    // fill the remaining array elements with zeros
    for (i = strSize; i < arrSize; i++)
        arr[i] = 0;

    return OK;
}

int hex_str_to_bin_array(uint8_t* arr, int arrSize, String str, int strSize)
{
    int i = 0;
    int j = 0;
    int vacantBits = 0;
    uint8_t n = 0;
    
    if (strSize * 4 > arrSize)
    {
        // check how many bits left on arr that can be populated with bits from the last digit.
        vacantBits = 4 - ((strSize * 4) - arrSize);  // nibble size in bits - (str digit * nibble size) - arr size in bits

        // maybe the last digit can fit in the 1,2, or 3 bits of the last digit
        if (vacantBits <= 0)
        {
            Serial.print("\nhex_str_to_bin_array: destination array not large enough, ");
            Serial.print("size: "); Serial.print(arrSize);
            Serial.print("\nString requires size: "); Serial.print(strSize * 4);
            Serial.print("\nVacant bits: "); Serial.print(vacantBits);
            Serial.println("\nBad Conversion");
            Serial.flush();
            return -ERR_BAD_CONVERSION;
        }

        if (vacantBits == 3 && chr_to_hex(str[0]) > 7)
            Serial.print("\nWarning, last digit is to large to fit register. Expect bad conversion.");
        if (vacantBits == 2 && chr_to_hex(str[0]) > 3)
            Serial.print("\nWarning, last digit is to large to fit register. Expect bad conversion.");
        if (vacantBits == 1 && chr_to_hex(str[0]) > 1)
            Serial.print("\nWarning, last digit is to large to fit register. Expect bad conversion.");
    }

    clear_reg(arr, arrSize);

    // last digit in received string is the least significant
    for (i = strSize - 1; i >= 0; i--)
    {
        n = chr_to_hex(str[i]);
        if (n == -1)
        {
            Serial.println("\nchr_to_hex: bad digit type");
            Serial.println("Bad Conversion");
            return -ERR_BAD_CONVERSION;
        }

        // do this if we reached the last digit and arrSize < strSize * 4
        if (i == 0 && vacantBits > 0)
        {
            switch (vacantBits)
            {
            case 3: // only hex digits [0, 7] may be written to last 3 bits of arr
                if (n & 0x04)
                    arr[j + 2] = 1;

            case 2: // only hex digits [0, 3] may be written to last 2 bits of arr
                if (n & 0x02)
                    arr[j + 1] = 1;

            case 1: // only hex digits [0, 1] may be written to last 1 bit of arr
                if (n & 0x01)
                    arr[j] = 1;
            default:
                break;  // break out of switch statement
            }
            break;  // break out of for loop
        }

        // copy nibble bits to destination array (LSB first)
        if (n & 0x01)
            arr[j] = 1;
        if (n & 0x02)
            arr[j + 1] = 1;
        if (n & 0x04)
            arr[j + 2] = 1;
        if (n & 0x08)
            arr[j + 3] = 1;

        j += 4; // update destination array index
    }

    return OK;
}

int dec_str_to_bin_array(uint8_t* arr, int arrSize, String str, int strSize)
{
    int i = 0;
    int j = 0;
    int vacantBits = 0;
    uint8_t n = 0;
    
    if (strSize * 4 > arrSize)
    {
        // check how many bits left on arr that can be populated with bits from the last digit.
        vacantBits = 4 - ((strSize * 4) - arrSize);  // nibble size in bits - (str digit * nibble size) - arr size in bits

        // maybe the last digit can fit in the 1,2, or 3 bits of the last digit
        if (vacantBits <= 0)
        {
            Serial.print("\ndecStrToBinArray: destination array not large enough");
            Serial.print("\nDestination array size in bits: "); Serial.print(arrSize);
            Serial.print("\nString requires size: "); Serial.print(strSize * 4);
            Serial.print("\nVacant bits: "); Serial.print(vacantBits);
            Serial.println("\nBad Conversion");
            Serial.flush();
            return -ERR_BAD_CONVERSION;
        }
        if (vacantBits == 3 && chr_to_hex(str[0]) > 7)
            Serial.print("\nWarning, last digit is to large to fit register. Expect bad conversion.");
        if (vacantBits == 2 && chr_to_hex(str[0]) > 3)
            Serial.print("\nWarning, last digit is to large to fit register. Expect bad conversion.");
        if (vacantBits == 1 && chr_to_hex(str[0]) > 1)
            Serial.print("\nWarning, last digit is to large to fit register. Expect bad conversion.");
    }

    clear_reg(arr, arrSize);

    // last digit in received string is the least significant
    for (i = strSize - 1; i >= 0; i--)
    {
        n = chr_to_hex(str[i]);
        if (n == -1)
        {
            Serial.println("\nchr_to_hex: bad digit type");
            Serial.println("Bad Conversion");
            return -ERR_BAD_CONVERSION;
        }

        // do this if we reached the last digit and arrSize < strSize * 4
        if (i == 0 && vacantBits > 0)
        {
            switch (vacantBits)
            {
            case 3: // only hex digits [0, 7] may be written to last 3 bits of arr
                if (n & 0x04)
                    arr[j + 2] = 1;

            case 2: // only hex digits [0, 3] may be written to last 2 bits of arr
                if (n & 0x02)
                    arr[j + 1] = 1;

            case 1: // only hex digits [0, 1] may be written to last 1 bit of arr
                if (n & 0x01)
                    arr[j] = 1;

            default: // break out of switch statement
                break;
            }
            break; // break out of for loop
        }

        // copy nibble bits to destination array (LSB first)
        if (n & 0x01)
            arr[j] = 1;
        if (n & 0x02)
            arr[j + 1] = 1;
        if (n & 0x04)
            arr[j + 2] = 1;
        if (n & 0x08)
            arr[j + 3] = 1;

        j += 4;  // update destination array index
    }

    return OK;
}

int int_to_bin_array(uint8_t* arr, uint32_t n, uint32_t len)
{
    uint32_t mask = 1;

    if (len > 32)
    {
        Serial.print("\nint_to_bin_array: array size is larger than 32");
        Serial.println("\nBad parameter");
        return -ERR_BAD_PARAMETER;
    }

    clear_reg(arr, len);

    for (int i = 0; i < len; i++)
    {
        arr[i] = (n & mask) ? 1 : 0;
        mask <<= 1;
    }

    return OK;
}

void print_array(uint8_t* arr, uint32_t len)
{
    for (int16_t i = len - 1; i >= 0; i--)
        Serial.print(arr[i], HEX);

    Serial.flush();
}
