//===-- YCoreMCTargetDesc.cpp - YCore Target Descriptions -----------------===//
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

#include "MCTargetDesc/YCoreMCTargetDesc.h"
#include "MCTargetDesc/YCoreInstPrinter.h"
#include "MCTargetDesc/YCoreMCAsmInfo.h"
#include "TargetInfo/YCoreTargetInfo.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/MC/MCDwarf.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

#define GET_INSTRINFO_MC_DESC
#include "YCoreGenInstrInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "YCoreGenSubtargetInfo.inc"

#define GET_REGINFO_MC_DESC
#include "YCoreGenRegisterInfo.inc"

static MCInstrInfo *createYCoreMCInstrInfo() {
  MCInstrInfo *X = new MCInstrInfo();
  InitYCoreMCInstrInfo(X);
  return X;
}

static MCRegisterInfo *createYCoreMCRegisterInfo(const Triple &TT) {
  MCRegisterInfo *X = new MCRegisterInfo();
  InitYCoreMCRegisterInfo(X, YCore::LR);
  return X;
}

static MCSubtargetInfo *
createYCoreMCSubtargetInfo(const Triple &TT, StringRef CPU, StringRef FS) {
  return createYCoreMCSubtargetInfoImpl(TT, CPU, /*TuneCPU*/ CPU, FS);
}

static MCAsmInfo *createYCoreMCAsmInfo(const MCRegisterInfo &MRI,
                                       const Triple &TT,
                                       const MCTargetOptions &Options) {
  MCAsmInfo *MAI = new YCoreMCAsmInfo(TT);

  // Initial state of the frame pointer is SP.
  MCCFIInstruction Inst = MCCFIInstruction::cfiDefCfa(nullptr, YCore::SP, 0);
  MAI->addInitialFrameState(Inst);

  return MAI;
}

static MCInstPrinter *createYCoreMCInstPrinter(const Triple &T,
                                               unsigned SyntaxVariant,
                                               const MCAsmInfo &MAI,
                                               const MCInstrInfo &MII,
                                               const MCRegisterInfo &MRI) {
  return new YCoreInstPrinter(MAI, MII, MRI);
}

// Force static initialization.
extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeYCoreTargetMC() {
  // Register the MC asm info.
  RegisterMCAsmInfoFn X(getTheYCoreTarget(), createYCoreMCAsmInfo);

  // Register the MC instruction info.
  TargetRegistry::RegisterMCInstrInfo(getTheYCoreTarget(),
                                      createYCoreMCInstrInfo);

  // Register the MC register info.
  TargetRegistry::RegisterMCRegInfo(getTheYCoreTarget(),
                                    createYCoreMCRegisterInfo);

  // Register the MC subtarget info.
  TargetRegistry::RegisterMCSubtargetInfo(getTheYCoreTarget(),
                                          createYCoreMCSubtargetInfo);

  // Register the MCInstPrinter
  TargetRegistry::RegisterMCInstPrinter(getTheYCoreTarget(),
                                        createYCoreMCInstPrinter);
}
