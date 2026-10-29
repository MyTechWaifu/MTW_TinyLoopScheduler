#if TLS_USE_IDLE == TLS_ENABLE
    /*
     ----------------------------------------------------------------------
     IDLE backend
     ----------------------------------------------------------------------
    */

    /*
     IDLE intentionally precedes POSTED and POLL. The lower state bits suppress
     sleep when a deferred ISR callback ran, PERIODIC work was due, DELAYED work
     was due, or the interval between tick() entries exceeded the selected
     threshold. Queue maintenance alone does not suppress sleep.

     POSTED and POLL deliberately do not set skip bits. Even when either has
     pending work, the MCU enters SLEEP_MODE_IDLE here when the IDLE skip mask
     is zero and continues with those phases only after a normal interrupt
     wakes it.
    */
    if ((kernel::state_mask & kernel::STATE_IDLE_MASK) == 0u) {
#include MTW_LS_PLATFORM_IDLE_FILE
    }

#endif // TLS_USE_IDLE
