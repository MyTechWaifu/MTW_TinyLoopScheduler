#if TLS_POLL == TLS_ENABLE
    /*
     ----------------------------------------------------------------------
     POLL phase
     ----------------------------------------------------------------------
    */

    /*
     phase_count snapshots the number of callbacks present at POLL entry and is
     the maximum invocation budget for this phase. Appending callbacks does not
     increase that budget. Immediate removals can reduce the queue while the
     phase is running. When a callback removes itself, the next entry shifts
     into the same index; the index is advanced only if the original callback
     still occupies that slot after it returns.
    */
    phase_count = kernel::poll_count;

#if TLS_PARTED == TLS_ENABLE
    /*
     PARTED macros need the active POLL slot as shared state because the macro
     expansion may live in a TLS_DECL_ONLY translation unit while this loop and
     the queue storage live in the implementation owner.
    */
    kernel::poll_slot = 0u;

    while (phase_count > 0u &&
           kernel::poll_slot < kernel::poll_count) {

        --phase_count;

        uint8_t index = kernel::poll_slot;
        Fn fn = kernel::poll_jobs[index];

        fn();

        if (index < kernel::poll_count &&
            kernel::poll_jobs[index] == fn) {

            ++kernel::poll_slot;
        }
    }

    kernel::poll_slot = kernel::POLL_SLOT_NONE;
#else
    /*
     Without PARTED, keep the active POLL index local so it can remain in a
     register and no persistent poll_slot byte is allocated.
    */
    uint8_t poll_index = 0u;

    while (phase_count > 0u &&
           poll_index < kernel::poll_count) {

        --phase_count;

        uint8_t index = poll_index;
        Fn fn = kernel::poll_jobs[index];

        fn();

        if (index < kernel::poll_count &&
            kernel::poll_jobs[index] == fn) {

            ++poll_index;
        }
    }
#endif
#endif // TLS_POLL
