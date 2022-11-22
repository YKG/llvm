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

bool YCoreFunctionInfo::isLargeFrame(const MachineFunction &MF) const {
  if (CachedEStackSize == -1) {
    CachedEStackSize = MF.getFrameInfo().estimateStackSize(MF);
  }
  // isLargeFrame() is used when deciding if spill slots should be added to
  // allow eliminateFrameIndex() to scavenge registers.
  // This is only required when there is no FP and offsets are greater than
  // ~256KB (~64Kwords). Thus only for code run on the emulator!
  //
  // The arbitrary value of 0xf000 allows frames of up to ~240KB before spill
  // slots are added for the use of eliminateFrameIndex() register scavenging.
  // For frames less than 240KB, it is assumed that there will be less than
  // 16KB of function arguments.
  return CachedEStackSize > 0xf000;
}

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

