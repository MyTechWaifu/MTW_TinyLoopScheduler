#if TLS_PERIODIC == TLS_ENABLE
    /*
     ----------------------------------------------------------------------
     PERIODIC maintenance: compact/rebuild at phase start
     ----------------------------------------------------------------------
     */

    if (kernel::state_mask &
        (kernel::STATE_COMPACT_PERIODIC | kernel::STATE_REBUILD_PERIODIC)) {

        if (kernel::state_mask & kernel::STATE_COMPACT_PERIODIC) {
            uint8_t write = 0u;

            /*
             Compact pass: remove tombstones left by removeEvery(). Physical
             records may move, so rebuild periodic_order[] as packed indices.
            */
            for (uint8_t read = 0u; read < kernel::periodic_count; ++read) {
                if (kernel::periodic[read].fn == 0) {
                    continue;
                }

                if (write != read) {
                    kernel::periodic[write] = kernel::periodic[read];
                }

                kernel::periodic_order[write] = write;
                ++write;
            }

            kernel::periodic_count = write;
        }

        /*
         Common sort path.

         Without compaction, physical indices remain valid. New jobs were
         already appended to periodic_order[], and rescheduled jobs keep their
         existing indices. Re-sort the existing order instead of rebuilding
         it as 0..periodic_count-1.

         After compaction, periodic_order[] was rebuilt above because physical
         record indices changed.
        */
        for (uint8_t i = 1u; i < kernel::periodic_count; ++i) {
            uint8_t key = kernel::periodic_order[i];
            uint32_t key_deadline = kernel::periodic[key].deadline_ms;
            uint8_t j = i;

            while (j > 0u) {
                uint8_t prev = kernel::periodic_order[(uint8_t)(j - 1u)];

                if ((int32_t)(key_deadline -
                      kernel::periodic[prev].deadline_ms) >= 0) {
                    break;
                }

                kernel::periodic_order[j] = prev;
                --j;
            }

            kernel::periodic_order[j] = key;
        }

        kernel::state_mask &= (uint8_t)~(kernel::STATE_COMPACT_PERIODIC |
                                         kernel::STATE_REBUILD_PERIODIC);
    }

    /*
     ----------------------------------------------------------------------
     PERIODIC due phase
     ----------------------------------------------------------------------
     */

    if (kernel::periodic_count > 0u) {
        phase_count = kernel::periodic_count;

        uint8_t index = kernel::periodic_order[0];

        /*
         Deadline comparison is signed-delta based. This is the standard
         millis()-safe form: it remains correct across uint32_t wrap-around
         as long as compared intervals are below 2^31 ms.
         */
        if ((int32_t)(now - kernel::periodic[index].deadline_ms) >= 0) {
#if TLS_USE_IDLE == TLS_ENABLE
            kernel::state_mask |= kernel::STATE_SKIP_PERIODIC;
#endif

            Fn fn = kernel::periodic[index].fn;

            if (fn != 0) {
                kernel::Period period_ms = kernel::periodic[index].period_ms;
                uint32_t old_deadline = kernel::periodic[index].deadline_ms;
                uint32_t next_deadline = old_deadline + (uint32_t)period_ms;

                /*
                 If the scheduler is only slightly late, keep the old cadence
                 by adding one period to the previous deadline. If it is very
                 late, reschedule from now to avoid a catch-up burst.
                 */
                if ((int32_t)(now - next_deadline) >= 0) {
                    kernel::periodic[index].deadline_ms =
                        now + (uint32_t)period_ms;
                } else {
                    kernel::periodic[index].deadline_ms = next_deadline;
                }

                fn();
            }

            for (uint8_t i = 1u; i < phase_count; ++i) {
                index = kernel::periodic_order[i];
                fn = kernel::periodic[index].fn;

                if (fn == 0) {
                    continue;
                }

                if ((int32_t)(now - kernel::periodic[index].deadline_ms) < 0) {
                    break;
                }

                kernel::Period period_ms = kernel::periodic[index].period_ms;
                uint32_t old_deadline = kernel::periodic[index].deadline_ms;
                uint32_t next_deadline = old_deadline + (uint32_t)period_ms;

                if ((int32_t)(now - next_deadline) >= 0) {
                    kernel::periodic[index].deadline_ms =
                        now + (uint32_t)period_ms;
                } else {
                    kernel::periodic[index].deadline_ms = next_deadline;
                }

                fn();
            }

            kernel::state_mask |= kernel::STATE_REBUILD_PERIODIC;
        }
    }

#endif // TLS_PERIODIC

