//==- IntrinsicsYCore.td - YCore intrinsics                 -*- tablegen -*-==//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines all of the YCore-specific intrinsics.
//
//===----------------------------------------------------------------------===//

let TargetPrefix = "ycore" in {  // All intrinsics start with "llvm.ycore.".
  // Miscellaneous instructions.
  def int_ycore_bitrev : Intrinsic<[llvm_i32_ty],[llvm_i32_ty],[IntrNoMem]>,
                         GCCBuiltin<"__builtin_bitrev">;
  def int_ycore_crc8 : Intrinsic<[llvm_i32_ty, llvm_i32_ty],
                                 [llvm_i32_ty,llvm_i32_ty,llvm_i32_ty],
                                 [IntrNoMem]>;
  def int_ycore_crc32 : Intrinsic<[llvm_i32_ty],
                                  [llvm_i32_ty,llvm_i32_ty,llvm_i32_ty],
                                  [IntrNoMem]>;
  def int_ycore_sext : Intrinsic<[llvm_i32_ty], [llvm_i32_ty, llvm_i32_ty],
                                 [IntrNoMem]>;
  def int_ycore_zext : Intrinsic<[llvm_i32_ty], [llvm_i32_ty, llvm_i32_ty],
                                 [IntrNoMem]>;
  def int_ycore_getid : Intrinsic<[llvm_i32_ty],[],[IntrNoMem]>,
                        GCCBuiltin<"__builtin_getid">;
  def int_ycore_getps : Intrinsic<[llvm_i32_ty],[llvm_i32_ty]>,
                        GCCBuiltin<"__builtin_getps">;
  def int_ycore_setps : Intrinsic<[],[llvm_i32_ty, llvm_i32_ty]>,
                        GCCBuiltin<"__builtin_setps">;
  def int_ycore_geted : Intrinsic<[llvm_i32_ty],[]>;
  def int_ycore_getet : Intrinsic<[llvm_i32_ty],[]>;
  def int_ycore_setsr : Intrinsic<[],[llvm_i32_ty]>;
  def int_ycore_clrsr : Intrinsic<[],[llvm_i32_ty]>;

  // Resource instructions.
  def int_ycore_getr : Intrinsic<[llvm_anyptr_ty],[llvm_i32_ty]>;
  def int_ycore_freer : Intrinsic<[],[llvm_anyptr_ty],
                                   [NoCapture<ArgIndex<0>>]>;
  def int_ycore_in : Intrinsic<[llvm_i32_ty],[llvm_anyptr_ty],[NoCapture<ArgIndex<0>>]>;
  def int_ycore_int : Intrinsic<[llvm_i32_ty],[llvm_anyptr_ty],
                                [NoCapture<ArgIndex<0>>]>;
  def int_ycore_inct : Intrinsic<[llvm_i32_ty],[llvm_anyptr_ty],
                                 [NoCapture<ArgIndex<0>>]>;
  def int_ycore_out : Intrinsic<[],[llvm_anyptr_ty, llvm_i32_ty],
                                [NoCapture<ArgIndex<0>>]>;
  def int_ycore_outt : Intrinsic<[],[llvm_anyptr_ty, llvm_i32_ty],
                                 [NoCapture<ArgIndex<0>>]>;
  def int_ycore_outct : Intrinsic<[],[llvm_anyptr_ty, llvm_i32_ty],
                                  [NoCapture<ArgIndex<0>>]>;
  def int_ycore_chkct : Intrinsic<[],[llvm_anyptr_ty, llvm_i32_ty],
                                  [NoCapture<ArgIndex<0>>]>;
  def int_ycore_testct : Intrinsic<[llvm_i32_ty],[llvm_anyptr_ty],
                                   [NoCapture<ArgIndex<0>>]>;
  def int_ycore_testwct : Intrinsic<[llvm_i32_ty],[llvm_anyptr_ty],
                                    [NoCapture<ArgIndex<0>>]>;
  def int_ycore_setd : Intrinsic<[],[llvm_anyptr_ty, llvm_i32_ty],
                                  [NoCapture<ArgIndex<0>>]>;
  def int_ycore_setc : Intrinsic<[],[llvm_anyptr_ty, llvm_i32_ty],
                                  [NoCapture<ArgIndex<0>>]>;
  def int_ycore_inshr : Intrinsic<[llvm_i32_ty],[llvm_anyptr_ty, llvm_i32_ty],
                                  [NoCapture<ArgIndex<0>>]>;
  def int_ycore_outshr : Intrinsic<[llvm_i32_ty],[llvm_anyptr_ty, llvm_i32_ty],
                                  [NoCapture<ArgIndex<0>>]>;
  def int_ycore_setpt : Intrinsic<[],[llvm_anyptr_ty, llvm_i32_ty],
                                  [NoCapture<ArgIndex<0>>]>;
  def int_ycore_clrpt : Intrinsic<[],[llvm_anyptr_ty],
                                  [NoCapture<ArgIndex<0>>]>;
  def int_ycore_getts : Intrinsic<[llvm_i32_ty],[llvm_anyptr_ty],
                                  [NoCapture<ArgIndex<0>>]>;
  def int_ycore_syncr : Intrinsic<[],[llvm_anyptr_ty],
                                  [NoCapture<ArgIndex<0>>]>;
  def int_ycore_settw : Intrinsic<[],[llvm_anyptr_ty, llvm_i32_ty],
                                  [NoCapture<ArgIndex<0>>]>;
  def int_ycore_setv : Intrinsic<[],[llvm_anyptr_ty, llvm_ptr_ty],
                                 [NoCapture<ArgIndex<0>>]>;
  def int_ycore_setev : Intrinsic<[],[llvm_anyptr_ty, llvm_ptr_ty],
                                  [NoCapture<ArgIndex<0>>]>;
  def int_ycore_eeu : Intrinsic<[],[llvm_anyptr_ty], [NoCapture<ArgIndex<0>>]>;
  def int_ycore_edu : Intrinsic<[],[llvm_anyptr_ty], [NoCapture<ArgIndex<0>>]>;
  def int_ycore_setclk : Intrinsic<[],[llvm_anyptr_ty, llvm_anyptr_ty],
                                   [NoCapture<ArgIndex<0>>, NoCapture<ArgIndex<1>>]>;
  def int_ycore_setrdy : Intrinsic<[],[llvm_anyptr_ty, llvm_anyptr_ty],
                                   [NoCapture<ArgIndex<0>>, NoCapture<ArgIndex<1>>]>;
  def int_ycore_setpsc : Intrinsic<[],[llvm_anyptr_ty, llvm_i32_ty],
                                   [NoCapture<ArgIndex<0>>]>;
  def int_ycore_peek : Intrinsic<[llvm_i32_ty],[llvm_anyptr_ty],
                                 [NoCapture<ArgIndex<0>>]>;
  def int_ycore_endin : Intrinsic<[llvm_i32_ty],[llvm_anyptr_ty],
                                 [NoCapture<ArgIndex<0>>]>;

  // Intrinsics for events.
  def int_ycore_waitevent : Intrinsic<[llvm_ptr_ty],[], [IntrReadMem]>;

  // If any of the resources owned by the thread are ready this returns the
  // vector of one of the ready resources. If no resources owned by the thread
  // are ready then the operand passed to the intrinsic is returned.
  def int_ycore_checkevent : Intrinsic<[llvm_ptr_ty],[llvm_ptr_ty]>;

  def int_ycore_clre : Intrinsic<[],[],[]>;

  // Intrinsics for threads.
  def int_ycore_getst : Intrinsic <[llvm_anyptr_ty],[llvm_anyptr_ty],
                                   [NoCapture<ArgIndex<0>>]>;
  def int_ycore_msync : Intrinsic <[],[llvm_anyptr_ty], [NoCapture<ArgIndex<0>>]>;
  def int_ycore_ssync : Intrinsic <[],[]>;
  def int_ycore_mjoin : Intrinsic <[],[llvm_anyptr_ty], [NoCapture<ArgIndex<0>>]>;
  def int_ycore_initsp : Intrinsic <[],[llvm_anyptr_ty, llvm_ptr_ty],
                                    [NoCapture<ArgIndex<0>>]>;
  def int_ycore_initpc : Intrinsic <[],[llvm_anyptr_ty, llvm_ptr_ty],
                                    [NoCapture<ArgIndex<0>>]>;
  def int_ycore_initlr : Intrinsic <[],[llvm_anyptr_ty, llvm_ptr_ty],
                                    [NoCapture<ArgIndex<0>>]>;
  def int_ycore_initcp : Intrinsic <[],[llvm_anyptr_ty, llvm_ptr_ty],
                                    [NoCapture<ArgIndex<0>>]>;
  def int_ycore_initdp : Intrinsic <[],[llvm_anyptr_ty, llvm_ptr_ty],
                                    [NoCapture<ArgIndex<0>>]>;
}
