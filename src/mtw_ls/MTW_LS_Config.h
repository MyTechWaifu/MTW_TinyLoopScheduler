#ifndef MTW_TINY_LOOP_SCHEDULER_CONFIG_H
#define MTW_TINY_LOOP_SCHEDULER_CONFIG_H

/*
 --------------------------------------------------------------------------
 Scheduler profile selection
 --------------------------------------------------------------------------
*/

/*
 Select one storage profile before including this header in the implementation
 owner:

   #define TLS_PROFILE TLS_TINY
   #define TLS_PROFILE TLS_MINI
   #define TLS_PROFILE TLS_MID
   #define TLS_PROFILE TLS_DEF
   #define TLS_PROFILE TLS_MAX
   #define TLS_PROFILE TLS_MANUAL

 If TLS_PROFILE is not defined, TLS_DEF is used.

 Profiles define physical queue capacities. The ordinary subsystem switches
 determine which of those queues are actually compiled. For example, TLS_MINI
 with TLS_DELAYED disabled retains MINI capacities for enabled queues while
 allocating no DELAYED queue.

 The following table is a recommended working load, not the physical capacity.
 Every predefined profile sets the corresponding physical capacity to one
 entry above the listed value. That extra entry is ordinary usable capacity;
 it is not reserved or protected. The recommendation leaves practical
 headroom for bursts and for PERIODIC/DELAYED tombstones that remain counted
 until their next maintenance pass.

 Recommended working load:

   Profile      PERIODIC  DELAYED  POLL  POSTED  ISR_REQUESTS
   TLS_TINY         1        1       1      1          1
   TLS_MINI         2        2       2      2          2
   TLS_MID          6        4       4      4          3
   TLS_DEF         12        8       8      8          6
   TLS_MAX         24       16      16     16         12

 Application code may occupy every physical slot. For more predictable
 behavior, size each queue at least one entry above the maximum simultaneous
 active or pending workload.

 TLS_MANUAL values are direct physical capacities; no additional headroom is
 added. Undefined manual values default to the TLS_DEF physical capacities.
 Capacity macros for disabled ordinary subsystems are still preprocessed and
 range-checked but do not allocate queue storage.
*/

#define TLS_TINY    1
#define TLS_MINI    2
#define TLS_MID     3
#define TLS_DEF     4
#define TLS_MAX     5
#define TLS_MANUAL  6

#ifndef TLS_PROFILE
#define TLS_PROFILE TLS_DEF
#endif

#if TLS_PROFILE != TLS_TINY && \
    TLS_PROFILE != TLS_MINI && \
    TLS_PROFILE != TLS_MID && \
    TLS_PROFILE != TLS_DEF && \
    TLS_PROFILE != TLS_MAX && \
    TLS_PROFILE != TLS_MANUAL
#warning "MTW TinyLoopScheduler: invalid TLS_PROFILE value; clamped to TLS_DEF"
#undef TLS_PROFILE
#define TLS_PROFILE TLS_DEF
#endif

/*
 --------------------------------------------------------------------------
 Profile-defined queue sizes
 --------------------------------------------------------------------------
*/

/*
 Fixed-capacity storage exists for:

   PERIODIC / DELAYED / POLL / POSTED / ISR_REQUESTS

 Each enabled queue uses a statically allocated array and uint8_t count. If a
 queue or buffer is full, the new request returns 0 and no existing entry is
 replaced or evicted. A disabled ordinary subsystem allocates no queue array or
 count. PARTED reuses POLL and, when enabled, adds only per-slot step state.
*/

#if TLS_PROFILE == TLS_MANUAL

/*
 Manual profile.

 Values define physical capacities directly and are later clamped to 2..255.
 Keep each enabled queue at least one entry above the expected maximum number
 of simultaneous active or pending jobs. Define only values that differ from
 the defaults below. Values for disabled ordinary subsystems do not allocate
 storage, although their macros are still validated by the preprocessor.
*/

#ifndef TLS_MAX_PERIODIC
#define TLS_MAX_PERIODIC      13
#endif

