//===-- YCoreISelLowering.cpp - YCore DAG Lowering Implementation ---------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the YCoreTargetLowering class.
//
//===----------------------------------------------------------------------===//

#include "YCoreISelLowering.h"
#include "YCore.h"
#include "YCoreMachineFunctionInfo.h"
#include "YCoreSubtarget.h"
#include "YCoreTargetMachine.h"
#include "YCoreTargetObjectFile.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineJumpTableInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/ValueTypes.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalAlias.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/IntrinsicsYCore.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/KnownBits.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm>

using namespace llvm;

#define DEBUG_TYPE "ycore-lower"

const char *YCoreTargetLowering::
getTargetNodeName(unsigned Opcode) const
{
  switch ((YCoreISD::NodeType)Opcode)
  {
    case YCoreISD::FIRST_NUMBER      : break;
    case YCoreISD::LDWSP             : return "YCoreISD::LDWSP";
    case YCoreISD::STWSP             : return "YCoreISD::STWSP";
    case YCoreISD::RETSP             : return "YCoreISD::RETSP";
    case YCoreISD::LADD              : return "YCoreISD::LADD";
    case YCoreISD::FRAME_TO_ARGS_OFFSET : return "YCoreISD::FRAME_TO_ARGS_OFFSET";
  }
  return nullptr;
}

YCoreTargetLowering::YCoreTargetLowering(const TargetMachine &TM,
                                         const YCoreSubtarget &Subtarget)
    : TargetLowering(TM), TM(TM), Subtarget(Subtarget) {

  // Set up the register classes.
  addRegisterClass(MVT::i32, &YCore::GRRegsRegClass);

  // Compute derived properties from the register classes
  computeRegisterProperties(Subtarget.getRegisterInfo());

  setStackPointerRegisterToSaveRestore(YCore::SP);

  setSchedulingPreference(Sched::Source);

  // Use i32 for setcc operations results (slt, sgt, ...).
  setBooleanContents(ZeroOrOneBooleanContent);
  setBooleanVectorContents(ZeroOrOneBooleanContent); // FIXME: Is this correct?

  // 64bit
  setOperationAction(ISD::ADD, MVT::i64, Custom);

  // Custom expand misaligned loads / stores.
  setOperationAction(ISD::LOAD, MVT::i32, Custom);
  setOperationAction(ISD::STORE, MVT::i32, Custom);

  MaxStoresPerMemset = MaxStoresPerMemsetOptSize = 4;
  MaxStoresPerMemmove = MaxStoresPerMemmoveOptSize
    = MaxStoresPerMemcpy = MaxStoresPerMemcpyOptSize = 2;

  // We have target-specific dag combine patterns for the following nodes:
  setTargetDAGCombine(ISD::STORE);
  setTargetDAGCombine(ISD::ADD);

  setMinFunctionAlignment(Align(2));
  setPrefFunctionAlignment(Align(4));
}

SDValue YCoreTargetLowering::
LowerOperation(SDValue Op, SelectionDAG &DAG) const {
  switch (Op.getOpcode())
  {
  case ISD::LOAD:               return LowerLOAD(Op, DAG);
  case ISD::STORE:              return LowerSTORE(Op, DAG);
  default:
    llvm_unreachable("unimplemented operand");
  }
}

SDValue YCoreTargetLowering::LowerLOAD(SDValue Op, SelectionDAG &DAG) const {
  const TargetLowering &TLI = DAG.getTargetLoweringInfo();
  LLVMContext &Context = *DAG.getContext();
  LoadSDNode *LD = cast<LoadSDNode>(Op);
  assert(LD->getExtensionType() == ISD::NON_EXTLOAD &&
         "Unexpected extension type");
  assert(LD->getMemoryVT() == MVT::i32 && "Unexpected load EVT");

  if (allowsMemoryAccessForAlignment(Context, DAG.getDataLayout(),
                                     LD->getMemoryVT(), *LD->getMemOperand())){
    return SDValue();
  }
  llvm_unreachable("unimplemented");
}

SDValue YCoreTargetLowering::LowerSTORE(SDValue Op, SelectionDAG &DAG) const {
  LLVMContext &Context = *DAG.getContext();
  StoreSDNode *ST = cast<StoreSDNode>(Op);
  assert(!ST->isTruncatingStore() && "Unexpected store type");
  assert(ST->getMemoryVT() == MVT::i32 && "Unexpected store EVT");

  if (allowsMemoryAccessForAlignment(Context, DAG.getDataLayout(),
                                     ST->getMemoryVT(), *ST->getMemOperand()))
    return SDValue();
  llvm_unreachable("unimplemented");
}

//===----------------------------------------------------------------------===//
//                      Calling Convention Implementation
//===----------------------------------------------------------------------===//

