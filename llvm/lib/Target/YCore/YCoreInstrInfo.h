//===-- YCoreInstrInfo.h - YCore Instruction Information --------*- C++ -*-===//
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

#ifndef LLVM_LIB_TARGET_YCORE_YCOREINSTRINFO_H
#define LLVM_LIB_TARGET_YCORE_YCOREINSTRINFO_H

#include "YCoreRegisterInfo.h"
#include "llvm/CodeGen/TargetInstrInfo.h"

#define GET_INSTRINFO_HEADER
#include "YCoreGenInstrInfo.inc"

namespace llvm {

class YCoreInstrInfo : public YCoreGenInstrInfo {
  const YCoreRegisterInfo RI;
  virtual void anchor();
public:
  YCoreInstrInfo();

  /// getRegisterInfo - TargetInstrInfo is a superset of MRegister info.  As
  /// such, whenever a client has an instance of instruction info, it should
  /// always be able to get register info as well (through this method).
  ///
  const TargetRegisterInfo &getRegisterInfo() const { return RI; }
};

}

#endif