#ifndef TLS_MAX_DELAYED
#define TLS_MAX_DELAYED       9
#endif

#ifndef TLS_MAX_POLL
#define TLS_MAX_POLL          9
#endif

#ifndef TLS_MAX_POSTED
#define TLS_MAX_POSTED        9
#endif

#ifndef TLS_MAX_ISR_REQUESTS
#define TLS_MAX_ISR_REQUESTS  7
#endif

#elif TLS_PROFILE == TLS_TINY

#undef TLS_MAX_PERIODIC
#undef TLS_MAX_DELAYED
#undef TLS_MAX_POLL
#undef TLS_MAX_POSTED
#undef TLS_MAX_ISR_REQUESTS

#define TLS_MAX_PERIODIC      2
#define TLS_MAX_DELAYED       2
#define TLS_MAX_POLL          2
#define TLS_MAX_POSTED        2
#define TLS_MAX_ISR_REQUESTS  2

#elif TLS_PROFILE == TLS_MINI

#undef TLS_MAX_PERIODIC
#undef TLS_MAX_DELAYED
#undef TLS_MAX_POLL
#undef TLS_MAX_POSTED
#undef TLS_MAX_ISR_REQUESTS

#define TLS_MAX_PERIODIC      3
#define TLS_MAX_DELAYED       3
#define TLS_MAX_POLL          3
#define TLS_MAX_POSTED        3
#define TLS_MAX_ISR_REQUESTS  3

#elif TLS_PROFILE == TLS_MID

#undef TLS_MAX_PERIODIC
#undef TLS_MAX_DELAYED
#undef TLS_MAX_POLL
#undef TLS_MAX_POSTED
#undef TLS_MAX_ISR_REQUESTS

#define TLS_MAX_PERIODIC      7
#define TLS_MAX_DELAYED       5
#define TLS_MAX_POLL          5
#define TLS_MAX_POSTED        5
#define TLS_MAX_ISR_REQUESTS  4

#elif TLS_PROFILE == TLS_MAX

#undef TLS_MAX_PERIODIC
#undef TLS_MAX_DELAYED
#undef TLS_MAX_POLL
#undef TLS_MAX_POSTED
#undef TLS_MAX_ISR_REQUESTS

#define TLS_MAX_PERIODIC      25
#define TLS_MAX_DELAYED       17
#define TLS_MAX_POLL          17
#define TLS_MAX_POSTED        17
#define TLS_MAX_ISR_REQUESTS  13

#else

/*
 TLS_DEF physical capacities. The documented recommended working load is one
 entry lower in each queue.
*/

#undef TLS_MAX_PERIODIC
#undef TLS_MAX_DELAYED
#undef TLS_MAX_POLL
#undef TLS_MAX_POSTED
#undef TLS_MAX_ISR_REQUESTS

#define TLS_MAX_PERIODIC      13
#define TLS_MAX_DELAYED       9
#define TLS_MAX_POLL          9
#define TLS_MAX_POSTED        9
#define TLS_MAX_ISR_REQUESTS  7

#endif

/*
 --------------------------------------------------------------------------
 Approximate memory usage
 --------------------------------------------------------------------------
*/

