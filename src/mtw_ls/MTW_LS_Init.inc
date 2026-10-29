    /*
     Read millis() before disabling interrupts to keep the critical section
     short.
    */
    uint32_t initial_now = (uint32_t)millis();

#if TLS_USE_IDLE == TLS_ENABLE
#include MTW_LS_PLATFORM_INIT_FILE
#endif
 
#if MTW_LS_HAS_CRITICAL
    uint8_t state = kernel::enterCritical();
#endif

    now = initial_now;
#if TLS_USE_IDLE == TLS_ENABLE
    kernel::idle_prev_ms = (uint16_t)initial_now;
#endif

#if TLS_POSTED == TLS_ENABLE
    kernel::posted_count = 0u;
#endif
#if TLS_ISR == TLS_ENABLE
    kernel::isr_request_count = 0u;
#endif
#if TLS_PERIODIC == TLS_ENABLE
    kernel::periodic_count = 0u;
#endif
#if TLS_DELAYED == TLS_ENABLE
    kernel::delayed_count = 0u;
#endif
#if TLS_POLL == TLS_ENABLE
    kernel::poll_count = 0u;
#endif
#if TLS_PARTED == TLS_ENABLE
    kernel::poll_slot = kernel::POLL_SLOT_NONE;
#endif
    kernel::state_mask = 0u;

#if MTW_LS_HAS_CRITICAL
    kernel::exitCritical(state);
#endif
