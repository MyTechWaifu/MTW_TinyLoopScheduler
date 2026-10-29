#if TLS_ISR == TLS_ENABLE
    /*
     ----------------------------------------------------------------------
     ISR request phase
     ----------------------------------------------------------------------
    */

    /*
     Fast path: avoid entering a critical section on the common empty-buffer
     path. A request arriving immediately after this check is processed by the
     next tick(), which is equivalent to a request arriving after this phase.
    */
    if (kernel::isr_request_count > 0u) {
        /*
            At least one deferred ISR callback will be executed, so
            this pass must not enter IDLE before POSTED and POLL.
        */

#if TLS_USE_IDLE == TLS_ENABLE
        kernel::state_mask |= kernel::STATE_SKIP_ISR;
#endif
     
        while (1) {
            Fn fn;
            uint8_t state = kernel::enterCritical();

            if (kernel::isr_request_count == 0u) {
                kernel::exitCritical(state);
                break;
            }

            /*
             Pop under the critical section, then execute outside it.
             Interrupts remain disabled for only a few instructions, and the
             deferred callback runs in normal main context.
            */
            --kernel::isr_request_count;
            fn = kernel::isr_requests[kernel::isr_request_count];

            kernel::exitCritical(state);

            fn();
        }
    }

#endif // TLS_ISR

