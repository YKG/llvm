//===-- YCoreMCAsmInfo.h - YCore asm properties ----------------*- C++ -*--===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the declaration of the YCoreMCAsmInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_YCORE_MCTARGETDESC_YCOREMCASMINFO_H
#define LLVM_LIB_TARGET_YCORE_MCTARGETDESC_YCOREMCASMINFO_H

#include "llvm/MC/MCAsmInfoELF.h"

namespace llvm {
class Triple;

class YCoreMCAsmInfo : public MCAsmInfoELF {
  void anchor() override;

public:
  explicit YCoreMCAsmInfo(const Triple &TT);
};

} // namespace llvm

#endif