/*
 Version 0.2.2 reference: ATmega328P, 16 MHz, Arduino AVR Boards 1.8.6, AVR GCC 7.3.0,
 -Os and LTO; init() in setup(), tick() in loop(), ordinary queues and default
 AVR IDLE enabled. Figures include the linked core and are rounded upward to
 multiples of 5 bytes. They are minimal-sketch planning references, not portable
 upper bounds. P32/P16 select TLS_PERIOD=32/16.

   Profile    P32 / PARTED off    P16 / PARTED off
              SRAM    Flash       SRAM    Flash
   TLS_TINY  ~ 65 B   ~1825 B     ~ 60 B   ~1850 B
   TLS_MINI  ~ 85 B   ~1825 B     ~ 80 B   ~1850 B
   TLS_MID   ~145 B   ~1825 B     ~130 B   ~1850 B
   TLS_DEF   ~245 B   ~1825 B     ~220 B   ~1850 B
   TLS_MAX   ~445 B   ~1825 B     ~395 B   ~1850 B

   Profile    P32 / PARTED on     P16 / PARTED on
              SRAM    Flash       SRAM    Flash
   TLS_TINY  ~ 65 B   ~1885 B     ~ 60 B   ~1885 B
   TLS_MINI  ~ 85 B   ~1885 B     ~ 80 B   ~1885 B
   TLS_MID   ~145 B   ~1885 B     ~130 B   ~1885 B
   TLS_DEF   ~245 B   ~1885 B     ~220 B   ~1885 B
   TLS_MAX   ~445 B   ~1885 B     ~395 B   ~1885 B

 The DELAYED prefix cache contributes one fixed SRAM byte for the whole queue.
 It has no setting and is included above. Disabling DELAYED removes its cache.
 PARTED-on builds above register no PARTED function; LTO can remove unused state.
 Actual PARTED applications must allow for the per-slot state array and active
 slot. Application data, Serial buffers and stack require separate budgets.

 Approximate SRAM planning cost per physical queue slot:

   PERIODIC, TLS_PERIOD=32       ~15 B
   PERIODIC, TLS_PERIOD=16       ~10 B
   DELAYED                      ~10 B
   POLL, PARTED disabled         ~5 B
   POLL, PARTED enabled          ~5 B
   POSTED                        ~5 B
   ISR request                   ~5 B

 Slot estimates are rounded planning values, not structure sizes. Timing queues
 include their order-table entry. Fixed queue counters, cached time, state and
 the DELAYED prefix byte are separate. PARTED adds state per physical POLL slot.
 Compiler, core, configuration and application reachability affect the result;
 use the real sketch build report for its actual SRAM and Flash usage.
*/

/*
 --------------------------------------------------------------------------
 Approximate CPU cost
 --------------------------------------------------------------------------
*/

/*
 Version 0.2.2 reference: simavr ATmega328P, 16 MHz, Arduino AVR Boards 1.8.6,
 AVR GCC 7.3.0, -Os and LTO, all ordinary queues plus PARTED, TLS_PERIOD=32,
 manual capacities 25. IDLE and background/timer activity are suppressed; the
 real Arduino millis() reads a stationary timer. Marker overhead is calibrated
 and excluded. Estimates are rounded upward to multiples of 5 CPU cycles;
 they are not worst-case guarantees or measurements on a physical board.

 Basic cost:

   Empty tick                         ~145 cycles
   Waiting tick, 4 PERIODIC+4 DELAYED  ~215 cycles
   Minimal direct callback             ~10 cycles

 Approximate tick growth per active/queued minimal callback (1..8 jobs):

   Active POLL callback                       ~50 cycles/job
   Queued POSTED callback                     ~30 cycles/job
   Deferred ISR callback (foreground)         ~35..40 cycles/job
   Active PARTED step, holding its state      ~60 cycles/job

 PARTED reference steps hold their state and perform one NOP. Per-job values
 include minimal callback dispatch. Budget actual application work separately.
 Deferred ISR means foreground execution, not the hardware ISR or UART handler.

 One mixed pass with PERIODIC, DELAYED, POSTED, ordinary POLL, deferred ISR and
 PARTED is about ~505 cycles, or ~35 us at 16 MHz.
 A DELAYED phase appending after 8 sorted records is about
 ~215 cycles; inserting first is about ~565 cycles.
 These are phase totals. Very short DELAYED queues can be slower with the cache.

 Waiting timing queues inspect the first deadline in constant time. Appending
 to a usable sorted prefix skips its old entries. Compaction or invalidation
 uses full insertion sort: O(N) for ordered input, O(N^2) in the reverse case.
*/

/*
 --------------------------------------------------------------------------
 Period storage width
 --------------------------------------------------------------------------
*/

