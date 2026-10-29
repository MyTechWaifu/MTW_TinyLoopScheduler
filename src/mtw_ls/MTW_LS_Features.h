/*
 --------------------------------------------------------------------------
 Version
 --------------------------------------------------------------------------
*/

#define TLS_VERSION_MAJOR 0
#define TLS_VERSION_MINOR 2
#define TLS_VERSION_PATCH 0
#define TLS_VERSION_STRING "0.2.0"

/*
 --------------------------------------------------------------------------
 Compile-time subsystem selection
 --------------------------------------------------------------------------
*/

#define TLS_ENABLE  1
#define TLS_DISABLE 0

/*
 PERIODIC, DELAYED, POSTED, POLL, and ISR are ordinary queue subsystems and
 are enabled by default. Define a switch before the first inclusion of this
 header to remove the corresponding subsystem completely:

   #define TLS_PERIODIC TLS_DISABLE
   #define TLS_DELAYED  TLS_DISABLE
   #define TLS_POSTED   TLS_DISABLE
   #define TLS_POLL     TLS_DISABLE
   #define TLS_ISR      TLS_DISABLE

 A disabled ordinary subsystem contributes no public API declaration, API
 implementation, queue storage, queue counter, initialization code, or
 tick() phase.

 PARTED is a separate optional macro layer over POLL. It is disabled by
 default, has no independent queue, and is configured below with TLS_PARTED.

 Queue profiles and TLS_MANUAL still define numeric capacities for all
 ordinary queues. These capacity macros consume no SRAM by themselves; a
 capacity belonging to a disabled subsystem is simply not used for storage.
*/

#ifndef TLS_PERIODIC
#define TLS_PERIODIC TLS_ENABLE
#endif

#ifndef TLS_DELAYED
#define TLS_DELAYED TLS_ENABLE
#endif

#ifndef TLS_POSTED
#define TLS_POSTED TLS_ENABLE
#endif

#ifndef TLS_POLL
#define TLS_POLL TLS_ENABLE
#endif

#ifndef TLS_ISR
#if MTW_LS_HAS_ISR
#define TLS_ISR TLS_ENABLE
#else
#define TLS_ISR TLS_DISABLE
#endif
#endif

// Full sleep subsystem switch; TLS_IDLE still selects its load threshold.
#ifndef TLS_USE_IDLE
#define TLS_USE_IDLE MTW_LS_HAS_IDLE
#endif

#ifndef TLS_PARTED
#define TLS_PARTED TLS_DISABLE
#endif

#ifndef TLS_SLEEP_GUARD
#define TLS_SLEEP_GUARD TLS_ENABLE
#endif

#if TLS_PERIODIC != TLS_ENABLE && TLS_PERIODIC != TLS_DISABLE
#warning "MTW TinyLoopScheduler: invalid TLS_PERIODIC value; clamped to TLS_ENABLE"
#undef TLS_PERIODIC
#define TLS_PERIODIC TLS_ENABLE
#endif

#if TLS_DELAYED != TLS_ENABLE && TLS_DELAYED != TLS_DISABLE
#warning "MTW TinyLoopScheduler: invalid TLS_DELAYED value; clamped to TLS_ENABLE"
#undef TLS_DELAYED
#define TLS_DELAYED TLS_ENABLE
#endif

#if TLS_POSTED != TLS_ENABLE && TLS_POSTED != TLS_DISABLE
#warning "MTW TinyLoopScheduler: invalid TLS_POSTED value; clamped to TLS_ENABLE"
#undef TLS_POSTED
#define TLS_POSTED TLS_ENABLE
#endif

#if TLS_POLL != TLS_ENABLE && TLS_POLL != TLS_DISABLE
#warning "MTW TinyLoopScheduler: invalid TLS_POLL value; clamped to TLS_ENABLE"
#undef TLS_POLL
#define TLS_POLL TLS_ENABLE
#endif

#if TLS_ISR != TLS_ENABLE && TLS_ISR != TLS_DISABLE
#if MTW_LS_HAS_ISR
#warning "MTW TinyLoopScheduler: invalid TLS_ISR value; clamped to TLS_ENABLE"
#else
#warning "MTW TinyLoopScheduler: invalid TLS_ISR value; clamped to TLS_DISABLE"
#endif
#undef TLS_ISR
#if MTW_LS_HAS_ISR
#define TLS_ISR TLS_ENABLE
#else
#define TLS_ISR TLS_DISABLE
#endif
#endif

#if TLS_USE_IDLE != TLS_ENABLE && TLS_USE_IDLE != TLS_DISABLE
#warning "MTW TinyLoopScheduler: invalid TLS_USE_IDLE value; using platform default"
#undef TLS_USE_IDLE
#define TLS_USE_IDLE MTW_LS_HAS_IDLE
#endif

#if TLS_USE_IDLE == TLS_ENABLE && !MTW_LS_HAS_IDLE
#error "MTW TinyLoopScheduler: IDLE is not implemented for this platform; use TLS_USE_IDLE TLS_DISABLE"
#endif

#if TLS_ISR == TLS_ENABLE && (!MTW_LS_HAS_ISR || !MTW_LS_HAS_CRITICAL)
#error "MTW TinyLoopScheduler: postISR is not implemented for this platform; use TLS_ISR TLS_DISABLE"
#endif

#if TLS_PARTED != TLS_ENABLE && TLS_PARTED != TLS_DISABLE
#warning "MTW TinyLoopScheduler: invalid TLS_PARTED value; clamped to TLS_DISABLE"
#undef TLS_PARTED
#define TLS_PARTED TLS_DISABLE
#endif

#if TLS_SLEEP_GUARD != TLS_ENABLE && TLS_SLEEP_GUARD != TLS_DISABLE
#warning "MTW TinyLoopScheduler: invalid TLS_SLEEP_GUARD value; clamped to TLS_ENABLE"
#undef TLS_SLEEP_GUARD
#define TLS_SLEEP_GUARD TLS_ENABLE
#endif

/*
 PARTED executes only through the POLL phase. When enabled, it adds one step
 state byte for every physical POLL slot plus one shared active-slot byte. It
 has no independent job queue and therefore cannot be enabled without POLL.
*/
#if TLS_PARTED == TLS_ENABLE && TLS_POLL == TLS_DISABLE
#error "MTW TinyLoopScheduler: TLS_PARTED requires TLS_POLL support"
#endif

#if TLS_PARTED == TLS_ENABLE
#pragma message("MTW TinyLoopScheduler: PARTED support enabled")
#endif

/*
 PARTED is disabled by default. In that configuration no poll_part[] array or
 shared poll_slot byte is allocated.

 Enable it before including this header in projects that define PARTED
 callbacks:

   #define TLS_PARTED TLS_ENABLE
   #include <MTW_TinyLoopScheduler.h>

 In a multi-file build, the implementation-owning translation unit and every
 TLS_DECL_ONLY translation unit that defines a PARTED callback must enable
 TLS_PARTED. All translation units must also use matching ordinary subsystem
 switches; otherwise declarations may not match the symbols emitted by the
 implementation owner.
*/

/*
 PARTED state values:

   0..247   user-defined execution steps;
   248..254 reserved for future PARTED control states;
   255      start/uninitialized state.

 Reserved values must not be selected by TLS_STEP() or by a constant
 TLS_NEXT(N).
*/
#define TLS_PART_MAX_STEP    247u
#define TLS_PART_STATE_START 255u

