//===- YCoreMachineFunctionInfo.h - YCore machine function info -*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file declares YCore-specific per-machine-function information.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_YCORE_YCOREMACHINEFUNCTIONINFO_H
#define LLVM_LIB_TARGET_YCORE_YCOREMACHINEFUNCTIONINFO_H

#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include <cassert>
#include <utility>
#include <vector>

namespace llvm {

/// YCoreFunctionInfo - This class is derived from MachineFunction private
/// YCore target-specific information for each MachineFunction.
class YCoreFunctionInfo : public MachineFunctionInfo {
  bool LRSpillSlotSet = false;
  int LRSpillSlot;
  unsigned ReturnStackOffset;
  bool ReturnStackOffsetSet = false;
  std::vector<std::pair<MachineBasicBlock::iterator, CalleeSavedInfo>>
  SpillLabels;

  virtual void anchor();

public:
  YCoreFunctionInfo() = default;

  explicit YCoreFunctionInfo(MachineFunction &MF) {}

  ~YCoreFunctionInfo() override = default;

  int createLRSpillSlot(MachineFunction &MF);
  bool hasLRSpillSlot() { return LRSpillSlotSet; }
  int getLRSpillSlot() const {
    assert(LRSpillSlotSet && "LR Spill slot not set");
    return LRSpillSlot;
  }

  void setReturnStackOffset(unsigned value) {
    assert(!ReturnStackOffsetSet && "Return stack offset set twice");
    ReturnStackOffset = value;
    ReturnStackOffsetSet = true;
  }

  unsigned getReturnStackOffset() const {
    assert(ReturnStackOffsetSet && "Return stack offset not set");
    return ReturnStackOffset;
  }
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_YCORE_YCOREMACHINEFUNCTIONINFO_H
