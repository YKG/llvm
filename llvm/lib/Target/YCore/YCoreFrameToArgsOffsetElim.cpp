//===-- YCoreFrameToArgsOffsetElim.cpp ----------------------------*- C++ -*-=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Replace Pseudo FRAME_TO_ARGS_OFFSET with the appropriate real offset.
//
//===----------------------------------------------------------------------===//

#include "YCore.h"
#include "YCoreInstrInfo.h"
#include "YCoreSubtarget.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
using namespace llvm;

namespace {
  struct YCoreFTAOElim : public MachineFunctionPass {
    static char ID;
    YCoreFTAOElim() : MachineFunctionPass(ID) {}

    bool runOnMachineFunction(MachineFunction &Fn) override;
    MachineFunctionProperties getRequiredProperties() const override {
      return MachineFunctionProperties().set(
          MachineFunctionProperties::Property::NoVRegs);
    }

    StringRef getPassName() const override {
      return "YCore FRAME_TO_ARGS_OFFSET Elimination";
    }
  };
  char YCoreFTAOElim::ID = 0;
}

/// createYCoreFrameToArgsOffsetEliminationPass - returns an instance of the
/// Frame to args offset elimination pass
FunctionPass *llvm::createYCoreFrameToArgsOffsetEliminationPass() {
  return new YCoreFTAOElim();
}

bool YCoreFTAOElim::runOnMachineFunction(MachineFunction &MF) {
  const YCoreInstrInfo &TII =
      *static_cast<const YCoreInstrInfo *>(MF.getSubtarget().getInstrInfo());
  return true;
}
