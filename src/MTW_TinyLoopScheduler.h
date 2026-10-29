// SPDX-License-Identifier: Apache-2.0
/*
 MTW TinyLoopScheduler

 Copyright 2026 MyTechWaifu

 Licensed under the Apache License, Version 2.0.

 Purpose:
   MTW TinyLoopScheduler is a small cooperative scheduler for AVR targets
   using an Arduino-compatible core. It replaces scattered millis() checks
   with named callbacks dispatched from one explicit place: tinyls::tick().

 Public API:
   - tinyls::every(fn, period_ms):
       register a repeating callback;
   - tinyls::after(fn, delay_ms):
       register a one-shot delayed callback;
   - tinyls::post(fn):
       queue a one-shot callback for the POSTED phase;
   - tinyls::poll(fn):
       register a persistent callback for the POLL phase;
   - tinyls::postISR(fn):
       defer a callback from a normal hardware ISR to main context;
   - tinyls::removeEvery(fn), tinyls::removeAfter(fn), and
     tinyls::removePoll(fn):
       remove the corresponding registered callback.

 Compile-time features:
   - TLS_PERIODIC controls every() and removeEvery();
   - TLS_DELAYED controls after() and removeAfter();
   - TLS_POSTED controls post();
   - TLS_POLL controls poll() and removePoll();
   - TLS_ISR controls postISR();
   - TLS_PARTED controls the PARTED macro API and its per-POLL-slot step
     state; PARTED requires TLS_POLL.
   - TLS_SLEEP_GUARD controls whether SLEEP_MODE_IDLE is reaffirmed before
     every actual sleep entry.

 PERIODIC, DELAYED, POSTED, POLL, and ISR are enabled by default. Disabling
 one of these ordinary subsystems removes its public declarations, runtime
 implementation, queue storage, counters, initialization code, and tick()
 phase. PARTED is disabled by default. Disabled PARTED keeps only diagnostic
 macro stubs so accidental PARTED source code fails at compile time; it adds
 no PARTED runtime storage or execution logic.

 Scheduler pass order:
   1. capture one common millis() snapshot and update the IDLE load gate;
   2. drain deferred ISR callbacks when TLS_ISR is enabled;
   3. compact/reorder PERIODIC storage as required and execute due callbacks;
   4. compact/reorder DELAYED storage as required and execute due callbacks;
   5. enter AVR SLEEP_MODE_IDLE when the IDLE gate permits it;
   6. fully drain one-shot POSTED callbacks when enabled;
   7. execute a bounded POLL pass when enabled.

 Runtime model:
   - cooperative execution only;
   - no preemption and no threads;
   - callbacks dispatched by the scheduler execute synchronously inside
     tinyls::tick() in main context;
   - tick() should be called frequently from loop();
   - callbacks must return quickly;
   - long work should be divided into short PARTED steps;
   - hardware ISRs may call only postISR(), when ISR support is enabled;
   - postISR() records a deferred request; the callback itself runs later
     from tick() in main context.

 API contexts:
   - postISR() is restricted to a normal AVR hardware ISR in which global
     interrupts remain disabled;
   - enabled non-ISR queue APIs are main-context APIs;
   - a callback deferred through postISR() already runs in main context and
     may call any enabled main-context scheduler API;
   - tick() is not reentrant and must not be called from a callback, PARTED
     step, ISR, or other nested context.

 Return values:
   - queue-management functions return uint8_t;
   - 1 means that the requested operation was performed;
   - 0 means that it was rejected, was not found, or could not be performed;
   - applications may ignore the returned value when failure is acceptable.

 Design goals:
   - static storage only;
   - no heap allocation;
   - no dynamic containers;
   - low SRAM usage;
   - predictable memory layout;
   - predictable execution path;
   - simple low-level C/C++ style code;
   - AVR support through Arduino-compatible cores.

 Multi-file builds:
   - exactly one translation unit must include this header without
     TLS_DECL_ONLY; that translation unit owns the implementation and all
     scheduler state;
   - Arduino normally combines all .ino tabs of one sketch into one generated
     C++ translation unit, which is the intended implementation owner;
   - the compile-time switches must be defined before the first inclusion of
     this header in that generated sketch translation unit;
   - every additional real .cpp translation unit that includes this header
     must define TLS_DECL_ONLY before the include;
   - all translation units must use matching TLS_PERIODIC, TLS_DELAYED,
     TLS_POSTED, TLS_POLL, TLS_ISR, TLS_PARTED, and TLS_SLEEP_GUARD values so
     declarations and available symbols remain consistent;
   - every translation unit that defines a PARTED function must enable
     TLS_PARTED, and the implementation-owning translation unit must also
     enable it so the shared PARTED state is allocated.

 Non-goals:
   - not a preemptive RTOS;
   - not a thread scheduler;
   - not a dynamic task manager;
   - not a hard real-time kernel.
*/
#ifndef MTW_TINY_LOOP_SCHEDULER_DECLARATIONS_H
#define MTW_TINY_LOOP_SCHEDULER_DECLARATIONS_H

