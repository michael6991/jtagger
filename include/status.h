#ifndef __STATUS__H__
#define __STATUS__H__

/**
 * Error return code definitions
 */
#define OK                            0
#define ERR_GENERAL                   1
#define ERR_BAD_CONVERSION            2
#define ERR_RESOURCE_EXHAUSTED        3
#define ERR_OUT_OF_BOUNDS             5
#define ERR_BAD_IDCODE                6
#define ERR_BAD_PREFIX_OR_SUFFIX      7
#define ERR_BAD_TAP_STATE             8
#define ERR_BAD_PARAMETER             9
#define ERR_INVALID_IR_OR_DR_LEN      10
#define ERR_TDO_STUCK_AT_0            11
#define ERR_TDO_STUCK_AT_1            12
#define ERR_TAP_DEVICE_UNAVAILABLE    13
#define ERR_TAP_DEVICE_ALREADY_ACTIVE 14
#define ERR_TAP_DEVICE_REMOVE_ISSUE   15

typedef int status_t;

#endif
