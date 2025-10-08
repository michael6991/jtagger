#include <Arduino.h>
#include <string.h>

#include "chain.h"
#include "../jtag_drv/jtag_drv.h"
#include "../../include/main.h"
#include "../../include/status.h"
#include "../../include/utils.h"


// Count number of active devices in chain
static uint32_t chain_active_devices = 0;
static uint32_t chain_ir_len = 0;

uint32_t chain_get_active_devices() { return chain_active_devices; }

uint32_t chain_get_total_ir_len() { return chain_ir_len; }

void chain_taps_init(tap_t* taps)
{
    for (size_t i = 0; i < MAX_ALLOWED_TAPS; i++)
    {
        memset(taps[i].name, 0, 32);
        taps[i].idcode = 0;
        taps[i].ir_len = 0;
        taps[i].ir_in_idx = 0;
        taps[i].ir_out_idx = 0;
        taps[i].active = false;
    }

    chain_active_devices = 0;
}

status_t chain_tap_add(tap_t* taps, int index, const char* name, const uint32_t idcode, const int ir_len)
{
    if (index < 0 || index >= MAX_ALLOWED_TAPS)
        return -ERR_OUT_OF_BOUNDS;
    
    if (taps[index].active)
        return -ERR_TAP_DEVICE_ALREADY_ACTIVE;
    
    if (ir_len > MAX_IR_LEN || ir_len < 0)
        return -ERR_INVALID_IR_OR_DR_LEN;

    strncpy(taps[index].name, name, 32);
    taps[index].idcode = idcode;
    taps[index].ir_len = ir_len;
    taps[index].ir_in_idx = 0;
    taps[index].ir_out_idx = 0;
    taps[index].active = false;

    return OK;
}

status_t chain_tap_remove(tap_t* taps, int index)
{
    if (index < 0 || index >= MAX_ALLOWED_TAPS)
        return -ERR_OUT_OF_BOUNDS;
    
    // tap should be deactivated first
    if (taps[index].active)
        return -ERR_TAP_DEVICE_ALREADY_ACTIVE;
    
    memset(taps[index].name, 0, 32);
    taps[index].idcode = 0;
    taps[index].ir_len = 0;
    taps[index].ir_in_idx = 0;
    taps[index].ir_out_idx = 0;

    return OK;
}

status_t chain_tap_activate(tap_t* taps, int index, const int ir_in_idx, const int ir_out_idx)
{
    if (index < 0 || index >= MAX_ALLOWED_TAPS)
        return -ERR_OUT_OF_BOUNDS;
    
    if (taps[index].active)
        return -ERR_TAP_DEVICE_ALREADY_ACTIVE;

    if ((ir_in_idx < 0) || (ir_out_idx < 0) || 
        (ir_in_idx >= MAX_IR_LEN) || (ir_out_idx >= MAX_IR_LEN))
    {
        return -ERR_OUT_OF_BOUNDS;
    }

    taps[index].ir_in_idx = ir_in_idx;
    taps[index].ir_out_idx = ir_out_idx;
    taps[index].active = true;
    chain_ir_len += taps[index].ir_len;
    chain_active_devices++;

    return OK;
}

status_t chain_tap_deactivate(tap_t* taps, int index)
{
    if (index < 0 || index >= MAX_ALLOWED_TAPS)
        return -ERR_OUT_OF_BOUNDS;

    chain_ir_len -= taps[index].ir_len;
    chain_active_devices--;
    taps[index].active = false;

    return OK;
}

status_t chain_tap_selector(tap_t* taps, int index, tap_t* out, uint8_t* ir_in, uint8_t* ir_out)
{
    if (index < 0 || index >= MAX_ALLOWED_TAPS)
        return -ERR_OUT_OF_BOUNDS;

    if ((taps[index].ir_len > 0) || (!taps[index].active))
        return -ERR_TAP_DEVICE_UNAVAILABLE;

    // prepare payload that puts all other devices into bypass
    for (int i = 0; i < MAX_ALLOWED_TAPS; i++)
    {
        // bypass is standarized as the "ones" instruction
        // i.e IR is filled with ones
        int_to_bin_array(&ir_in[taps[i].ir_in_idx],
                        (1 << taps[i].ir_len) - 1,
                        taps[i].ir_len);
    }
    // TODO: DEBUG ONLY, delete later
    Serial.print("\nchain_tap_selector IR in: ");
    print_array(ir_in, 32);
    Serial.println();

    // insert the payload
    insert_ir(ir_in, ir_out, chain_ir_len, RUN_TEST_IDLE);
    out = &taps[index];

    Serial.print("\nSelected TAP device: "); Serial.print(index, DEC);
    Serial.print(" idcode: "); Serial.print(out->idcode, HEX);
    Serial.print(" ir len: "); Serial.println(out->ir_len, DEC);

    return OK;
}

void chain_print_active_taps(tap_t* taps)
{
    for (int i = 0; i < MAX_ALLOWED_TAPS; i++)
    {
        if (taps[i].active)
        {
            Serial.print("\nTAP device "); Serial.print(i, DEC); Serial.print("active");
            Serial.print("\nname: "); Serial.print(taps[i].name); Serial.print(" idcode: "); Serial.print(taps[i].idcode, HEX);
            Serial.print("\nir len: "); Serial.println(taps[i].ir_len, DEC);
            Serial.flush();
        }
    }
}
