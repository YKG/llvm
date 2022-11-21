//===-- YCoreInstrInfo.cpp - YCore Instruction Information ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the YCore implementation of the TargetInstrInfo class.
//
//===----------------------------------------------------------------------===//

#include "YCoreInstrInfo.h"
#include "YCore.h"
#include "YCoreMachineFunctionInfo.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define GET_INSTRINFO_CTOR_DTOR
#include "YCoreGenInstrInfo.inc"

// Pin the vtable to this file.
void YCoreInstrInfo::anchor() {}

YCoreInstrInfo::YCoreInstrInfo()
  : YCoreGenInstrInfo(YCore::ADJCALLSTACKDOWN, YCore::ADJCALLSTACKUP),
    RI() {
}
