#ifndef __MAX10_FUNCS_H__
#define __MAX10_FUNCS_H__

#include <stdint.h>

uint32_t max10_read_user_code(const uint8_t ir_len, uint8_t* ir_in, uint8_t* ir_out, uint8_t* dr_in, uint8_t* dr_out);
void max10_read_ufm_range(const uint8_t ir_len, uint8_t* ir_in, uint8_t* ir_out, uint8_t* dr_in, uint8_t* dr_out, const uint32_t start, const uint32_t num);
void max10_read_ufm_range_burst(const uint8_t ir_len, uint8_t* ir_in, uint8_t* ir_out, uint8_t* dr_in, uint8_t* dr_out, const uint32_t start, const uint32_t num);
void max10_read_flash_session(const uint8_t ir_len, uint8_t* ir_in, uint8_t* ir_out, uint8_t* dr_in, uint8_t* dr_out);
void max10_erase_device(const uint8_t ir_len, uint8_t* ir_in, uint8_t * ir_out, uint8_t* dr_in, uint8_t* dr_out);
void max10_main(const uint8_t ir_len, uint8_t* ir_in, uint8_t* ir_out, uint8_t* dr_in, uint8_t* dr_out);

#endif
