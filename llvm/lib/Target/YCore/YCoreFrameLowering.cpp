//===-- YCoreFrameLowering.cpp - Frame info for YCore Target --------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains YCore frame information that doesn't fit anywhere else
// cleanly...
//
//===----------------------------------------------------------------------===//

#include "YCoreFrameLowering.h"
#include "YCore.h"
#include "YCoreInstrInfo.h"
#include "YCoreMachineFunctionInfo.h"
#include "YCoreSubtarget.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/RegisterScavenging.h"
#include "llvm/CodeGen/TargetLowering.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Target/TargetOptions.h"
#include <algorithm> // std::sort

using namespace llvm;

static const unsigned FramePtr = YCore::R10;
static const int MaxImmU16 = (1<<16) - 1;

// helper functions. FIXME: Eliminate.
static inline bool isImmU6(unsigned val) {
  return val < (1 << 6);
}

static inline bool isImmU16(unsigned val) {
  return val < (1 << 16);
}

// Helper structure with compare function for handling stack slots.
namespace {
struct StackSlotInfo {
  int FI;
  int Offset;
  unsigned Reg;
  StackSlotInfo(int f, int o, int r) : FI(f), Offset(o), Reg(r){};
};
}  // end anonymous namespace

static bool CompareSSIOffset(const StackSlotInfo& a, const StackSlotInfo& b) {
  return a.Offset < b.Offset;
}

/// The SP register is moved in steps of 'MaxImmU16' towards the bottom of the
/// frame. During these steps, it may be necessary to spill registers.
/// IfNeededExtSP emits the necessary EXTSP instructions to move the SP only
/// as far as to make 'OffsetFromBottom' reachable using an STWSP_lru6.
/// \param OffsetFromTop the spill offset from the top of the frame.
/// \param [in,out] Adjusted the current SP offset from the top of the frame.
static void IfNeededExtSP(MachineBasicBlock &MBB,
                          MachineBasicBlock::iterator MBBI, const DebugLoc &dl,
                          const TargetInstrInfo &TII, int OffsetFromTop,
                          int &Adjusted, int FrameSize, bool emitFrameMoves) {
  while (OffsetFromTop > Adjusted) {
    assert(Adjusted < FrameSize && "OffsetFromTop is beyond FrameSize");
    int remaining = FrameSize - Adjusted;
    int OpImm = (remaining > MaxImmU16) ? MaxImmU16 : remaining;
    int Opcode = isImmU6(OpImm) ? YCore::EXTSP_u6 : YCore::EXTSP_lu6;
    BuildMI(MBB, MBBI, dl, TII.get(Opcode)).addImm(OpImm);
    Adjusted += OpImm;
//    if (emitFrameMoves)
//      EmitDefCfaOffset(MBB, MBBI, dl, TII, Adjusted*4);
  }
}

/// The SP register is moved in steps of 'MaxImmU16' towards the top of the
/// frame. During these steps, it may be necessary to re-load registers.
/// IfNeededLDAWSP emits the necessary LDAWSP instructions to move the SP only
/// as far as to make 'OffsetFromTop' reachable using an LDAWSP_lru6.
/// \param OffsetFromTop the spill offset from the top of the frame.
/// \param [in,out] RemainingAdj the current SP offset from the top of the
/// frame.
static void IfNeededLDAWSP(MachineBasicBlock &MBB,
                           MachineBasicBlock::iterator MBBI, const DebugLoc &dl,
                           const TargetInstrInfo &TII, int OffsetFromTop,
                           int &RemainingAdj) {
}

/// Creates an ordered list of registers that are spilled
/// during the emitPrologue/emitEpilogue.
/// Registers are ordered according to their frame offset.
/// As offsets are negative, the largest offsets will be first.
static void GetSpillList(SmallVectorImpl<StackSlotInfo> &SpillList,
                         MachineFrameInfo &MFI, YCoreFunctionInfo *XFI,
                         bool fetchLR, bool fetchFP) {
  llvm::sort(SpillList, CompareSSIOffset);
}

//===----------------------------------------------------------------------===//
// YCoreFrameLowering:
//===----------------------------------------------------------------------===//

