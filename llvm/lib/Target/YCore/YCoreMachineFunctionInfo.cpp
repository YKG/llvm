//===-- YCoreMachineFunctionInfo.cpp - YCore machine function info --------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "YCoreMachineFunctionInfo.h"
#include "YCoreInstrInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/IR/Function.h"

using namespace llvm;

void YCoreFunctionInfo::anchor() { }

int YCoreFunctionInfo::createLRSpillSlot(MachineFunction &MF) {
  const TargetRegisterClass &RC = YCore::GRRegsRegClass;
  const TargetRegisterInfo &TRI = *MF.getSubtarget().getRegisterInfo();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  if (! MF.getFunction().isVarArg()) {
    // A fixed offset of 0 allows us to save / restore LR using entsp / retsp.
    LRSpillSlot = MFI.CreateFixedObject(TRI.getSpillSize(RC), 0, true);
  } else {
    // never reach
  }
  LRSpillSlotSet = true;
  return LRSpillSlot;
}

