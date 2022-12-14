//===- YCoreDisassembler.cpp - Disassembler for YCore -----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file is part of the YCore Disassembler.
///
//===----------------------------------------------------------------------===//

#include "TargetInfo/YCoreTargetInfo.h"
#include "YCore.h"
#include "YCoreRegisterInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/MC/MCFixedLenDisassembler.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

#define DEBUG_TYPE "ycore-disassembler"

typedef MCDisassembler::DecodeStatus DecodeStatus;

namespace {

/// A disassembler class for YCore.
class YCoreDisassembler : public MCDisassembler {
public:
  YCoreDisassembler(const MCSubtargetInfo &STI, MCContext &Ctx) :
    MCDisassembler(STI, Ctx) {}

  DecodeStatus getInstruction(MCInst &Instr, uint64_t &Size,
                              ArrayRef<uint8_t> Bytes, uint64_t Address,
                              raw_ostream &CStream) const override;
};
}

#include "YCoreGenDisassemblerTables.inc"

MCDisassembler::DecodeStatus
YCoreDisassembler::getInstruction(MCInst &instr, uint64_t &Size,
                                  ArrayRef<uint8_t> Bytes, uint64_t Address,
                                  raw_ostream &cStream) const {
  llvm_unreachable("TODO");
}

static MCDisassembler *createYCoreDisassembler(const Target &T,
                                               const MCSubtargetInfo &STI,
                                               MCContext &Ctx) {
  return new YCoreDisassembler(STI, Ctx);
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeYCoreDisassembler() {
  // Register the disassembler.
  TargetRegistry::RegisterMCDisassembler(getTheYCoreTarget(),
                                         createYCoreDisassembler);
}
