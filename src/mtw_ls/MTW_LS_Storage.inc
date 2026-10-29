/*
 Internal period storage type.
*/
#if TLS_PERIOD == 16
typedef uint16_t Period;
#else
typedef uint32_t Period;
#endif

/*
 One persistent scheduler control byte.

 Bits 0..3 are current-pass IDLE-skip causes. tick() clears them at entry while
 preserving bits 4..7. IDLE is entered when all four lower bits remain zero
 after the ISR, PERIODIC, and DELAYED phases.

 Bits 4..7 request PERIODIC or DELAYED reordering/compaction. They persist
 across tick() calls because every(), after(), removeEvery(), and removeAfter()
 may update queues in main context outside the corresponding maintenance phase.
*/
enum {
    STATE_SKIP_ISR          = 0x01u,
    STATE_SKIP_DELTA        = 0x02u,
    STATE_SKIP_PERIODIC     = 0x04u,
    STATE_SKIP_DELAYED      = 0x08u,

    STATE_REBUILD_PERIODIC  = 0x10u,
    STATE_COMPACT_PERIODIC  = 0x20u,
    STATE_REBUILD_DELAYED   = 0x40u,
    STATE_COMPACT_DELAYED   = 0x80u,

    STATE_IDLE_MASK         = 0x0Fu,
    STATE_MAINT_MASK        = 0xF0u
};

/*
 --------------------------------------------------------------------------
 Internal job records
 --------------------------------------------------------------------------
*/

#if TLS_PERIODIC == TLS_ENABLE

typedef struct {
    Fn       fn;
    Period   period_ms;
    uint32_t deadline_ms;
} PeriodicJob;

#endif // TLS_PERIODIC

#if TLS_DELAYED == TLS_ENABLE

typedef struct {
    Fn       fn;
    uint32_t deadline_ms;
} DelayedJob;

#endif // TLS_DELAYED

/*
 --------------------------------------------------------------------------
 Scheduler storage
 --------------------------------------------------------------------------
*/

/*
 Each ordinary queue is emitted only when its subsystem is enabled. Profile
 capacity macros remain defined and validated for configuration consistency,
 but a disabled queue has no array or count and therefore consumes no queue
 SRAM. PARTED, when enabled, adds poll_part[] alongside the enabled POLL queue.
*/

#if TLS_POSTED == TLS_ENABLE
static Fn posted[TLS_MAX_POSTED];
static uint8_t posted_count = 0u;
#endif

#if TLS_ISR == TLS_ENABLE
static volatile Fn isr_requests[TLS_MAX_ISR_REQUESTS];
static volatile uint8_t isr_request_count = 0u;
#endif

#if TLS_PERIODIC == TLS_ENABLE
static PeriodicJob periodic[TLS_MAX_PERIODIC];
static uint8_t periodic_order[TLS_MAX_PERIODIC];
static uint8_t periodic_count = 0u;
#endif

#if TLS_DELAYED == TLS_ENABLE
static DelayedJob delayed[TLS_MAX_DELAYED];
static uint8_t delayed_order[TLS_MAX_DELAYED];
static uint8_t delayed_count = 0u;
#endif

#if TLS_POLL == TLS_ENABLE
static Fn poll_jobs[TLS_MAX_POLL];
static uint8_t poll_count = 0u;
#endif

#if TLS_PARTED == TLS_ENABLE
/*
 poll_slot and poll_part are shared implementation symbols. External linkage
 allows PARTED macros expanded in TLS_DECL_ONLY .cpp translation units to use
 the single state instance owned by the implementation translation unit.
*/
uint8_t poll_slot = POLL_SLOT_NONE;
uint8_t poll_part[TLS_MAX_POLL];
#endif

/*
 Persistent state byte.

 Lower bits are cleared on every tick. Upper maintenance bits survive until
 the corresponding PERIODIC or DELAYED phase services them.
*/
static uint8_t state_mask = 0u;

/*
 Low 16 bits of the previous tick-entry timestamp. Unsigned subtraction from
 the next low-16 snapshot yields the modulo-65,536 ms interval used only by the
 IDLE load heuristic. Scheduler deadlines do not use this value.
*/
#if TLS_USE_IDLE == TLS_ENABLE
static uint16_t idle_prev_ms = 0u;
#endif

