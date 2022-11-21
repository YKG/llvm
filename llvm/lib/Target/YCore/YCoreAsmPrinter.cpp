//===-- YCoreAsmPrinter.cpp - YCore LLVM assembly writer ------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains a printer that converts from our internal representation
// of machine-dependent LLVM code to the XAS-format YCore assembly language.
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/YCoreInstPrinter.h"
#include "TargetInfo/YCoreTargetInfo.h"
#include "YCore.h"
#include "YCoreInstrInfo.h"
#include "YCoreMCInstLower.h"
#include "YCoreSubtarget.h"
#include "YCoreTargetMachine.h"
#include "YCoreTargetStreamer.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineJumpTableInfo.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Mangler.h"
#include "llvm/IR/Module.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSymbolELF.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetLoweringObjectFile.h"
#include <algorithm>
#include <cctype>
using namespace llvm;

#define DEBUG_TYPE "asm-printer"

namespace {
  class YCoreAsmPrinter : public AsmPrinter {
    YCoreMCInstLower MCInstLowering;
    YCoreTargetStreamer &getTargetStreamer();

  public:
    explicit YCoreAsmPrinter(TargetMachine &TM,
                             std::unique_ptr<MCStreamer> Streamer)
        : AsmPrinter(TM, std::move(Streamer)), MCInstLowering(*this) {}

    StringRef getPassName() const override { return "YCore Assembly Printer"; }

    void emitFunctionEntryLabel() override;
    void emitInstruction(const MachineInstr *MI) override;
    void emitFunctionBodyStart() override;
    void emitFunctionBodyEnd() override;
  };
} // end of anonymous namespace

YCoreTargetStreamer &YCoreAsmPrinter::getTargetStreamer() {
  return static_cast<YCoreTargetStreamer&>(*OutStreamer->getTargetStreamer());
}

void YCoreAsmPrinter::emitFunctionBodyStart() {
  MCInstLowering.Initialize(&MF->getContext());
}

/// EmitFunctionBodyEnd - Targets can override this to emit stuff after
/// the last basic block in the function.
void YCoreAsmPrinter::emitFunctionBodyEnd() {
  // Emit function end directives
  getTargetStreamer().emitCCBottomFunction(CurrentFnSym->getName());
}

void YCoreAsmPrinter::emitFunctionEntryLabel() {
  // Mark the start of the function
  getTargetStreamer().emitCCTopFunction(CurrentFnSym->getName());
  OutStreamer->emitLabel(CurrentFnSym);
}

void YCoreAsmPrinter::emitInstruction(const MachineInstr *MI) {
  SmallString<128> Str;
  raw_svector_ostream O(Str);
  MCInst TmpInst;
  MCInstLowering.Lower(MI, TmpInst);

  EmitToStreamer(*OutStreamer, TmpInst);
}

// Force static initialization.
extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeYCoreAsmPrinter() {
  RegisterAsmPrinter<YCoreAsmPrinter> X(getTheYCoreTarget());
}
