    /*
     Preserve queue-maintenance bits and clear only per-pass IDLE-skip bits.
    */
#if TLS_USE_IDLE == TLS_ENABLE
    kernel::state_mask &= kernel::STATE_MAINT_MASK;
#endif

    /*
     Capture one common timestamp before any callback executes. Scheduler
     deadlines and callbacks dispatched in this pass use the same tinyls::now
     value. The low-16 subtraction below is only the IDLE load heuristic.
    */
    now = (uint32_t)millis();

#if TLS_USE_IDLE == TLS_ENABLE
    uint16_t current_ms16 = (uint16_t)now;
    uint16_t tick_delta16 =
        (uint16_t)(current_ms16 - kernel::idle_prev_ms);

    kernel::idle_prev_ms = current_ms16;

    if (tick_delta16 > (uint8_t)TLS_IDLE_THRESHOLD_MS) {
        kernel::state_mask |= kernel::STATE_SKIP_DELTA;
    }

#endif // TLS_USE_IDLE
