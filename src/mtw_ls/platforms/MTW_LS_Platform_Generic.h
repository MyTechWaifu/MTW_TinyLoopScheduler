// Arduino-compatible fallback. Use TLS only from one main execution context.
// No sleep backend, ISR request support, or critical-section backend is provided.
#define MTW_LS_HAS_IDLE     0
#define MTW_LS_HAS_ISR      0
#define MTW_LS_HAS_CRITICAL 0
