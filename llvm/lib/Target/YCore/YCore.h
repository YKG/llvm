//===-- YCore.h - Top-level interface for YCore representation --*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the entry points for global functions defined in the LLVM
// YCore back-end.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_YCORE_YCORE_H
#define LLVM_LIB_TARGET_YCORE_YCORE_H

#include "MCTargetDesc/YCoreMCTargetDesc.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {
  class FunctionPass;
  class ModulePass;
  class TargetMachine;
  class YCoreTargetMachine;

  void initializeYCoreLowerThreadLocalPass(PassRegistry &p);

  FunctionPass *createYCoreFrameToArgsOffsetEliminationPass();
  FunctionPass *createYCoreISelDag(YCoreTargetMachine &TM,
                                   CodeGenOpt::Level OptLevel);
  ModulePass *createYCoreLowerThreadLocalPass();

} // end namespace llvm;

#endif