#include "YCoreGenCallingConv.inc"

//===----------------------------------------------------------------------===//
//             Formal Arguments Calling Convention Implementation
//===----------------------------------------------------------------------===//

namespace {
  struct ArgDataPair { SDValue SDV; ISD::ArgFlagsTy Flags; };
}

/// YCore formal arguments implementation
SDValue YCoreTargetLowering::LowerFormalArguments(
    SDValue Chain, CallingConv::ID CallConv, bool isVarArg,
    const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &dl,
    SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const {
  switch (CallConv)
  {
    default:
      report_fatal_error("Unsupported calling convention");
    case CallingConv::C:
    case CallingConv::Fast:
      return LowerCCCArguments(Chain, CallConv, isVarArg,
                               Ins, dl, DAG, InVals);
  }
}

/// LowerCCCArguments - transform physical registers into
/// virtual registers and generate load operations for
/// arguments places on the stack.
/// TODO: sret
SDValue YCoreTargetLowering::LowerCCCArguments(
    SDValue Chain, CallingConv::ID CallConv, bool isVarArg,
    const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &dl,
    SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const {
  MachineFunction &MF = DAG.getMachineFunction();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  MachineRegisterInfo &RegInfo = MF.getRegInfo();
  YCoreFunctionInfo *XFI = MF.getInfo<YCoreFunctionInfo>();

  // Assign locations to all of the incoming arguments.
  SmallVector<CCValAssign, 16> ArgLocs;
  CCState CCInfo(CallConv, isVarArg, DAG.getMachineFunction(), ArgLocs,
                 *DAG.getContext());

  CCInfo.AnalyzeFormalArguments(Ins, CC_YCore);

  unsigned StackSlotSize = YCoreFrameLowering::stackSlotSize();

  unsigned LRSaveSize = StackSlotSize;

  if (!isVarArg)
    XFI->setReturnStackOffset(CCInfo.getNextStackOffset() + LRSaveSize);

  // All getCopyFromReg ops must precede any getMemcpys to prevent the
  // scheduler clobbering a register before it has been copied.
  // The stages are:
  // 1. CopyFromReg (and load) arg & vararg registers.
  // 2. Chain CopyFromReg nodes into a TokenFactor.
  // 3. Memcpy 'byVal' args & push final InVals.
  // 4. Chain mem ops nodes into a TokenFactor.
  SmallVector<SDValue, 4> CFRegNode;
  SmallVector<ArgDataPair, 4> ArgData;
  SmallVector<SDValue, 4> MemOps;

  // 1a. CopyFromReg (and load) arg registers.
  for (unsigned i = 0, e = ArgLocs.size(); i != e; ++i) {

    CCValAssign &VA = ArgLocs[i];
    SDValue ArgIn;

    if (VA.isRegLoc()) {
      // Arguments passed in registers
      EVT RegVT = VA.getLocVT();
      switch (RegVT.getSimpleVT().SimpleTy) {
      default:
        {
#ifndef NDEBUG
          errs() << "LowerFormalArguments Unhandled argument type: "
                 << RegVT.getEVTString() << "\n";
#endif
          llvm_unreachable(nullptr);
        }
      case MVT::i32:
        Register VReg = RegInfo.createVirtualRegister(&YCore::GRRegsRegClass);
        RegInfo.addLiveIn(VA.getLocReg(), VReg);
        ArgIn = DAG.getCopyFromReg(Chain, dl, VReg, RegVT);
        CFRegNode.push_back(ArgIn.getValue(ArgIn->getNumValues() - 1));
      }
    } else {
      llvm_unreachable("unimplemented");
    }
    const ArgDataPair ADP = { ArgIn, Ins[i].Flags };
    ArgData.push_back(ADP);
  }

  // 2. chain CopyFromReg nodes into a TokenFactor.
  if (!CFRegNode.empty())
    Chain = DAG.getNode(ISD::TokenFactor, dl, MVT::Other, CFRegNode);

  // 3. Memcpy 'byVal' args & push final InVals.
  // Aggregates passed "byVal" need to be copied by the callee.
  // The callee will use a pointer to this copy, rather than the original
  // pointer.
  for (SmallVectorImpl<ArgDataPair>::const_iterator ArgDI = ArgData.begin(),
                                                    ArgDE = ArgData.end();
       ArgDI != ArgDE; ++ArgDI) {

    if (ArgDI->Flags.isByVal() && ArgDI->Flags.getByValSize()) {
      llvm_unreachable("unimplemented");
    } else {
      InVals.push_back(ArgDI->SDV);
    }
  }

  return Chain;
}

//===----------------------------------------------------------------------===//
//               Return Value Calling Convention Implementation
//===----------------------------------------------------------------------===//

bool YCoreTargetLowering::
CanLowerReturn(CallingConv::ID CallConv, MachineFunction &MF,
               bool isVarArg,
               const SmallVectorImpl<ISD::OutputArg> &Outs,
               LLVMContext &Context) const {
  SmallVector<CCValAssign, 16> RVLocs;
  CCState CCInfo(CallConv, isVarArg, MF, RVLocs, Context);
  if (!CCInfo.CheckReturn(Outs, RetCC_YCore))
    return false;
  if (CCInfo.getNextStackOffset() != 0 && isVarArg)
    return false;
  return true;
}

SDValue
YCoreTargetLowering::LowerReturn(SDValue Chain, CallingConv::ID CallConv,
                                 bool isVarArg,
                                 const SmallVectorImpl<ISD::OutputArg> &Outs,
                                 const SmallVectorImpl<SDValue> &OutVals,
                                 const SDLoc &dl, SelectionDAG &DAG) const {

  YCoreFunctionInfo *XFI =
    DAG.getMachineFunction().getInfo<YCoreFunctionInfo>();
  MachineFrameInfo &MFI = DAG.getMachineFunction().getFrameInfo();

  // CCValAssign - represent the assignment of
  // the return value to a location
  SmallVector<CCValAssign, 16> RVLocs;

  // CCState - Info about the registers and stack slot.
  CCState CCInfo(CallConv, isVarArg, DAG.getMachineFunction(), RVLocs,
                 *DAG.getContext());

  // Analyze return values.
  if (!isVarArg)
    CCInfo.AllocateStack(XFI->getReturnStackOffset(), Align(4));

  CCInfo.AnalyzeReturn(Outs, RetCC_YCore);

  SDValue Flag;
  SmallVector<SDValue, 4> RetOps(1, Chain);

  // Return on YCore is always a "retsp 0"
  RetOps.push_back(DAG.getConstant(0, dl, MVT::i32));

  SmallVector<SDValue, 4> MemOpChains;
  // Handle return values that must be copied to memory.
  for (unsigned i = 0, e = RVLocs.size(); i != e; ++i) {
    CCValAssign &VA = RVLocs[i];
    if (VA.isRegLoc())
      continue;
    llvm_unreachable("unimplemented");
  }

  // Transform all store nodes into one single node because
  // all stores are independent of each other.
  if (!MemOpChains.empty())
    Chain = DAG.getNode(ISD::TokenFactor, dl, MVT::Other, MemOpChains);

  // Now handle return values copied to registers.
  for (unsigned i = 0, e = RVLocs.size(); i != e; ++i) {
    CCValAssign &VA = RVLocs[i];
    if (!VA.isRegLoc())
      continue;
    // Copy the result values into the output registers.
    Chain = DAG.getCopyToReg(Chain, dl, VA.getLocReg(), OutVals[i], Flag);

    // guarantee that all emitted copies are
    // stuck together, avoiding something bad
    Flag = Chain.getValue(1);
    RetOps.push_back(DAG.getRegister(VA.getLocReg(), VA.getLocVT()));
  }

  RetOps[0] = Chain;  // Update chain.

  // Add the flag if we have it.
  if (Flag.getNode())
    RetOps.push_back(Flag);

  return DAG.getNode(YCoreISD::RETSP, dl, MVT::Other, RetOps);
}

//===----------------------------------------------------------------------===//
// Target Optimization Hooks
//===----------------------------------------------------------------------===//

SDValue YCoreTargetLowering::PerformDAGCombine(SDNode *N,
                                             DAGCombinerInfo &DCI) const {
  SelectionDAG &DAG = DCI.DAG;
  SDLoc dl(N);
  switch (N->getOpcode()) {
  default: break;
  case ISD::ADD: {
    // Fold 32 bit expressions such as add(add(mul(x,y),a),b) ->
    // lmul(x, y, a, b). The high result of lmul will be ignored.
    // This is only profitable if the intermediate results are unused
    // elsewhere.
    SDValue Mul0, Mul1, Addend0, Addend1;
    APInt HighMask = APInt::getHighBitsSet(64, 32);
  }
  break;
  case ISD::STORE: {
    // Replace unaligned store of unaligned load with memmove.
    StoreSDNode *ST = cast<StoreSDNode>(N);
    if (!DCI.isBeforeLegalize() ||
        allowsMemoryAccessForAlignment(*DAG.getContext(), DAG.getDataLayout(),
                                       ST->getMemoryVT(),
                                       *ST->getMemOperand()) ||
        ST->isVolatile() || ST->isIndexed()) {
      break;
    }
    llvm_unreachable("unimplemented");
    break;
  }
  }
  return SDValue();
}
