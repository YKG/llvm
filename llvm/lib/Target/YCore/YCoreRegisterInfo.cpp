//===-- YCoreRegisterInfo.cpp - YCore Register Information ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the YCore implementation of the MRegisterInfo class.
//
//===----------------------------------------------------------------------===//

#include "YCoreRegisterInfo.h"
#include "YCore.h"
#include "YCoreInstrInfo.h"
#include "YCoreMachineFunctionInfo.h"
#include "YCoreSubtarget.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/RegisterScavenging.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/CodeGen/TargetFrameLowering.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"

using namespace llvm;

#define DEBUG_TYPE "ycore-reg-info"

#define GET_REGINFO_TARGET_DESC
#include "YCoreGenRegisterInfo.inc"

YCoreRegisterInfo::YCoreRegisterInfo()
  : YCoreGenRegisterInfo(YCore::LR) {
}

static inline bool isImmU6(unsigned val) {
  return val < (1 << 6);
}

static inline bool isImmU16(unsigned val) {
  return val < (1 << 16);
}


static void InsertSPImmInst(MachineBasicBlock::iterator II,
                            const YCoreInstrInfo &TII,
                            unsigned Reg, int Offset) {
  MachineInstr &MI = *II;
  MachineBasicBlock &MBB = *MI.getParent();
  DebugLoc dl = MI.getDebugLoc();
  bool isU6 = isImmU6(Offset);

  switch (MI.getOpcode()) {
  int NewOpcode;
  case YCore::LDWFI:
    NewOpcode = (isU6) ? YCore::LDWSP_ru6 : YCore::LDWSP_lru6;
    BuildMI(MBB, II, dl, TII.get(NewOpcode), Reg)
          .addImm(Offset)
          .addMemOperand(*MI.memoperands_begin());
    break;
  case YCore::STWFI:
    NewOpcode = (isU6) ? YCore::STWSP_ru6 : YCore::STWSP_lru6;
    BuildMI(MBB, II, dl, TII.get(NewOpcode))
          .addReg(Reg, getKillRegState(MI.getOperand(0).isKill()))
          .addImm(Offset)
          .addMemOperand(*MI.memoperands_begin());
    break;
  default:
    llvm_unreachable("Unexpected Opcode");
  }
}

bool YCoreRegisterInfo::needsFrameMoves(const MachineFunction &MF) {
  return MF.needsFrameMoves();
}

const MCPhysReg *
YCoreRegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
  // The callee saved registers LR & FP are explicitly handled during
  // emitPrologue & emitEpilogue and related functions.
  static const MCPhysReg CalleeSavedRegs[] = {
    YCore::R4, YCore::R5, YCore::R6, YCore::R7,
    YCore::R8, YCore::R9, YCore::R10,
    0
  };
  static const MCPhysReg CalleeSavedRegsFP[] = {
    YCore::R4, YCore::R5, YCore::R6, YCore::R7,
    YCore::R8, YCore::R9,
    0
  };
  const YCoreFrameLowering *TFI = getFrameLowering(*MF);
  return CalleeSavedRegs;
}

BitVector YCoreRegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  BitVector Reserved(getNumRegs());
  const YCoreFrameLowering *TFI = getFrameLowering(MF);

  Reserved.set(YCore::CP);
  Reserved.set(YCore::DP);
  Reserved.set(YCore::SP);
  Reserved.set(YCore::LR);
  return Reserved;
}

bool
YCoreRegisterInfo::requiresRegisterScavenging(const MachineFunction &MF) const {
  return true;
}

void
YCoreRegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator II,
                                       int SPAdj, unsigned FIOperandNum,
                                       RegScavenger *RS) const {
  assert(SPAdj == 0 && "Unexpected");
  MachineInstr &MI = *II;
  MachineOperand &FrameOp = MI.getOperand(FIOperandNum);
  int FrameIndex = FrameOp.getIndex();

  MachineFunction &MF = *MI.getParent()->getParent();
  const YCoreInstrInfo &TII =
      *static_cast<const YCoreInstrInfo *>(MF.getSubtarget().getInstrInfo());

  const YCoreFrameLowering *TFI = getFrameLowering(MF);
  int Offset = MF.getFrameInfo().getObjectOffset(FrameIndex);
  int StackSize = MF.getFrameInfo().getStackSize();

  Offset += StackSize;

  Register FrameReg = getFrameRegister(MF);

  // fold constant into offset.
  Offset += MI.getOperand(FIOperandNum + 1).getImm();
  MI.getOperand(FIOperandNum + 1).ChangeToImmediate(0);

  assert(Offset%4 == 0 && "Misaligned stack offset");
  LLVM_DEBUG(errs() << "Offset             : " << Offset << "\n"
                    << "<--------->\n");
  Offset/=4;

  Register Reg = MI.getOperand(0).getReg();
  assert(YCore::GRRegsRegClass.contains(Reg) && "Unexpected register operand");

    if (isImmU16(Offset))
      InsertSPImmInst(II, TII, Reg, Offset);
    else
      llvm_unreachable("Impossible reg-to-reg copy");//InsertSPConstInst(II, TII, Reg, Offset, RS);
  // Erase old instruction.
  MachineBasicBlock &MBB = *MI.getParent();
  MBB.erase(II);
}


Register YCoreRegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  const YCoreFrameLowering *TFI = getFrameLowering(MF);

  return TFI->hasFP(MF) ? YCore::R10 : YCore::SP;
}
