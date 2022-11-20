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

// helper functions
static inline bool isImmUs(unsigned val) {
  return val <= 11;
}

static inline bool isImmU6(unsigned val) {
  return val < (1 << 6);
}

static inline bool isImmU16(unsigned val) {
  return val < (1 << 16);
}


static void InsertFPImmInst(MachineBasicBlock::iterator II,
                            const YCoreInstrInfo &TII,
                            unsigned Reg, unsigned FrameReg, int Offset ) {
  MachineInstr &MI = *II;
  MachineBasicBlock &MBB = *MI.getParent();
  DebugLoc dl = MI.getDebugLoc();

  switch (MI.getOpcode()) {
  case YCore::LDWFI:
    BuildMI(MBB, II, dl, TII.get(YCore::LDW_2rus), Reg)
          .addReg(FrameReg)
          .addImm(Offset)
          .addMemOperand(*MI.memoperands_begin());
    break;
  case YCore::STWFI:
    BuildMI(MBB, II, dl, TII.get(YCore::STW_2rus))
          .addReg(Reg, getKillRegState(MI.getOperand(0).isKill()))
          .addReg(FrameReg)
          .addImm(Offset)
          .addMemOperand(*MI.memoperands_begin());
    break;
//  case YCore::LDAWFI:
//    BuildMI(MBB, II, dl, TII.get(YCore::LDAWF_l2rus), Reg)
//          .addReg(FrameReg)
//          .addImm(Offset);
//    break;
  default:
    llvm_unreachable("Unexpected Opcode");
  }
}

static void InsertFPConstInst(MachineBasicBlock::iterator II,
                              const YCoreInstrInfo &TII,
                              unsigned Reg, unsigned FrameReg,
                              int Offset, RegScavenger *RS ) {
  assert(RS && "requiresRegisterScavenging failed");
  MachineInstr &MI = *II;
  MachineBasicBlock &MBB = *MI.getParent();
  DebugLoc dl = MI.getDebugLoc();
  Register ScratchOffset = RS->scavengeRegister(&YCore::GRRegsRegClass, II, 0);
  RS->setRegUsed(ScratchOffset);
  TII.loadImmediate(MBB, II, ScratchOffset, Offset);

  switch (MI.getOpcode()) {
  case YCore::LDWFI:
    BuildMI(MBB, II, dl, TII.get(YCore::LDW_3r), Reg)
          .addReg(FrameReg)
          .addReg(ScratchOffset, RegState::Kill)
          .addMemOperand(*MI.memoperands_begin());
    break;
  case YCore::STWFI:
    BuildMI(MBB, II, dl, TII.get(YCore::STW_l3r))
          .addReg(Reg, getKillRegState(MI.getOperand(0).isKill()))
          .addReg(FrameReg)
          .addReg(ScratchOffset, RegState::Kill)
          .addMemOperand(*MI.memoperands_begin());
    break;
//  case YCore::LDAWFI:
//    BuildMI(MBB, II, dl, TII.get(YCore::LDAWF_l3r), Reg)
//          .addReg(FrameReg)
//          .addReg(ScratchOffset, RegState::Kill);
//    break;
  default:
    llvm_unreachable("Unexpected Opcode");
  }
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
  case YCore::LDAWFI:
    NewOpcode = (isU6) ? YCore::LDAWSP_ru6 : YCore::LDAWSP_lru6;
    BuildMI(MBB, II, dl, TII.get(NewOpcode), Reg)
          .addImm(Offset);
    break;
  default:
    llvm_unreachable("Unexpected Opcode");
  }
}

