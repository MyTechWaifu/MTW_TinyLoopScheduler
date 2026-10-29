// Platform capabilities; public defaults are resolved by MTW_LS_Features.h.
#define MTW_LS_HAS_IDLE     1
#define MTW_LS_HAS_ISR      1
#define MTW_LS_HAS_CRITICAL 1

// These paths are relative to the shared mtw_ls/ fragments that include them.
#define MTW_LS_PLATFORM_CRITICAL_FILE "platforms/MTW_LS_Platform_AVR_Critical.inc"
#define MTW_LS_PLATFORM_INIT_FILE     "platforms/MTW_LS_Platform_AVR_Init.inc"
#define MTW_LS_PLATFORM_IDLE_FILE     "platforms/MTW_LS_Platform_AVR_Idle.inc"

#include <avr/sleep.h>
