#if TLS_DELAYED == TLS_ENABLE
    /*
     ----------------------------------------------------------------------
     DELAYED maintenance: compact/rebuild at phase start
     ----------------------------------------------------------------------
     */

    if (kernel::state_mask &
        (kernel::STATE_COMPACT_DELAYED | kernel::STATE_REBUILD_DELAYED)) {

        if (kernel::state_mask & kernel::STATE_COMPACT_DELAYED) {
            uint8_t write = 0u;

            /*
             Compact pass: remove DELAYED jobs that have already fired or
             were deleted. Physical records may move, so rebuild
             delayed_order[] as packed indices.
            */
            for (uint8_t read = 0u; read < kernel::delayed_count; ++read) {
                if (kernel::delayed[read].fn == 0) {
                    continue;
                }

                if (write != read) {
                    kernel::delayed[write] = kernel::delayed[read];
                }

                kernel::delayed_order[write] = write;
                ++write;
            }

            kernel::delayed_count = write;
        }

        /*
         Common sort path.

         Without compaction, physical indices remain valid. New jobs were
         already appended to delayed_order[]. Re-sort the existing order
         instead of rebuilding it as 0..delayed_count-1.

         After compaction, delayed_order[] was rebuilt above because physical
         record indices changed.
        */
     
        for (uint8_t i = 1u; i < kernel::delayed_count; ++i) {
            uint8_t key = kernel::delayed_order[i];
            uint32_t key_deadline = kernel::delayed[key].deadline_ms;
            uint8_t j = i;

            while (j > 0u) {
                uint8_t prev = kernel::delayed_order[(uint8_t)(j - 1u)];

                if ((int32_t)(key_deadline -
                      kernel::delayed[prev].deadline_ms) >= 0) {
                    break;
                }

                kernel::delayed_order[j] = prev;
                --j;
            }

            kernel::delayed_order[j] = key;
        }

        kernel::state_mask &= (uint8_t)~(kernel::STATE_COMPACT_DELAYED |
                                         kernel::STATE_REBUILD_DELAYED);
    }

    /*
     ----------------------------------------------------------------------
     DELAYED due phase
     ----------------------------------------------------------------------
     */

    if (kernel::delayed_count > 0u) {
        phase_count = kernel::delayed_count;

        uint8_t index = kernel::delayed_order[0];

        if ((int32_t)(now - kernel::delayed[index].deadline_ms) >= 0) {
#if TLS_USE_IDLE == TLS_ENABLE
            kernel::state_mask |= kernel::STATE_SKIP_DELAYED;
#endif

            Fn fn = kernel::delayed[index].fn;

            if (fn != 0) {
                /*
                 Mark the record expired before calling user code. This lets
                 the same function pass the DELAYED duplicate check if it calls
                 after() again. Re-registration can still fail when the physical
                 queue is full because this tombstone remains counted until the
                 next DELAYED maintenance pass.
                 */
                kernel::delayed[index].fn = 0;
                fn();
            }

            for (uint8_t i = 1u; i < phase_count; ++i) {
                index = kernel::delayed_order[i];
                fn = kernel::delayed[index].fn;

                if (fn == 0) {
                    continue;
                }

                if ((int32_t)(now - kernel::delayed[index].deadline_ms) < 0) {
                    break;
                }

                kernel::delayed[index].fn = 0;
                fn();
            }

            kernel::state_mask |= (kernel::STATE_COMPACT_DELAYED |
                                   kernel::STATE_REBUILD_DELAYED);
        }
    }

#endif // TLS_DELAYED