#include <Arduino.h>

#include "mtw_ls/MTW_LS_Platform.h"

#include "mtw_ls/MTW_LS_Features.h"
#include "mtw_ls/MTW_LS_Public.h"
#include "mtw_ls/MTW_LS_Parted.h"

#endif // MTW_TINY_LOOP_SCHEDULER_DECLARATIONS_H

#if !defined(TLS_DECL_ONLY) && \
    !defined(MTW_TINY_LOOP_SCHEDULER_IMPLEMENTATION_H)
#define MTW_TINY_LOOP_SCHEDULER_IMPLEMENTATION_H

#include "mtw_ls/MTW_LS_Config.h"

/*
 --------------------------------------------------------------------------
 Scheduler implementation
 --------------------------------------------------------------------------
*/

namespace tinyls {

/*
 Public cached time definition.
*/
uint32_t now = 0u;

namespace kernel {

#include "mtw_ls/MTW_LS_Storage.inc"
#include "mtw_ls/MTW_LS_Critical.inc"
} // namespace kernel

/*
 --------------------------------------------------------------------------
 Public scheduler implementation
 --------------------------------------------------------------------------
*/

void init(void)
{
#include "mtw_ls/MTW_LS_Init.inc"
}

void tick(void)
{
#include "mtw_ls/MTW_LS_TickBegin.inc"
#include "mtw_ls/MTW_LS_TickISR.inc"
#if TLS_PERIODIC == TLS_ENABLE || \
    TLS_DELAYED == TLS_ENABLE || \
    TLS_POLL == TLS_ENABLE
    uint8_t phase_count;
#endif

#include "mtw_ls/MTW_LS_TickPeriodic.inc"
#include "mtw_ls/MTW_LS_TickDelayed.inc"
#include "mtw_ls/MTW_LS_TickIdle.inc"
#include "mtw_ls/MTW_LS_TickPosted.inc"
#include "mtw_ls/MTW_LS_TickPoll.inc"
}

#include "mtw_ls/MTW_LS_API.inc"
} // namespace tinyls

#endif // !TLS_DECL_ONLY && !MTW_TINY_LOOP_SCHEDULER_IMPLEMENTATION_H


