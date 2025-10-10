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

status_t chain_tap_add(tap_t* taps, const uint32_t index, const char* name, const uint32_t idcode, const uint32_t ir_len)
{
    if (index >= MAX_ALLOWED_TAPS)
        return -ERR_OUT_OF_BOUNDS;
    
    if (taps[index].active)
        return -ERR_TAP_DEVICE_ALREADY_ACTIVE;
    
    if (ir_len > MAX_IR_LEN)
        return -ERR_INVALID_IR_OR_DR_LEN;

    // the new tap should be the first one in chain or adjacent
    // after an exisiting active TAP
    if (index != 0 && chain_active_devices != 0)
    {
        if (chain_active_devices - 1 != index)
            return -ERR_BAD_PARAMETER;

        if (!taps[index - 1].active)
            return -ERR_TAP_DEVICE_UNAVAILABLE;
    }        

    strncpy(taps[index].name, name, 32);
    taps[index].idcode = idcode;
    taps[index].ir_len = ir_len;
    taps[index].ir_in_idx = 0;
    taps[index].ir_out_idx = 0;
    taps[index].active = false;

    return OK;
}

status_t chain_tap_remove(tap_t* taps, const uint32_t index)
{
    if (index >= MAX_ALLOWED_TAPS)
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

status_t chain_tap_activate(tap_t* taps, const uint32_t index)
{
    if (index >= MAX_ALLOWED_TAPS)
        return -ERR_OUT_OF_BOUNDS;
    
    if (taps[index].active)
        return -ERR_TAP_DEVICE_ALREADY_ACTIVE;

    // make sure the activated device has IR length that
    // does not exceeds the available maximum we set
    uint32_t available_ir_len = MAX_IR_LEN - chain_ir_len;
    if (taps[index].ir_len > available_ir_len)
        return -ERR_RESOURCE_EXHAUSTED;

    // if tap device is first in chain
    if (index == 0) {
        taps[index].ir_in_idx = 0;
        taps[index].ir_out_idx = taps[index].ir_len - 1;
    } else {
        taps[index].ir_in_idx = taps[index - 1].ir_out_idx + 1;
        taps[index].ir_out_idx = taps[index - 1].ir_out_idx + 1 + taps[index].ir_len - 1;
    }

    taps[index].active = true;
    chain_ir_len += taps[index].ir_len;
    chain_active_devices++;

    return OK;
}

status_t chain_tap_deactivate(tap_t* taps, const uint32_t index)
{
    if (index >= MAX_ALLOWED_TAPS)
        return -ERR_OUT_OF_BOUNDS;

    chain_ir_len -= taps[index].ir_len;
    chain_active_devices--;
    taps[index].active = false;

    return OK;
}

status_t chain_tap_selector(tap_t* taps, const uint32_t index, tap_t* out, uint8_t* ir_in, uint8_t* ir_out)
{
    if (index >= MAX_ALLOWED_TAPS)
        return -ERR_OUT_OF_BOUNDS;

    if (!taps[index].active)
        return -ERR_TAP_DEVICE_UNAVAILABLE;

    // put all devices to bypass.
    // bypass is standarized as the "ones" instruction
    // i.e IR is filled with ones
    for (uint32_t i = 0; i < chain_ir_len; i++)
        ir_in[i] = 1;

    Serial.print("\nchain_tap_selector putting all active devices to bypass");
    insert_ir(ir_in, ir_out, chain_ir_len, RUN_TEST_IDLE);
    out = &taps[index];

    Serial.print("\nSelected TAP device: "); Serial.print(index, DEC);
    Serial.print(" idcode: "); Serial.print(out->idcode, HEX);
    Serial.print(" ir len: "); Serial.println(out->ir_len, DEC);
    Serial.flush();

    return OK;
}

void chain_print_active_taps(tap_t* taps)
{
    Serial.print("\nTotal active devices: "); Serial.print(chain_active_devices, DEC);
    Serial.print("\nTotal IR length: "); Serial.print(chain_ir_len, DEC);
    Serial.flush();

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
