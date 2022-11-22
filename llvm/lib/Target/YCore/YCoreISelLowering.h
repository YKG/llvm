//===-- YCoreISelLowering.h - YCore DAG Lowering Interface ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines the interfaces that YCore uses to lower LLVM code into a
// selection DAG.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_YCORE_YCOREISELLOWERING_H
#define LLVM_LIB_TARGET_YCORE_YCOREISELLOWERING_H

#include "YCore.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/TargetLowering.h"

namespace llvm {

  // Forward delcarations
  class YCoreSubtarget;

  namespace YCoreISD {
    enum NodeType : unsigned {
      // Start the numbering where the builtin ops and target ops leave off.
      FIRST_NUMBER = ISD::BUILTIN_OP_END,

      // Load word from stack
      LDWSP,

      // Store word to stack
      STWSP,

      // Corresponds to retsp instruction
      RETSP,

      // Corresponds to LADD instruction
      LADD,

      // Offset from frame pointer to the first (possible) on-stack argument
      FRAME_TO_ARGS_OFFSET,
    };
  }

  //===--------------------------------------------------------------------===//
  // TargetLowering Implementation
  //===--------------------------------------------------------------------===//
  class YCoreTargetLowering : public TargetLowering
  {
  public:
    explicit YCoreTargetLowering(const TargetMachine &TM,
                                 const YCoreSubtarget &Subtarget);

    using TargetLowering::isZExtFree;

    /// LowerOperation - Provide custom lowering hooks for some operations.
    SDValue LowerOperation(SDValue Op, SelectionDAG &DAG) const override;

    /// getTargetNodeName - This method returns the name of a target specific
    //  DAG node.
    const char *getTargetNodeName(unsigned Opcode) const override;

    /// If a physical register, this returns the register that receives the
    /// exception address on entry to an EH pad.
    Register
    getExceptionPointerRegister(const Constant *PersonalityFn) const override {
      return YCore::R0;
    }

    /// If a physical register, this returns the register that receives the
    /// exception typeid on entry to a landing pad.
    Register
    getExceptionSelectorRegister(const Constant *PersonalityFn) const override {
      return YCore::R1;
    }

  private:
    const TargetMachine &TM;
    const YCoreSubtarget &Subtarget;

    // Lower Operand helpers
    SDValue LowerCCCArguments(SDValue Chain, CallingConv::ID CallConv,
                              bool isVarArg,
                              const SmallVectorImpl<ISD::InputArg> &Ins,
                              const SDLoc &dl, SelectionDAG &DAG,
                              SmallVectorImpl<SDValue> &InVals) const;
//    SDValue getReturnAddressFrameIndex(SelectionDAG &DAG) const;
//
//    SDValue lowerLoadWordFromAlignedBasePlusOffset(const SDLoc &DL,
//                                                   SDValue Chain, SDValue Base,
//                                                   int64_t Offset,
//                                                   SelectionDAG &DAG) const;

    // Lower Operand specifics
    SDValue LowerLOAD(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerSTORE(SDValue Op, SelectionDAG &DAG) const;
//    SDValue ExpandADDSUB(SDNode *Op, SelectionDAG &DAG) const;

    SDValue PerformDAGCombine(SDNode *N, DAGCombinerInfo &DCI) const override;

    SDValue
    LowerFormalArguments(SDValue Chain, CallingConv::ID CallConv, bool isVarArg,
                         const SmallVectorImpl<ISD::InputArg> &Ins,
                         const SDLoc &dl, SelectionDAG &DAG,
                         SmallVectorImpl<SDValue> &InVals) const override;

    SDValue LowerReturn(SDValue Chain, CallingConv::ID CallConv, bool isVarArg,
                        const SmallVectorImpl<ISD::OutputArg> &Outs,
                        const SmallVectorImpl<SDValue> &OutVals,
                        const SDLoc &dl, SelectionDAG &DAG) const override;

    bool
      CanLowerReturn(CallingConv::ID CallConv, MachineFunction &MF,
                     bool isVarArg,
                     const SmallVectorImpl<ISD::OutputArg> &ArgsFlags,
                     LLVMContext &Context) const override;
    bool shouldInsertFencesForAtomic(const Instruction *I) const override {
      return true;
    }
  };
}

#endif