/*
                                                   ...                    .;UUn;
                                               :!zzzUUzz: ,:;;;:,      .:!zUz:!z
                                            ,znUUUzzzUn!zUzUnfffnUz:,;zzUnfnz:;U
                                          ,nBUUfBDDDDMMB;!nfMBnUz!;zUUnfBDUzMB;U
                          !;.            zBU!fMMBnzzUMDfUz!fMBfU. ;UBMMDB: BMM!U
                         ,fzU!:     .,.,nB;zDMBBfffnz:UB:;nBBBU.;fMDMMMBz:;zMM!U
                         ,n;!nnUz::zzzzzU:;BUnBMDMMMDM;!.!fBMfU;;,UBMMfnMM;:MM!U!,
                          n:!nfMDMBfnUz!!,;.!fBfnU!:;UM;;MfnfBDDBU,:fn,fMMn;fB!z!fU:
                          zzfMnUBDMMDDDBU;.UMMfBfUfMfnUfBzUBBfBMMDMU:.:fMMf:ffz; .!nn:
                        .!zznDM:.UMMMMMn.:MDMBDUzMM;.;zfn!zUBDMMMMMDB; UMMn:Bz!U    ;fn,
                       !fU.U!MMU;nnnBB! zDMMMDzUDB,!f;;ffUMBUUMMMMMBMDU:BMU!M!znz .   !fz
                     ;fU,  z!fDf!fMz,z.UDMMMDzUDM!fDzfDBBMMMDfUBMMMMBfDn;f;zM:ffz!     .Uf:
                   .Uf:    .n!Dn!BMB: UDBMMDf!DMnnDBnDBMDznDMDMfMMMMMBUBB, fB.,DUn,      :fU
                  ;Bz       !zUM;nDD:zDBMMMM!MMMnMMBBM,nD,,MMMMMMMBMMMMzUf,nz;;UDzU        nf.
                 Uf:         U!fU;fU!DfBMMDnnDMBBMMMMB .f, nDMMMMMMfMMMDn!!...BzMfU; . .zz. !B:
               .fn         . ;n;M;.:DffMMMMUMMMMMMMBMf  ,. ;MMBMMBfMfMMMDMn;  zBBMz:;; !UU!  :B!
              ,Bz   ;;    zz  UzUn.MBnMMMMBfMBMMMMMfMf .,;;.nBBfDMnBMnBMBfMDMnznnfU!:  .!UU   .Bz
             :B; . ;Uzz  Uz   .n;,BBzMMBMMfBMBMMMMMnBf,nBMD;!UMnnDMUBMfBMBU!UBBfnUU!:     !n.   fz
            :B: .  !nz: :n   . ;UnM:BDfBMMfMBfMMMBBnff!DMMMB:U;M!UDB!MDBBMMf;    nDnU  .   ;f:   fz
           :B: .  !n,   ,n    :nfM,nDBnMMMfDfnBMMMBUnBzMMMMDn!!;B:zDf:fDMMMMDBU;:!MfU,!,    .n;  .B!
          ,B: .  UU     U!   !BnM:;DMfBMMBnDnUfBMMBn!MUMMMMMMn!z:U:;DB.!BDDMBBBfnzzz!n!   !U;.n!  ,B:
         .B; . .n!  . ,n!  .nB!M;.BMMnMMMfnDz!nUDMBf,fzMMMMMDf;:n;U:,fM, !nBMMMBffnBzn   .n!n  n,  :B.
         f! . :n:    ;n:  :Bn,B! zDBBnMMDnUD!,f:DMBB,zzBMMDB!!MBnBnU;.!fz  .,;!!!;UD;;z . ,!U: U: . zn
        Un    U; .  zn.  ;B!.BU.:BBBBfMMMnzD! n,UDMM;,;fDDU:nDDMn!!:;,: !Uz, .:!U,UDU:n: .  ;z U,    f!
       ;B     U:  .Uz   :B: Bf,nzDnBfBMMBn;D;!U!.nMMzz;;Dz;BBfz,  :.!zU  ;:z!:;:.;!Df!fn: . ;z n:    ,B.
       B:     U: ,n;    f; UD:nznDzBfBBMff.M!Uf;U:UDU!M,UBBfz.:  ,fzMU:,;:UU;  , nzMM:Ufn:  ;z :n:::  zn
      Un  :zz U: U;    ;U .Bz!!!BM:MnfBMnn.!z!z::! !M!fM!BBz,zU!!nzzn,f;n,fM;  !.!BfDz:nnn! .n! .fzU:  B:
     ,B.  Uzn,U: U:    !: ;n;;:UMB.MnUBfB!::UfDDn!n;:n!MMBMUff;!!!Un.;B;B!nf...!!.MBMf!fUnnU. Uz :nU,  !n
     UU    n! U: U:    :: z!!.;UBU ff:MzM;,zz:!!zUUBfzzUMMUzUnBfBMDD!nU;D!B! ,z,f nMMD!U:zUzU: zU z;    f:
    ,B.    n, U: U:     , !!; :Uf; zM ffUn ,.:;.;  ;nBMUz:nMMDMMMMMM;n.zDzz   f,n,:DMMfz; :zzn: n z;    zn
    zU .  UU  n: U:       :U,..Uf,:.M;.M!U!.,!Df!!zzn::zz,nDMMMMDDMU;;,fD;:...fn,f.UDMMzU.  z!U,n ;n    .B.
    f:  .n!  UU  n: ,,     U!  ;f:U zf :B;;,;nzffUfBDU!DDM:nMMMBnUzUn!zBD:,U.!zD;!B,BMMfzz   Uz!!U ;n, . n!
   :f  :n: .nz .n! :UU;    .n;  U;U, n! :f;. UDBBMMMDUUMMMMzzzzUnfMDf:fBM: z:UzMM:nB!MMDUn;  !Uz !n.,n;  !n
   UU .f, :n: :n:  :UU;      ;; :n;.; z: .zz: UDDMMBn!BfMMMDMMMDDMMMz,BBM: z:UnfDB,BMnMMMnf: !nz  :n:.n! ,f.
   f; .n  n: ;n. .  f;  :.      UD;zU  :,  ,;:,znUzUnMMBMMMMMBMMMMMM,!MBM!;U ;MfMDU fDBMMMnn;nn, . .z..n  f;
  ,B. .n. U: U; . ,n;  :f      !Mf;D! .    , .; UMMMDMMMMMMBBBMMMMM; UBMM,;,  BMMMD! UMMMMMBU!,     ;..n  Uz
  ;f  .n. U: z;  ;n: . Un    .!BBzMM.,,  .!! !; ;UMDMMMMBBMBMMMMDB;n.ffMB,  , ;DfMMD! ;fMMBMB!...  .zz.n  !n
  zn  .n. U: z; !n.    !MUzzzfMBBMDz n. .!M, U, z ,nMDDMMMBfMMMDB.nB;ffMB:  .; nBnDMDz UBMMfBDMU!!!zfU n  :f
  Uz  .n. U: z; n.      zfffBnUBMDf.Bz  !MM, z!.U   .!nMDDMMMMDn zDfnUBMBz . z: fnUDMDU,BMMMnznMMMBnn,:f, ,f
  n!  znz n: z; n,       ,:;,!BMDf;Bf  :UBM;; zU,      .!nBDDM! !DMfn:MMfn   .n. UzzMMD!.BDBDBU;:;!z: ;n; .f.
  f;  :z.;n. U; n,      :;znBMDMnzMf . z:fDnz,          :,.!z, !DMBn!:DMBnz . :n, !;:fDM:,BBnDDMn;..!n:   .f.
  f:    zn. !n. n,    :znBBBBDf;nDn.:. !;!DMnf;    ..   .U    zDMMff,.MMMfn! ,.,z, .. ;fU ,Mn!MMMMMU .Uz  .f.
  f:  .Uz  UU  ;n.   !nBMUzBDU,BB: :.   !;nDDnU!  .;.  . nz .nDMMDnn; UDMDzU; ,  ,,. ...:  :D:zDMBUMD! U! .f.
  f;  ;U .nz  zU    znBf:;MDz.f! .!Unnnz.::zDM:n: :.     UMUBDMMMMB!n; BMMB:n!!:UUznfBMBfn! zB fMMf.nDU n.,f
  n;  ;z n! .nz    ;nBU :DDU z, UMDDDDDfnn!.BD!;;;!;!zUzfBMDMMMDMMBU!U!;MMM;znUffzBDMMMMMDDB!; ;BnDU UD!z:,f
  n!  ;U n,.n;     UnU, fDB ..,BDMMMMMUnBUU!MD,;zUMDDDBfDMMMMMBnnnfBfUz,MMM,z!fDzBDMMMMMMMMMDz .nzMB. nMU,;n
  zU  ;z n,,n     .nU!,.MBz  ,MMMMMDBUUfUnfMM;.nDMBBMMMfBMMBfBBBBMMMDD!UMDU;nzDfUDMMMMMMMMMMMDz U:MM, ,MU.zU
  !n  ;U n,,n      n!z .Mn: .BMMMMDfUnnMMUzU!zBMMMMMMfnUznBBMMMMMMMMBnfDMU;fUfDUBMMMMMMMMMMMMMM,:,Mf,, Bn.n;
  :f .Un,U,,n   .  !z;: nB. UDMMMDfUBzzD:;fBMDMMMMMMn!..,;nMMMMMMMBnnBBM!zBMzMMUMMMMMMMMMMMMMMDU :D!!..B; f,
  .B, zz.U,,n ,n:   zUU,.B!.MMMDMnzn!!zn:fMMMMMMMMMf!:  :;nMMMMMMMUUnBzznDDnUDfnDMMMMMMMMMMMMMMB zUz! zf.:f
   n!    U,,n Uz     ,n: ,!zDMMfzzz:zMMfnUfMMMMMMMBz;. ,;zMMMMMMDnf;nDU,nMMzfDUfMMMMMMMMMMMMMMMM:.;B.zf: Uz
   zn . .f.,U.n;.    :z.. .BBnnUUzUfMMMMMDMMMMMMMMMz;;;;!fMMMMMMMfznzMDfUn;!BMzBMMMMMMMMMMMMMMMD; fB:;   f:z.
   ,B. :n; :n.nfz!;;!!;!  .UUfnUnBDDMMMMMMMMMMMMMMMBfBnnBBBBMMMMMMfUzzfMDMfUnfzMMMMMMMMMMMMMMMMD! BM.   !n Uz
    n! z: :n: ;UMBffffn. :UBBzUMDMMMMMMMMMMMMMMMBMMMDDBfnfBMMMMMMMMMBn::BMfnn!UMMMMBBMMMMMMMMMMD! nDn::,;;!zn.
    ;f   ;n,   .,Uffn! .zfff!fDMMMMMMMMMMMMMMMMMfBMDBUUfMMMMMMMMMMMMMDD;;DM:UUUMMfMnMMMMMMMMMMMD; ,MDfzzzznnn.
     f; :U  .  .;:    ,UUzn!BDMMMMMMMMMMMMMMMMMMBUMzzfMMMMMMMMMMMMMMMMDf;DD!;nUMMBzUDMMMMMMMMMMM:  ,fDDMMMnU!
     !f :f:  ,!UU!. ::z!UfzBMMMMMMMMMMMMMMMMMMMDf,zfDMMMMMMMMMMMMMBDMMMzfBB!!!nBMD!zDMMMMMMMMMMM. U, :!zzzz!
      f! :nnnBMBU..!!!:zUUnDMMMMMMMMMMMMMMMMMMMM!zMDMMMMMMMMMMMMMM!fBfUBBUU:!:nfMMMzMMMMMMMMMMMf  nM! ;n!!:
      :U ;nMDDn: ,Uz!.U UzMMMMMMMMMMMMMMMMMMMMMUfDMMMMMMMMMMMMMMMMf!UnfU;U!n!:fnMMDnnDMMMMMMMMDn  :DDf!!z,
       !nBMDB;  :n!! U,:fUDMMMMMMMMMMMMMMMMMMMUBDMMMMMMMMMMMMMMMMMMMfnUUBnnU;!nnfMMMzMMMMMMMMMDz   UDMDfzUz
     .nUnMDU.. :n;z U:zznnDMMMMMMMMMMMMMMMMMMzBMMMMMMMMMMMMMMMMMMMMMMDDMffU :!!nnMMDnnDMMMMMMMM: .  fMMBMnzU
    .f;nDDz;U. n;z ;!z::nnDMMMMMMMMMMMMMMMMMzfDMMMMMMMMMMMMMMMMMMMMMMBffn!  zz!znBMMBzMMMMMMMMB. .; ,MMMUMfzz
    n:UDMU,B, !;!;:U U.,nnDMMMMMMMMMMMMMMMDUnDMMMMMMMMMMMMMMMMMMMMMBffn!.   !n!!UfMMDzBMMMMMMDn . !; UDMf:DUU;
   !z:DMn nz ,z,U!zz.U, fUMMMMMMMMMMMMMMMMBzDMMMMMMMMMMMMMMMMMMMBfffn! ;U!  ,z,z!fMMDnnDMMMMMDz . ,n ;MMD:!D!U
   n,nDM,.f. !::!z U:U  UUBMMMMMMMMMMMMMMDzBMMMMMMMMMMMMMMMMMMBfffU:   zUz  :U.z,ffMMfUDMMMMMM;   .f.,BMD! MUn
   n.fDf ,U ,!!U U.U,U,U;nnDMMMMMMMMMMMMMfzDMMMMMMMMMMMMMMMMBffn!. .!; ,n  !U.:U nnMMBzDMMMMMM.   .f.:fMM: fUU
   n,nDU ,! ;;zU:z.U zz!;fUBMMMMMMMMMMMMDznDMMMMMMMMMMMMMMBnfn;.!!.!UU :z !U ;U. znMMBzDMMMMDf    ,f ;BDf  fUU
   ;U;MU  , z:!:,z n. !z:,fnMMMMMMMMMMMMM;BMMMMMMMMMMMMMBnfn; ,Unn,.n. !! U,:U !:;nBDfzDMMMMDU    z! fDB, ,Un:
    !zUf:   U,!,.n.:n. ,n !fnDMMMMMMMMMMB;MMMMMMMMMMMMMnff! :UU:,. U!  U; U.!:,nz:nfDnUDMMMMD;   .z nDn:, ;n!
     :!zn:  n,;z :n :nz.z! UnfDMMMMMMMMDn:DMMMMMMMMMMfnfz..Uz.   ;U! .zU  n z: n.,nfDUnDMMMMM,   , !DU U;!U;
  ..   .zz; Uz Uz :U.!z..U! nnBDMMMMMMMDz:DMMMMMMMMMnfn;, z; :zzU!  !U; .U;.n ,z !nfD!BMMMMMf      BB ;U!;.
 ;U   . z:n.!B!.!z :U ,;  !U nnBDMMMMMMM,,MMMMMMMDBnB!.z;,n. UUU  ;n! :Un::n: z;:fUMfzDMMMMDU  ,. ,MU!,U    :,
 U!    ,U:M,,nz; z: U: U;  !z,nnfDMMMMDn  fMMMMMMnff. zz!U,  !z  zU. :fU:;U  z! .UUD!nDMMMMD!  !.  BfB,;U:,;n!
 Uz,.,:z;BM. U!z:.U ,n. ;U. nn:znfMMMDB.  zDMMMMnfz ;n:.,   ,n   n  !U...n ;n;,U:Uff:MMMMMMM,  z:  ;MMM!;zUzU:
 !nnUnnnMM:,..f;z ;U .U!::U:.;U;znnMDB,   .BMMMnB; UU .!!!!!U: .U! ,n   U; zU.zUUUB,UDMMMMMB   !U .,:fMDMfnnz
  zUfBBBU.;U  ;n!! :U,.nn. U!  ;znnUz.   ! :DMnf,:n: !n:.,::  !n: :n: :U!.!:  U:Un!.MMMMMMDn   ,B, U,.zzUUU!
   ::,,.:;M,   ;n!! .U!,,:: !U!, ;nBz  .ff. ;fn,zU ,n! .!;:,;n! .UU. zU. nUz z;UU! nDMMMMMDz    B; Un f;;:.
     . z!Bn  .  :nzz: ;!zz!z..,!U:.;Bf:zUzU ;n:U; zU. zU;:;!!  !n: :n; :U;:.!;Uz: :MMMMMMMD; . ,M; nB z,
      .U!D! :: . .zUnz:,,;.:z!!::;!:.zf!!fnzB;z.,n; ;n; .;:,,;n! .Uz  zU.  ,zn!   fMMMMMMMM, . UB .Mf z;
      :zzD! !;     ,;znnz:::..,!!:!nz!;zfU!zznz!U..zz .zU;;;!!. !n: ;n;   ,nf;   zDMMMMMMMB.  !B, nD; n,
      .U:Dn :n     nn;,,!zzzUz!!z!::UfBn;,,,::!UUUn! ;z: .;::,;U!  U!  .;nfz ,; ,MMMMMMMMMB .UU .fDz Uz
       zzzMU !z.  ;DDMBz   ....  .zBBn!!znfffnUz!!!znnz:!U;:;;;. :U;,!nfn;.  !: nDMMMMMMMDf z: ;MB:.Uz
        zzzfU;:,  fMMMMDU  ;, .,;nfUUfBMMMMMMMMMBfU!;:!zUnUUUUUUnffnnz;. :.  !.;DMMMMMMMMDn , ;Dz ,!:
         ,!!zzU, zDMMMMMD! ,! .:UfBMMMMMMMMBMMMMMMMMBnz;,.,,::;;:,.    ,z:  .z fMMMMMMMMMDf   fU  .
              !z.MMMMMMMMf  z. fDMMMMMMMMMMfMMMMMMMMMMMBfU;;:.    ,;:.,;.   :z,MMMMMMMMMMMf   n;
*/
