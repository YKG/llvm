//===-- YCoreTargetTransformInfo.h - YCore specific TTI ---------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file
/// This file a TargetTransformInfo::Concept conforming object specific to the
/// YCore target machine. It uses the target's detailed information to
/// provide more precise answers to certain TTI queries, while letting the
/// target independent and default TTI implementations handle the rest.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_YCORE_YCORETARGETTRANSFORMINFO_H
#define LLVM_LIB_TARGET_YCORE_YCORETARGETTRANSFORMINFO_H

#include "YCore.h"
#include "YCoreTargetMachine.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/CodeGen/BasicTTIImpl.h"
#include "llvm/CodeGen/TargetLowering.h"

namespace llvm {

class YCoreTTIImpl : public BasicTTIImplBase<YCoreTTIImpl> {
  typedef BasicTTIImplBase<YCoreTTIImpl> BaseT;
  typedef TargetTransformInfo TTI;
  friend BaseT;

  const YCoreSubtarget *ST;
  const YCoreTargetLowering *TLI;

  const YCoreSubtarget *getST() const { return ST; }
  const YCoreTargetLowering *getTLI() const { return TLI; }

public:
  explicit YCoreTTIImpl(const YCoreTargetMachine *TM, const Function &F)
      : BaseT(TM, F.getParent()->getDataLayout()), ST(TM->getSubtargetImpl()),
        TLI(ST->getTargetLowering()) {}

  unsigned getNumberOfRegisters(unsigned ClassID) const {
    bool Vector = (ClassID == 1);
    if (Vector) {
      return 0;
    }
    return 12;
  }
};

} // end namespace llvm

#endif
