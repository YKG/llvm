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

static DecodeStatus DecodeGRRegsRegisterClass(MCInst &Inst,
                                              unsigned RegNo,
                                              uint64_t Address,
                                              const void *Decoder);

static DecodeStatus DecodeRRegsRegisterClass(MCInst &Inst,
                                             unsigned RegNo,
                                             uint64_t Address,
                                             const void *Decoder);

static DecodeStatus DecodeBitpOperand(MCInst &Inst, unsigned Val,
                                      uint64_t Address, const void *Decoder);

static DecodeStatus Decode3RInstruction(MCInst &Inst,
                                        unsigned Insn,
                                        uint64_t Address,
                                        const void *Decoder);

static DecodeStatus Decode3RImmInstruction(MCInst &Inst,
                                           unsigned Insn,
                                           uint64_t Address,
                                           const void *Decoder);

static DecodeStatus Decode2RUSInstruction(MCInst &Inst,
                                          unsigned Insn,
                                          uint64_t Address,
                                          const void *Decoder);

static DecodeStatus Decode2RUSBitpInstruction(MCInst &Inst,
                                              unsigned Insn,
                                              uint64_t Address,
                                              const void *Decoder);

#include "YCoreGenDisassemblerTables.inc"

static DecodeStatus DecodeGRRegsRegisterClass(MCInst &Inst,
                                              unsigned RegNo,
                                              uint64_t Address,
                                              const void *Decoder)
{
  if (RegNo > 11)
    return MCDisassembler::Fail;
  unsigned Reg = getReg(Decoder, YCore::GRRegsRegClassID, RegNo);
  Inst.addOperand(MCOperand::createReg(Reg));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeRRegsRegisterClass(MCInst &Inst,
                                             unsigned RegNo,
                                             uint64_t Address,
                                             const void *Decoder)
{
  if (RegNo > 15)
    return MCDisassembler::Fail;
  unsigned Reg = getReg(Decoder, YCore::RRegsRegClassID, RegNo);
  Inst.addOperand(MCOperand::createReg(Reg));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeBitpOperand(MCInst &Inst, unsigned Val,
                                      uint64_t Address, const void *Decoder) {
  if (Val > 11)
    return MCDisassembler::Fail;
  static const unsigned Values[] = {
    32 /*bpw*/, 1, 2, 3, 4, 5, 6, 7, 8, 16, 24, 32
  };
  Inst.addOperand(MCOperand::createImm(Values[Val]));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeNegImmOperand(MCInst &Inst, unsigned Val,
                                        uint64_t Address, const void *Decoder) {
  Inst.addOperand(MCOperand::createImm(-(int64_t)Val));
  return MCDisassembler::Success;
}

static DecodeStatus
Decode2OpInstruction(unsigned Insn, unsigned &Op1, unsigned &Op2) {
  unsigned Combined = fieldFromInstruction(Insn, 6, 5);
  if (Combined < 27)
    return MCDisassembler::Fail;
  if (fieldFromInstruction(Insn, 5, 1)) {
    if (Combined == 31)
      return MCDisassembler::Fail;
    Combined += 5;
  }
  Combined -= 27;
  unsigned Op1High = Combined % 3;
  unsigned Op2High = Combined / 3;
  Op1 = (Op1High << 2) | fieldFromInstruction(Insn, 2, 2);
  Op2 = (Op2High << 2) | fieldFromInstruction(Insn, 0, 2);
  return MCDisassembler::Success;
}

static DecodeStatus
Decode3OpInstruction(unsigned Insn, unsigned &Op1, unsigned &Op2,
                     unsigned &Op3) {
  unsigned Combined = fieldFromInstruction(Insn, 6, 5);
  if (Combined >= 27)
    return MCDisassembler::Fail;

  unsigned Op1High = Combined % 3;
  unsigned Op2High = (Combined / 3) % 3;
  unsigned Op3High = Combined / 9;
  Op1 = (Op1High << 2) | fieldFromInstruction(Insn, 4, 2);
  Op2 = (Op2High << 2) | fieldFromInstruction(Insn, 2, 2);
  Op3 = (Op3High << 2) | fieldFromInstruction(Insn, 0, 2);
  return MCDisassembler::Success;
}

static DecodeStatus
Decode2OpInstructionFail(MCInst &Inst, unsigned Insn, uint64_t Address,
                         const void *Decoder) {
  return MCDisassembler::Fail;
}

static DecodeStatus
Decode2RInstruction(MCInst &Inst, unsigned Insn, uint64_t Address,
                    const void *Decoder) {
  unsigned Op1, Op2;
  DecodeStatus S = Decode2OpInstruction(Insn, Op1, Op2);
  if (S != MCDisassembler::Success)
    return Decode2OpInstructionFail(Inst, Insn, Address, Decoder);

  DecodeGRRegsRegisterClass(Inst, Op1, Address, Decoder);
  DecodeGRRegsRegisterClass(Inst, Op2, Address, Decoder);
  return S;
}

static DecodeStatus
Decode2RImmInstruction(MCInst &Inst, unsigned Insn, uint64_t Address,
                       const void *Decoder) {
  unsigned Op1, Op2;
  DecodeStatus S = Decode2OpInstruction(Insn, Op1, Op2);
  if (S != MCDisassembler::Success)
    return Decode2OpInstructionFail(Inst, Insn, Address, Decoder);

  Inst.addOperand(MCOperand::createImm(Op1));
  DecodeGRRegsRegisterClass(Inst, Op2, Address, Decoder);
  return S;
}

static DecodeStatus
DecodeR2RInstruction(MCInst &Inst, unsigned Insn, uint64_t Address,
                     const void *Decoder) {
  unsigned Op1, Op2;
  DecodeStatus S = Decode2OpInstruction(Insn, Op2, Op1);
  if (S != MCDisassembler::Success)
    return Decode2OpInstructionFail(Inst, Insn, Address, Decoder);

  DecodeGRRegsRegisterClass(Inst, Op1, Address, Decoder);
  DecodeGRRegsRegisterClass(Inst, Op2, Address, Decoder);
  return S;
}

static DecodeStatus
Decode2RSrcDstInstruction(MCInst &Inst, unsigned Insn, uint64_t Address,
                          const void *Decoder) {
  unsigned Op1, Op2;
  DecodeStatus S = Decode2OpInstruction(Insn, Op1, Op2);
  if (S != MCDisassembler::Success)
    return Decode2OpInstructionFail(Inst, Insn, Address, Decoder);

  DecodeGRRegsRegisterClass(Inst, Op1, Address, Decoder);
  DecodeGRRegsRegisterClass(Inst, Op1, Address, Decoder);
  DecodeGRRegsRegisterClass(Inst, Op2, Address, Decoder);
  return S;
}

static DecodeStatus
DecodeRUSInstruction(MCInst &Inst, unsigned Insn, uint64_t Address,
                     const void *Decoder) {
  unsigned Op1, Op2;
  DecodeStatus S = Decode2OpInstruction(Insn, Op1, Op2);
  if (S != MCDisassembler::Success)
    return Decode2OpInstructionFail(Inst, Insn, Address, Decoder);

  DecodeGRRegsRegisterClass(Inst, Op1, Address, Decoder);
  Inst.addOperand(MCOperand::createImm(Op2));
  return S;
}

static DecodeStatus
DecodeRUSBitpInstruction(MCInst &Inst, unsigned Insn, uint64_t Address,
                         const void *Decoder) {
  unsigned Op1, Op2;
  DecodeStatus S = Decode2OpInstruction(Insn, Op1, Op2);
  if (S != MCDisassembler::Success)
    return Decode2OpInstructionFail(Inst, Insn, Address, Decoder);

  DecodeGRRegsRegisterClass(Inst, Op1, Address, Decoder);
  DecodeBitpOperand(Inst, Op2, Address, Decoder);
  return S;
}

static DecodeStatus
DecodeRUSSrcDstBitpInstruction(MCInst &Inst, unsigned Insn, uint64_t Address,
                               const void *Decoder) {
  unsigned Op1, Op2;
  DecodeStatus S = Decode2OpInstruction(Insn, Op1, Op2);
  if (S != MCDisassembler::Success)
    return Decode2OpInstructionFail(Inst, Insn, Address, Decoder);

  DecodeGRRegsRegisterClass(Inst, Op1, Address, Decoder);
  DecodeGRRegsRegisterClass(Inst, Op1, Address, Decoder);
  DecodeBitpOperand(Inst, Op2, Address, Decoder);
  return S;
}

static DecodeStatus
DecodeL2OpInstructionFail(MCInst &Inst, unsigned Insn, uint64_t Address,
                          const void *Decoder) {
  return MCDisassembler::Fail;
}

static DecodeStatus
Decode3RInstruction(MCInst &Inst, unsigned Insn, uint64_t Address,
                    const void *Decoder) {
  unsigned Op1, Op2, Op3;
  DecodeStatus S = Decode3OpInstruction(Insn, Op1, Op2, Op3);
  if (S == MCDisassembler::Success) {
    DecodeGRRegsRegisterClass(Inst, Op1, Address, Decoder);
    DecodeGRRegsRegisterClass(Inst, Op2, Address, Decoder);
    DecodeGRRegsRegisterClass(Inst, Op3, Address, Decoder);
  }
  return S;
}

static DecodeStatus
Decode3RImmInstruction(MCInst &Inst, unsigned Insn, uint64_t Address,
                       const void *Decoder) {
  unsigned Op1, Op2, Op3;
  DecodeStatus S = Decode3OpInstruction(Insn, Op1, Op2, Op3);
  if (S == MCDisassembler::Success) {
    Inst.addOperand(MCOperand::createImm(Op1));
    DecodeGRRegsRegisterClass(Inst, Op2, Address, Decoder);
    DecodeGRRegsRegisterClass(Inst, Op3, Address, Decoder);
  }
  return S;
}

static DecodeStatus
Decode2RUSInstruction(MCInst &Inst, unsigned Insn, uint64_t Address,
                      const void *Decoder) {
  unsigned Op1, Op2, Op3;
  DecodeStatus S = Decode3OpInstruction(Insn, Op1, Op2, Op3);
  if (S == MCDisassembler::Success) {
    DecodeGRRegsRegisterClass(Inst, Op1, Address, Decoder);
    DecodeGRRegsRegisterClass(Inst, Op2, Address, Decoder);
    Inst.addOperand(MCOperand::createImm(Op3));
  }
  return S;
}

static DecodeStatus
Decode2RUSBitpInstruction(MCInst &Inst, unsigned Insn, uint64_t Address,
                      const void *Decoder) {
  unsigned Op1, Op2, Op3;
  DecodeStatus S = Decode3OpInstruction(Insn, Op1, Op2, Op3);
  if (S == MCDisassembler::Success) {
    DecodeGRRegsRegisterClass(Inst, Op1, Address, Decoder);
    DecodeGRRegsRegisterClass(Inst, Op2, Address, Decoder);
    DecodeBitpOperand(Inst, Op3, Address, Decoder);
  }
  return S;
}

static DecodeStatus
DecodeL3RInstruction(MCInst &Inst, unsigned Insn, uint64_t Address,
                     const void *Decoder) {
  unsigned Op1, Op2, Op3;
  DecodeStatus S =
    Decode3OpInstruction(fieldFromInstruction(Insn, 0, 16), Op1, Op2, Op3);
  if (S == MCDisassembler::Success) {
    DecodeGRRegsRegisterClass(Inst, Op1, Address, Decoder);
    DecodeGRRegsRegisterClass(Inst, Op2, Address, Decoder);
    DecodeGRRegsRegisterClass(Inst, Op3, Address, Decoder);
  }
  return S;
}

static DecodeStatus
DecodeL3RSrcDstInstruction(MCInst &Inst, unsigned Insn, uint64_t Address,
                           const void *Decoder) {
  unsigned Op1, Op2, Op3;
  DecodeStatus S =
  Decode3OpInstruction(fieldFromInstruction(Insn, 0, 16), Op1, Op2, Op3);
  if (S == MCDisassembler::Success) {
    DecodeGRRegsRegisterClass(Inst, Op1, Address, Decoder);
    DecodeGRRegsRegisterClass(Inst, Op1, Address, Decoder);
    DecodeGRRegsRegisterClass(Inst, Op2, Address, Decoder);
    DecodeGRRegsRegisterClass(Inst, Op3, Address, Decoder);
  }
  return S;
}

static DecodeStatus
DecodeL2RUSInstruction(MCInst &Inst, unsigned Insn, uint64_t Address,
                       const void *Decoder) {
  unsigned Op1, Op2, Op3;
  DecodeStatus S =
  Decode3OpInstruction(fieldFromInstruction(Insn, 0, 16), Op1, Op2, Op3);
  if (S == MCDisassembler::Success) {
    DecodeGRRegsRegisterClass(Inst, Op1, Address, Decoder);
    DecodeGRRegsRegisterClass(Inst, Op2, Address, Decoder);
    Inst.addOperand(MCOperand::createImm(Op3));
  }
  return S;
}

static DecodeStatus
DecodeL2RUSBitpInstruction(MCInst &Inst, unsigned Insn, uint64_t Address,
                           const void *Decoder) {
  unsigned Op1, Op2, Op3;
  DecodeStatus S =
  Decode3OpInstruction(fieldFromInstruction(Insn, 0, 16), Op1, Op2, Op3);
  if (S == MCDisassembler::Success) {
    DecodeGRRegsRegisterClass(Inst, Op1, Address, Decoder);
    DecodeGRRegsRegisterClass(Inst, Op2, Address, Decoder);
    DecodeBitpOperand(Inst, Op3, Address, Decoder);
  }
  return S;
}

static DecodeStatus
DecodeL6RInstruction(MCInst &Inst, unsigned Insn, uint64_t Address,
                     const void *Decoder) {
  unsigned Op1, Op2, Op3, Op4, Op5, Op6;
  DecodeStatus S =
    Decode3OpInstruction(fieldFromInstruction(Insn, 0, 16), Op1, Op2, Op3);
  if (S != MCDisassembler::Success)
    return S;
  S = Decode3OpInstruction(fieldFromInstruction(Insn, 16, 16), Op4, Op5, Op6);
  if (S != MCDisassembler::Success)
    return S;
  DecodeGRRegsRegisterClass(Inst, Op1, Address, Decoder);
  DecodeGRRegsRegisterClass(Inst, Op4, Address, Decoder);
  DecodeGRRegsRegisterClass(Inst, Op2, Address, Decoder);
  DecodeGRRegsRegisterClass(Inst, Op3, Address, Decoder);
  DecodeGRRegsRegisterClass(Inst, Op5, Address, Decoder);
  DecodeGRRegsRegisterClass(Inst, Op6, Address, Decoder);
  return S;
}

MCDisassembler::DecodeStatus
YCoreDisassembler::getInstruction(MCInst &instr, uint64_t &Size,
                                  ArrayRef<uint8_t> Bytes, uint64_t Address,
                                  raw_ostream &cStream) const {
  uint16_t insn16;

  if (!readInstruction16(Bytes, Address, Size, insn16)) {
    return Fail;
  }

  // Calling the auto-generated decoder function.
  DecodeStatus Result = decodeInstruction(DecoderTable16, instr, insn16,
                                          Address, this, STI);
  if (Result != Fail) {
    Size = 2;
    return Result;
  }

  uint32_t insn32;

  if (!readInstruction32(Bytes, Address, Size, insn32)) {
    return Fail;
  }

  // Calling the auto-generated decoder function.
  Result = decodeInstruction(DecoderTable32, instr, insn32, Address, this, STI);
  if (Result != Fail) {
    Size = 4;
    return Result;
  }

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
