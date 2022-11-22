//===-- YCoreTargetMachine.cpp - Define TargetMachine for YCore -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "YCoreTargetMachine.h"
#include "MCTargetDesc/YCoreMCTargetDesc.h"
#include "TargetInfo/YCoreTargetInfo.h"
#include "YCore.h"
#include "YCoreTargetObjectFile.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/CodeGen.h"

using namespace llvm;

static Reloc::Model getEffectiveRelocModel(Optional<Reloc::Model> RM) {
  return RM.getValueOr(Reloc::Static);
}

static CodeModel::Model
getEffectiveYCoreCodeModel(Optional<CodeModel::Model> CM) {
  return CodeModel::Small;
}

/// Create an ILP32 architecture model
///
YCoreTargetMachine::YCoreTargetMachine(const Target &T, const Triple &TT,
                                       StringRef CPU, StringRef FS,
                                       const TargetOptions &Options,
                                       Optional<Reloc::Model> RM,
                                       Optional<CodeModel::Model> CM,
                                       CodeGenOpt::Level OL, bool JIT)
    : LLVMTargetMachine(
          T, "e-m:e-p:32:32-i1:8:32-i8:8:32-i16:16:32-i64:32-f64:32-a:0:32-n32",
          TT, CPU, FS, Options, getEffectiveRelocModel(RM),
          getEffectiveYCoreCodeModel(CM), OL),
      TLOF(std::make_unique<YCoreTargetObjectFile>()),
      Subtarget(TT, std::string(CPU), std::string(FS), *this) {
  initAsmInfo();
}

YCoreTargetMachine::~YCoreTargetMachine() = default;

namespace {

/// YCore Code Generator Pass Configuration Options.
class YCorePassConfig : public TargetPassConfig {
public:
  YCorePassConfig(YCoreTargetMachine &TM, PassManagerBase &PM)
    : TargetPassConfig(TM, PM) {}

  YCoreTargetMachine &getYCoreTargetMachine() const {
    return getTM<YCoreTargetMachine>();
  }

  void addIRPasses() override;
  bool addPreISel() override;
  bool addInstSelector() override;
};

} // end anonymous namespace

TargetPassConfig *YCoreTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new YCorePassConfig(*this, PM);
}

void YCorePassConfig::addIRPasses() {
  addPass(createAtomicExpandPass());

  TargetPassConfig::addIRPasses();
}

bool YCorePassConfig::addPreISel() {
  return false;
}

bool YCorePassConfig::addInstSelector() {
  addPass(createYCoreISelDag(getYCoreTargetMachine(), getOptLevel()));
  return false;
}

// Force static initialization.
extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeYCoreTarget() {
  RegisterTargetMachine<YCoreTargetMachine> X(getTheYCoreTarget());
}