YCoreFrameLowering::YCoreFrameLowering(const YCoreSubtarget &sti)
    : TargetFrameLowering(TargetFrameLowering::StackGrowsDown, Align(4), 0) {
  // Do nothing
}

bool YCoreFrameLowering::hasFP(const MachineFunction &MF) const {
  return MF.getTarget().Options.DisableFramePointerElim(MF) ||
         MF.getFrameInfo().hasVarSizedObjects();
}

void YCoreFrameLowering::emitPrologue(MachineFunction &MF,
                                      MachineBasicBlock &MBB) const {
  assert(&MF.front() == &MBB && "Shrink-wrapping not yet supported");
  MachineBasicBlock::iterator MBBI = MBB.begin();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  MachineModuleInfo *MMI = &MF.getMMI();
  const MCRegisterInfo *MRI = MMI->getContext().getRegisterInfo();
  const YCoreInstrInfo &TII = *MF.getSubtarget<YCoreSubtarget>().getInstrInfo();
  YCoreFunctionInfo *XFI = MF.getInfo<YCoreFunctionInfo>();
  // Debug location must be unknown since the first debug location is used
  // to determine the end of the prologue.
  DebugLoc dl;

  if (MFI.getMaxAlign() > getStackAlign())
    report_fatal_error("emitPrologue unsupported alignment: " +
                       Twine(MFI.getMaxAlign().value()));

  const AttributeList &PAL = MF.getFunction().getAttributes();
  if (PAL.hasAttrSomewhere(Attribute::Nest))
    BuildMI(MBB, MBBI, dl, TII.get(YCore::LDWSP_ru6), YCore::R11).addImm(0);
    // FIX: Needs addMemOperand() but can't use getFixedStack() or getStack().

  // Work out frame sizes.
  // We will adjust the SP in stages towards the final FrameSize.
  assert(MFI.getStackSize()%4 == 0 && "Misaligned frame size");
  const int FrameSize = MFI.getStackSize() / 4;
  int Adjusted = 0;

  bool saveLR = XFI->hasLRSpillSlot();
  bool UseENTSP = saveLR && FrameSize
                  && (MFI.getObjectOffset(XFI->getLRSpillSlot()) == 0);
  if (UseENTSP)
    saveLR = false;
  bool FP = hasFP(MF);
  bool emitFrameMoves = YCoreRegisterInfo::needsFrameMoves(MF);

  if (UseENTSP) {
    // Allocate space on the stack at the same time as saving LR.
    Adjusted = (FrameSize > MaxImmU16) ? MaxImmU16 : FrameSize;
    int Opcode = isImmU6(Adjusted) ? YCore::ENTSP_u6 : YCore::ENTSP_lu6;
    MBB.addLiveIn(YCore::LR);
    MachineInstrBuilder MIB = BuildMI(MBB, MBBI, dl, TII.get(Opcode));
    MIB.addImm(Adjusted);
    MIB->addRegisterKilled(YCore::LR, MF.getSubtarget().getRegisterInfo(),
                           true);
  }

  // If necessary, save LR and FP to the stack, as we EXTSP.
  SmallVector<StackSlotInfo,2> SpillList;
  GetSpillList(SpillList, MFI, XFI, saveLR, FP);
  // We want the nearest (negative) offsets first, so reverse list.
  std::reverse(SpillList.begin(), SpillList.end());

  // Complete any remaining Stack adjustment.
  IfNeededExtSP(MBB, MBBI, dl, TII, FrameSize, Adjusted, FrameSize,
                emitFrameMoves);
  assert(Adjusted==FrameSize && "IfNeededExtSP has not completed adjustment");

  if (FP) {
    // Set the FP from the SP.
    BuildMI(MBB, MBBI, dl, TII.get(YCore::LDAWSP_ru6), FramePtr).addImm(0);
  }

}

