//===-- YCoreTargetObjectFile.cpp - YCore object files --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "YCoreTargetObjectFile.h"
#include "YCoreSubtarget.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCSectionELF.h"
#include "llvm/Target/TargetMachine.h"

using namespace llvm;


void YCoreTargetObjectFile::Initialize(MCContext &Ctx, const TargetMachine &TM){
  TargetLoweringObjectFileELF::Initialize(Ctx, TM);

  BSSSection = Ctx.getELFSection(".dp.bss", ELF::SHT_NOBITS,
                                 ELF::SHF_ALLOC | ELF::SHF_WRITE |
                                     ELF::YCORE_SHF_DP_SECTION);
  DataSection = Ctx.getELFSection(".dp.data", ELF::SHT_PROGBITS,
                                  ELF::SHF_ALLOC | ELF::SHF_WRITE |
                                      ELF::YCORE_SHF_DP_SECTION);
  DataRelROSection = Ctx.getELFSection(".dp.rodata", ELF::SHT_PROGBITS,
                                       ELF::SHF_ALLOC | ELF::SHF_WRITE |
                                           ELF::YCORE_SHF_DP_SECTION);
  ReadOnlySection =
      Ctx.getELFSection(".cp.rodata", ELF::SHT_PROGBITS,
                        ELF::SHF_ALLOC | ELF::YCORE_SHF_CP_SECTION);
  MergeableConst4Section = Ctx.getELFSection(
      ".cp.rodata.cst4", ELF::SHT_PROGBITS,
      ELF::SHF_ALLOC | ELF::SHF_MERGE | ELF::YCORE_SHF_CP_SECTION, 4);
  MergeableConst8Section = Ctx.getELFSection(
      ".cp.rodata.cst8", ELF::SHT_PROGBITS,
      ELF::SHF_ALLOC | ELF::SHF_MERGE | ELF::YCORE_SHF_CP_SECTION, 8);
  MergeableConst16Section = Ctx.getELFSection(
      ".cp.rodata.cst16", ELF::SHT_PROGBITS,
      ELF::SHF_ALLOC | ELF::SHF_MERGE | ELF::YCORE_SHF_CP_SECTION, 16);
  CStringSection =
      Ctx.getELFSection(".cp.rodata.string", ELF::SHT_PROGBITS,
                        ELF::SHF_ALLOC | ELF::SHF_MERGE | ELF::SHF_STRINGS |
                            ELF::YCORE_SHF_CP_SECTION);
 }

MCSection *YCoreTargetObjectFile::SelectSectionForGlobal(
    const GlobalObject *GO, SectionKind Kind, const TargetMachine &TM) const {

  if (Kind.isText())                    return TextSection;
}
