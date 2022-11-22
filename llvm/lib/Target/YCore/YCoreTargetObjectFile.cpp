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
  BSSSectionLarge = Ctx.getELFSection(".dp.bss.large", ELF::SHT_NOBITS,
                                      ELF::SHF_ALLOC | ELF::SHF_WRITE |
                                          ELF::YCORE_SHF_DP_SECTION);
  DataSection = Ctx.getELFSection(".dp.data", ELF::SHT_PROGBITS,
                                  ELF::SHF_ALLOC | ELF::SHF_WRITE |
                                      ELF::YCORE_SHF_DP_SECTION);
  DataSectionLarge = Ctx.getELFSection(".dp.data.large", ELF::SHT_PROGBITS,
                                       ELF::SHF_ALLOC | ELF::SHF_WRITE |
                                           ELF::YCORE_SHF_DP_SECTION);
  DataRelROSection = Ctx.getELFSection(".dp.rodata", ELF::SHT_PROGBITS,
                                       ELF::SHF_ALLOC | ELF::SHF_WRITE |
                                           ELF::YCORE_SHF_DP_SECTION);
  DataRelROSectionLarge = Ctx.getELFSection(
      ".dp.rodata.large", ELF::SHT_PROGBITS,
      ELF::SHF_ALLOC | ELF::SHF_WRITE | ELF::YCORE_SHF_DP_SECTION);
  ReadOnlySection =
      Ctx.getELFSection(".cp.rodata", ELF::SHT_PROGBITS,
                        ELF::SHF_ALLOC | ELF::YCORE_SHF_CP_SECTION);
  ReadOnlySectionLarge =
      Ctx.getELFSection(".cp.rodata.large", ELF::SHT_PROGBITS,
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
  // TextSection       - see MObjectFileInfo.cpp
  // StaticCtorSection - see MObjectFileInfo.cpp
  // StaticDtorSection - see MObjectFileInfo.cpp
 }

static unsigned getYCoreSectionType(SectionKind K) {
  if (K.isBSS())
    return ELF::SHT_NOBITS;
  return ELF::SHT_PROGBITS;
}

MCSection *YCoreTargetObjectFile::SelectSectionForGlobal(
    const GlobalObject *GO, SectionKind Kind, const TargetMachine &TM) const {

  bool UseCPRel = GO->hasLocalLinkage();

  if (Kind.isText())                    return TextSection;
  if (UseCPRel) {
    if (Kind.isMergeable1ByteCString()) return CStringSection;
    if (Kind.isMergeableConst4())       return MergeableConst4Section;
    if (Kind.isMergeableConst8())       return MergeableConst8Section;
    if (Kind.isMergeableConst16())      return MergeableConst16Section;
  }
  Type *ObjType = GO->getValueType();
  auto &DL = GO->getParent()->getDataLayout();
  if (TM.getCodeModel() == CodeModel::Small || !ObjType->isSized() ||
      DL.getTypeAllocSize(ObjType) < CodeModelLargeSize) {
    if (Kind.isReadOnly())              return UseCPRel? ReadOnlySection
                                                       : DataRelROSection;
    if (Kind.isBSS() || Kind.isCommon())return BSSSection;
    if (Kind.isData())
      return DataSection;
    if (Kind.isReadOnlyWithRel())       return DataRelROSection;
  } else {
    if (Kind.isReadOnly())              return UseCPRel? ReadOnlySectionLarge
                                                       : DataRelROSectionLarge;
    if (Kind.isBSS() || Kind.isCommon())return BSSSectionLarge;
    if (Kind.isData())
      return DataSectionLarge;
    if (Kind.isReadOnlyWithRel())       return DataRelROSectionLarge;
  }

  assert((Kind.isThreadLocal() || Kind.isCommon()) && "Unknown section kind");
  report_fatal_error("Target does not support TLS or Common sections");
}