void YCoreFrameLowering::emitEpilogue(MachineFunction &MF,
                                     MachineBasicBlock &MBB) const {
  MachineFrameInfo &MFI = MF.getFrameInfo();
  MachineBasicBlock::iterator MBBI = MBB.getLastNonDebugInstr();
  const YCoreInstrInfo &TII = *MF.getSubtarget<YCoreSubtarget>().getInstrInfo();
  YCoreFunctionInfo *XFI = MF.getInfo<YCoreFunctionInfo>();
  DebugLoc dl = MBBI->getDebugLoc();
  unsigned RetOpcode = MBBI->getOpcode();

  // Work out frame sizes.
  // We will adjust the SP in stages towards the final FrameSize.
  int RemainingAdj = MFI.getStackSize();
  assert(RemainingAdj%4 == 0 && "Misaligned frame size");
  RemainingAdj /= 4;

  bool restoreLR = XFI->hasLRSpillSlot();
  bool UseRETSP = restoreLR && RemainingAdj
                  && (MFI.getObjectOffset(XFI->getLRSpillSlot()) == 0);
  if (UseRETSP)
    restoreLR = false;
  bool FP = hasFP(MF);

  // If necessary, restore LR and FP from the stack, as we EXTSP.
  SmallVector<StackSlotInfo,2> SpillList;
  GetSpillList(SpillList, MFI, XFI, restoreLR, FP);
//  RestoreSpillList(MBB, MBBI, dl, TII, RemainingAdj, SpillList);

  if (RemainingAdj) {
    // Complete all but one of the remaining Stack adjustments.
    IfNeededLDAWSP(MBB, MBBI, dl, TII, 0, RemainingAdj);
    if (UseRETSP) {
      // Fold prologue into return instruction
      assert(RetOpcode == YCore::RETSP_u6
             || RetOpcode == YCore::RETSP_lu6);
      int Opcode = isImmU6(RemainingAdj) ? YCore::RETSP_u6 : YCore::RETSP_lu6;
      MachineInstrBuilder MIB = BuildMI(MBB, MBBI, dl, TII.get(Opcode))
                                  .addImm(RemainingAdj);
      for (unsigned i = 3, e = MBBI->getNumOperands(); i < e; ++i)
        MIB->addOperand(MBBI->getOperand(i)); // copy any variadic operands
      MBB.erase(MBBI);  // Erase the previous return instruction.
    } else {
      llvm_unreachable("TODO");
      // Don't erase the return instruction.
    }
  } // else Don't erase the return instruction.
}

void YCoreFrameLowering::determineCalleeSaves(MachineFunction &MF,
                                              BitVector &SavedRegs,
                                              RegScavenger *RS) const {
  TargetFrameLowering::determineCalleeSaves(MF, SavedRegs, RS);

  YCoreFunctionInfo *XFI = MF.getInfo<YCoreFunctionInfo>();

  const MachineRegisterInfo &MRI = MF.getRegInfo();
  bool LRUsed = MRI.isPhysRegModified(YCore::LR);

  if (!LRUsed && !MF.getFunction().isVarArg() &&
      MF.getFrameInfo().estimateStackSize(MF))
    // If we need to extend the stack it is more efficient to use entsp / retsp.
    // We force the LR to be saved so these instructions are used.
    LRUsed = true;

  if (LRUsed) {
    // We will handle the LR in the prologue/epilogue
    // and allocate space on the stack ourselves.
    SavedRegs.reset(YCore::LR);
    XFI->createLRSpillSlot(MF);
  }
}

void YCoreFrameLowering::
processFunctionBeforeFrameFinalized(MachineFunction &MF,
                                    RegScavenger *RS) const {
  assert(RS && "requiresRegisterScavenging failed");
  MachineFrameInfo &MFI = MF.getFrameInfo();
  const TargetRegisterClass &RC = YCore::GRRegsRegClass;
  const TargetRegisterInfo &TRI = *MF.getSubtarget().getRegisterInfo();
  YCoreFunctionInfo *XFI = MF.getInfo<YCoreFunctionInfo>();
  // Reserve slots close to SP or frame pointer for Scavenging spills.
  // When using SP for small frames, we don't need any scratch registers.
  // When using SP for large frames, we may need 2 scratch registers.
  // When using FP, for large or small frames, we may need 1 scratch register.
  unsigned Size = TRI.getSpillSize(RC);
  Align Alignment = TRI.getSpillAlign(RC);
  if (XFI->isLargeFrame(MF) || hasFP(MF))
    RS->addScavengingFrameIndex(MFI.CreateStackObject(Size, Alignment, false));
  if (XFI->isLargeFrame(MF) && !hasFP(MF))
    RS->addScavengingFrameIndex(MFI.CreateStackObject(Size, Alignment, false));
}
