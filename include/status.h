#ifndef __STATUS__H__
#define __STATUS__H__

/**
 * Error return code definitions
 */
#define OK                            0
#define ERR_GENERAL                   1
#define ERR_BAD_CONVERSION            2
#define ERR_OUT_OF_BOUNDS             3
#define ERR_BAD_IDCODE                4
#define ERR_BAD_PREFIX_OR_SUFFIX      5
#define ERR_BAD_TAP_STATE             6
#define ERR_BAD_PARAMETER             7
#define ERR_INVALID_IR_OR_DR_LEN      8
#define ERR_TDO_STUCK_AT_0            9
#define ERR_TDO_STUCK_AT_1            10
#define ERR_TAP_DEVICE_UNAVAILABLE    11
#define ERR_TAP_DEVICE_ALREADY_ACTIVE 12

typedef int status_t;

#endif
