//===-- YCoreTargetStreamer.h - YCore Target Streamer ----------*- C++ -*--===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_YCORE_YCORETARGETSTREAMER_H
#define LLVM_LIB_TARGET_YCORE_YCORETARGETSTREAMER_H

#include "llvm/MC/MCStreamer.h"

namespace llvm {
class YCoreTargetStreamer : public MCTargetStreamer {
public:
  YCoreTargetStreamer(MCStreamer &S);
  ~YCoreTargetStreamer() override;
  virtual void emitCCTopData(StringRef Name) = 0;
  virtual void emitCCTopFunction(StringRef Name) = 0;
  virtual void emitCCBottomData(StringRef Name) = 0;
  virtual void emitCCBottomFunction(StringRef Name) = 0;
};
}

#endif
