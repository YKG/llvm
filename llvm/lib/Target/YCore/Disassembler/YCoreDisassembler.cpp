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

static bool readInstruction16(ArrayRef<uint8_t> Bytes, uint64_t Address,
                              uint64_t &Size, uint16_t &Insn) {
  // We want to read exactly 2 Bytes of data.
  if (Bytes.size() < 2) {
    Size = 0;
    return false;
  }
  // Encoded as a little-endian 16-bit word in the stream.
  Insn = (Bytes[0] << 0) | (Bytes[1] << 8);
  return true;
}

static bool readInstruction32(ArrayRef<uint8_t> Bytes, uint64_t Address,
                              uint64_t &Size, uint32_t &Insn) {
  // We want to read exactly 4 Bytes of data.
  if (Bytes.size() < 4) {
    Size = 0;
    return false;
  }
  // Encoded as a little-endian 32-bit word in the stream.
  Insn =
      (Bytes[0] << 0) | (Bytes[1] << 8) | (Bytes[2] << 16) | (Bytes[3] << 24);
  return true;
}

static unsigned getReg(const void *D, unsigned RC, unsigned RegNo) {
  const YCoreDisassembler *Dis = static_cast<const YCoreDisassembler*>(D);
  const MCRegisterInfo *RegInfo = Dis->getContext().getRegisterInfo();
  return *(RegInfo->getRegClass(RC).begin() + RegNo);
}

#include "YCoreGenDisassemblerTables.inc"

MCDisassembler::DecodeStatus
YCoreDisassembler::getInstruction(MCInst &instr, uint64_t &Size,
                                  ArrayRef<uint8_t> Bytes, uint64_t Address,
                                  raw_ostream &cStream) const {
  llvm_unreachable("TODO");
//
//  uint16_t insn16;
//
//  if (!readInstruction16(Bytes, Address, Size, insn16)) {
//    return Fail;
//  }
//
//  // Calling the auto-generated decoder function.
//  DecodeStatus Result = decodeInstruction(DecoderTable16, instr, insn16,
//                                          Address, this, STI);
//  if (Result != Fail) {
//    Size = 2;
//    return Result;
//  }
//
//  uint32_t insn32;
//
//  if (!readInstruction32(Bytes, Address, Size, insn32)) {
//    return Fail;
//  }
//
//  // Calling the auto-generated decoder function.
//  Result = decodeInstruction(DecoderTable32, instr, insn32, Address, this, STI);
//  if (Result != Fail) {
//    Size = 4;
//    return Result;
//  }

  return Fail;
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