/*
 Internal period and delay input width.

 TLS_PERIOD = 16:
   Use this only when every PERIODIC interval and DELAYED timeout is within
   0..65,535 ms, approximately 65.535 seconds. PERIODIC records store their
   repeat period as uint16_t, which saves two bytes per PERIODIC record.
   DELAYED records store only a 32-bit absolute deadline, but after() still
   narrows its input through the same internal Period type. Deadline arithmetic
   remains 32-bit in both modes.

 TLS_PERIOD = 32:
   Use this for supported intervals longer than 65,535 ms.

 Public every() and after() parameters remain uint32_t in both modes, giving
 TLS_DECL_ONLY translation units stable declarations. The implementation does
 not clamp or reject values that exceed the selected storage width. In
 TLS_PERIOD=16 mode, C++ uint16_t narrowing applies modulo 65,536; for example,
 65,536 becomes 0 and 65,537 becomes 1. Callers must enforce the documented
 range before registration.

 Absolute deadlines use uint32_t because Arduino millis() is 32-bit. Deadline
 tests use signed-delta arithmetic:

       (int32_t)(now - deadline_ms) >= 0

 This comparison is rollover-safe only when the effective stored period or
 delay does not exceed INT32_MAX milliseconds. Therefore the supported maximum
 is 65,535 ms in TLS_PERIOD=16 mode and 2,147,483,647 ms (about 24.855 days) in
 TLS_PERIOD=32 mode. Larger TLS_PERIOD=32 values are accepted by the API but
 have unsupported deadline semantics and may appear due at the wrong time.

 Longer calendar-scale waits should be implemented with a shorter supported
 scheduler interval plus an application counter. No runtime range validation
 or saturation is performed.
*/

#ifndef TLS_PERIOD
#define TLS_PERIOD 32
#endif

#if TLS_PERIOD != 16 && TLS_PERIOD != 32
#warning "MTW TinyLoopScheduler: invalid TLS_PERIOD value; clamped to 32"
#undef TLS_PERIOD
#define TLS_PERIOD 32
#endif

/*
 --------------------------------------------------------------------------
 Dynamic IDLE selection
 --------------------------------------------------------------------------
*/

/*
 Dynamic IDLE threshold selection.

 TLS_IDLE selects a load-gate threshold; it does not enable or disable the
 IDLE backend. At the beginning of tick(), the scheduler measures the elapsed
 interval between the current and previous tick() entry using the low 16 bits
 of millis(). If that interval is greater than the selected threshold, IDLE is
 skipped for the current pass.

 Public selector:

   #define TLS_IDLE TLS_AGGRESSIVE  //  3 ms threshold
   #define TLS_IDLE TLS_NORMAL      //  8 ms threshold
   #define TLS_IDLE TLS_LAZY        // 16 ms threshold

 TLS_NORMAL is the default. The comparison is strictly greater-than, so an
 interval equal to the threshold does not set the load skip bit.

 IDLE is entered when no deferred ISR callback was executed, no PERIODIC
 callback was due, no DELAYED callback was due, and the tick-entry interval
 did not exceed the threshold. POSTED entries and persistent POLL
 callbacks intentionally do not suppress IDLE; they run after an interrupt
 wakes the MCU.

 The 16-bit interval is a lightweight heuristic and wraps modulo 65,536 ms.
 This wrap does not affect scheduler deadlines, which remain 32-bit, but the
 load gate is not meaningful for gaps of 65.536 seconds or longer.
 
 TLS_SLEEP_GUARD controls how the fixed SLEEP_MODE_IDLE selection is
 maintained. init() selects SLEEP_MODE_IDLE in both configurations.

   #define TLS_SLEEP_GUARD TLS_ENABLE

     Safe default. Before every actual sleep entry, tick() selects
     SLEEP_MODE_IDLE again. This protects the scheduler if application code or
     another library changed the MCU sleep-mode selection after init().

   #define TLS_SLEEP_GUARD TLS_DISABLE

     Optimized policy. SLEEP_MODE_IDLE is selected only by init(); tick() does
     not repeat the mode-selection write before sleeping. After init(), the
     application and third-party libraries must not change the MCU sleep mode.
     sleep_enable(), sleep_cpu(), and sleep_disable() still run for every
     actual sleep entry in both configurations. 
*/

#define TLS_AGGRESSIVE 1
#define TLS_NORMAL     2
#define TLS_LAZY       3