static void InsertSPConstInst(MachineBasicBlock::iterator II,
                                const YCoreInstrInfo &TII,
                                unsigned Reg, int Offset, RegScavenger *RS ) {
  assert(RS && "requiresRegisterScavenging failed");
  MachineInstr &MI = *II;
  MachineBasicBlock &MBB = *MI.getParent();
  DebugLoc dl = MI.getDebugLoc();
  unsigned OpCode = MI.getOpcode();

  unsigned ScratchBase;
  if (OpCode==YCore::STWFI) {
    ScratchBase = RS->scavengeRegister(&YCore::GRRegsRegClass, II, 0);
    RS->setRegUsed(ScratchBase);
  } else
    ScratchBase = Reg;
  BuildMI(MBB, II, dl, TII.get(YCore::LDAWSP_ru6), ScratchBase).addImm(0);
  Register ScratchOffset = RS->scavengeRegister(&YCore::GRRegsRegClass, II, 0);
  RS->setRegUsed(ScratchOffset);
  TII.loadImmediate(MBB, II, ScratchOffset, Offset);

  switch (OpCode) {
  case YCore::LDWFI:
    BuildMI(MBB, II, dl, TII.get(YCore::LDW_3r), Reg)
          .addReg(ScratchBase, RegState::Kill)
          .addReg(ScratchOffset, RegState::Kill)
          .addMemOperand(*MI.memoperands_begin());
    break;
  case YCore::STWFI:
    BuildMI(MBB, II, dl, TII.get(YCore::STW_l3r))
          .addReg(Reg, getKillRegState(MI.getOperand(0).isKill()))
          .addReg(ScratchBase, RegState::Kill)
          .addReg(ScratchOffset, RegState::Kill)
          .addMemOperand(*MI.memoperands_begin());
    break;
//  case YCore::LDAWFI:
//    BuildMI(MBB, II, dl, TII.get(YCore::LDAWF_l3r), Reg)
//          .addReg(ScratchBase, RegState::Kill)
//          .addReg(ScratchOffset, RegState::Kill);
//    break;
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
  if (TFI->hasFP(*MF))
    return CalleeSavedRegsFP;
  return CalleeSavedRegs;
}

BitVector YCoreRegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  BitVector Reserved(getNumRegs());
  const YCoreFrameLowering *TFI = getFrameLowering(MF);

  Reserved.set(YCore::CP);
  Reserved.set(YCore::DP);
  Reserved.set(YCore::SP);
  Reserved.set(YCore::LR);
  if (TFI->hasFP(MF)) {
    Reserved.set(YCore::R10);
  }
  return Reserved;
}

bool
YCoreRegisterInfo::requiresRegisterScavenging(const MachineFunction &MF) const {
  return true;
}

bool
YCoreRegisterInfo::useFPForScavengingIndex(const MachineFunction &MF) const {
  return false;
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

  #ifndef NDEBUG
  LLVM_DEBUG(errs() << "\nFunction         : " << MF.getName() << "\n");
  LLVM_DEBUG(errs() << "<--------->\n");
  LLVM_DEBUG(MI.print(errs()));
  LLVM_DEBUG(errs() << "FrameIndex         : " << FrameIndex << "\n");
  LLVM_DEBUG(errs() << "FrameOffset        : " << Offset << "\n");
  LLVM_DEBUG(errs() << "StackSize          : " << StackSize << "\n");
#endif

  Offset += StackSize;

  Register FrameReg = getFrameRegister(MF);

  // Special handling of DBG_VALUE instructions.
  if (MI.isDebugValue()) {
    MI.getOperand(FIOperandNum).ChangeToRegister(FrameReg, false /*isDef*/);
    MI.getOperand(FIOperandNum + 1).ChangeToImmediate(Offset);
    return;
  }

  // fold constant into offset.
  Offset += MI.getOperand(FIOperandNum + 1).getImm();
  MI.getOperand(FIOperandNum + 1).ChangeToImmediate(0);

  assert(Offset%4 == 0 && "Misaligned stack offset");
  LLVM_DEBUG(errs() << "Offset             : " << Offset << "\n"
                    << "<--------->\n");
  Offset/=4;

  Register Reg = MI.getOperand(0).getReg();
  assert(YCore::GRRegsRegClass.contains(Reg) && "Unexpected register operand");

  if (TFI->hasFP(MF)) {
    if (isImmUs(Offset))
      InsertFPImmInst(II, TII, Reg, FrameReg, Offset);
    else
      InsertFPConstInst(II, TII, Reg, FrameReg, Offset, RS);
  } else {
    if (isImmU16(Offset))
      InsertSPImmInst(II, TII, Reg, Offset);
    else
      InsertSPConstInst(II, TII, Reg, Offset, RS);
  }
  // Erase old instruction.
  MachineBasicBlock &MBB = *MI.getParent();
  MBB.erase(II);
}


Register YCoreRegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  const YCoreFrameLowering *TFI = getFrameLowering(MF);

  return TFI->hasFP(MF) ? YCore::R10 : YCore::SP;
}
