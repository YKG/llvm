//===-- YCoreSubtarget.cpp - YCore Subtarget Information ------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the YCore specific subclass of TargetSubtargetInfo.
//
//===----------------------------------------------------------------------===//

#include "YCoreSubtarget.h"
#include "YCore.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

#define DEBUG_TYPE "ycore-subtarget"

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "YCoreGenSubtargetInfo.inc"

void YCoreSubtarget::anchor() { }

YCoreSubtarget::YCoreSubtarget(const Triple &TT, const std::string &CPU,
                               const std::string &FS, const TargetMachine &TM)
    : YCoreGenSubtargetInfo(TT, CPU, /*TuneCPU*/ CPU, FS), FrameLowering(*this),
      TLInfo(TM, *this) {}