#ifndef TLS_IDLE
#define TLS_IDLE TLS_NORMAL
#endif

#if TLS_IDLE != TLS_AGGRESSIVE && \
    TLS_IDLE != TLS_NORMAL && \
    TLS_IDLE != TLS_LAZY
#warning "MTW TinyLoopScheduler: invalid TLS_IDLE value; clamped to TLS_NORMAL"
#undef TLS_IDLE
#define TLS_IDLE TLS_NORMAL
#endif

#undef TLS_IDLE_THRESHOLD_MS

#if TLS_IDLE == TLS_AGGRESSIVE
#define TLS_IDLE_THRESHOLD_MS 3
#elif TLS_IDLE == TLS_NORMAL
#define TLS_IDLE_THRESHOLD_MS 8
#else
#define TLS_IDLE_THRESHOLD_MS 16
#endif

/*
 --------------------------------------------------------------------------
 Compile-time range clamps
 --------------------------------------------------------------------------
*/

/*
 Every physical queue capacity is clamped independently to 2..255, including
 macros belonging to disabled ordinary subsystems. Disabled queues still do
 not allocate storage.

 Keep each enabled physical queue at least one entry above the expected maximum
 number of simultaneously active or pending jobs. Predefined profiles already
 follow that recommendation. uint8_t counts support capacities up to 255 while
 keeping SRAM and AVR instruction cost low.
*/

#if TLS_MAX_PERIODIC < 2
#warning "MTW TinyLoopScheduler: TLS_MAX_PERIODIC was below 2; clamped to 2"
#undef TLS_MAX_PERIODIC
#define TLS_MAX_PERIODIC 2
#endif

#if TLS_MAX_PERIODIC > 255
#warning "MTW TinyLoopScheduler: TLS_MAX_PERIODIC was above 255; clamped to 255"
#undef TLS_MAX_PERIODIC
#define TLS_MAX_PERIODIC 255
#endif

#if TLS_MAX_DELAYED < 2
#warning "MTW TinyLoopScheduler: TLS_MAX_DELAYED was below 2; clamped to 2"
#undef TLS_MAX_DELAYED
#define TLS_MAX_DELAYED 2
#endif

#if TLS_MAX_DELAYED > 255
#warning "MTW TinyLoopScheduler: TLS_MAX_DELAYED was above 255; clamped to 255"
#undef TLS_MAX_DELAYED
#define TLS_MAX_DELAYED 255
#endif

#if TLS_MAX_POLL < 2
#warning "MTW TinyLoopScheduler: TLS_MAX_POLL was below 2; clamped to 2"
#undef TLS_MAX_POLL
#define TLS_MAX_POLL 2
#endif

#if TLS_MAX_POLL > 255
#warning "MTW TinyLoopScheduler: TLS_MAX_POLL was above 255; clamped to 255"
#undef TLS_MAX_POLL
#define TLS_MAX_POLL 255
#endif

#if TLS_MAX_POSTED < 2
#warning "MTW TinyLoopScheduler: TLS_MAX_POSTED was below 2; clamped to 2"
#undef TLS_MAX_POSTED
#define TLS_MAX_POSTED 2
#endif

#if TLS_MAX_POSTED > 255
#warning "MTW TinyLoopScheduler: TLS_MAX_POSTED was above 255; clamped to 255"
#undef TLS_MAX_POSTED
#define TLS_MAX_POSTED 255
#endif

#if TLS_MAX_ISR_REQUESTS < 2
#warning "MTW TinyLoopScheduler: TLS_MAX_ISR_REQUESTS was below 2; clamped to 2"
#undef TLS_MAX_ISR_REQUESTS
#define TLS_MAX_ISR_REQUESTS 2
#endif

#if TLS_MAX_ISR_REQUESTS > 255
#warning "MTW TinyLoopScheduler: TLS_MAX_ISR_REQUESTS was above 255; clamped to 255"
#undef TLS_MAX_ISR_REQUESTS
#define TLS_MAX_ISR_REQUESTS 255
#endif

#endif // MTW_TINY_LOOP_SCHEDULER_CONFIG_H
