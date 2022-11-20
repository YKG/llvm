//===-- YCoreMCTargetDesc.h - YCore Target Descriptions ---------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file provides YCore specific target descriptions.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_YCORE_MCTARGETDESC_YCOREMCTARGETDESC_H
#define LLVM_LIB_TARGET_YCORE_MCTARGETDESC_YCOREMCTARGETDESC_H

// Defines symbolic names for YCore registers.  This defines a mapping from
// register name to register number.
//
#define GET_REGINFO_ENUM
#include "YCoreGenRegisterInfo.inc"

// Defines symbolic names for the YCore instructions.
//
#define GET_INSTRINFO_ENUM
#include "YCoreGenInstrInfo.inc"

#define GET_SUBTARGETINFO_ENUM
#include "YCoreGenSubtargetInfo.inc"

#endif // LLVM_LIB_TARGET_YCORE_MCTARGETDESC_YCOREMCTARGETDESC_H
