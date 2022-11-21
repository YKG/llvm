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

  // YCore does not have the NodeTypes below.
  setOperationAction(ISD::BR_CC,     MVT::i32,   Expand);
  setOperationAction(ISD::SELECT_CC, MVT::i32,   Expand);

  // 64bit
  setOperationAction(ISD::ADD, MVT::i64, Custom);
  setOperationAction(ISD::SUB, MVT::i64, Custom);
  setOperationAction(ISD::SMUL_LOHI, MVT::i32, Custom);
  setOperationAction(ISD::UMUL_LOHI, MVT::i32, Custom);
  setOperationAction(ISD::MULHS, MVT::i32, Expand);
  setOperationAction(ISD::MULHU, MVT::i32, Expand);
  setOperationAction(ISD::SHL_PARTS, MVT::i32, Expand);
  setOperationAction(ISD::SRA_PARTS, MVT::i32, Expand);
  setOperationAction(ISD::SRL_PARTS, MVT::i32, Expand);

  // Bit Manipulation
  setOperationAction(ISD::CTPOP, MVT::i32, Expand);
  setOperationAction(ISD::ROTL , MVT::i32, Expand);
  setOperationAction(ISD::ROTR , MVT::i32, Expand);
  setOperationAction(ISD::BITREVERSE , MVT::i32, Legal);

  setOperationAction(ISD::TRAP, MVT::Other, Legal);

  // Jump tables.
  setOperationAction(ISD::BR_JT, MVT::Other, Custom);

  setOperationAction(ISD::GlobalAddress, MVT::i32,   Custom);
  setOperationAction(ISD::BlockAddress, MVT::i32 , Custom);

  // Conversion of i64 -> double produces constantpool nodes
  setOperationAction(ISD::ConstantPool, MVT::i32,   Custom);

  // Loads
  for (MVT VT : MVT::integer_valuetypes()) {
    setLoadExtAction(ISD::EXTLOAD, VT, MVT::i1, Promote);
    setLoadExtAction(ISD::ZEXTLOAD, VT, MVT::i1, Promote);
    setLoadExtAction(ISD::SEXTLOAD, VT, MVT::i1, Promote);

    setLoadExtAction(ISD::SEXTLOAD, VT, MVT::i8, Expand);
    setLoadExtAction(ISD::ZEXTLOAD, VT, MVT::i16, Expand);
  }

  // Custom expand misaligned loads / stores.
  setOperationAction(ISD::LOAD, MVT::i32, Custom);
  setOperationAction(ISD::STORE, MVT::i32, Custom);

  // Varargs
  setOperationAction(ISD::VAEND, MVT::Other, Expand);
  setOperationAction(ISD::VACOPY, MVT::Other, Expand);
  setOperationAction(ISD::VAARG, MVT::Other, Custom);
  setOperationAction(ISD::VASTART, MVT::Other, Custom);

  // Dynamic stack
  setOperationAction(ISD::STACKSAVE, MVT::Other, Expand);
  setOperationAction(ISD::STACKRESTORE, MVT::Other, Expand);
  setOperationAction(ISD::DYNAMIC_STACKALLOC, MVT::i32, Expand);

  // Exception handling
  setOperationAction(ISD::EH_RETURN, MVT::Other, Custom);
  setOperationAction(ISD::FRAME_TO_ARGS_OFFSET, MVT::i32, Custom);

  // Atomic operations
  // We request a fence for ATOMIC_* instructions, to reduce them to Monotonic.
  // As we are always Sequential Consistent, an ATOMIC_FENCE becomes a no OP.
  setOperationAction(ISD::ATOMIC_FENCE, MVT::Other, Custom);
  setOperationAction(ISD::ATOMIC_LOAD, MVT::i32, Custom);
  setOperationAction(ISD::ATOMIC_STORE, MVT::i32, Custom);

  // TRAMPOLINE is custom lowered.
  setOperationAction(ISD::INIT_TRAMPOLINE, MVT::Other, Custom);
  setOperationAction(ISD::ADJUST_TRAMPOLINE, MVT::Other, Custom);

  // We want to custom lower some of our intrinsics.
  setOperationAction(ISD::INTRINSIC_WO_CHAIN, MVT::Other, Custom);

  MaxStoresPerMemset = MaxStoresPerMemsetOptSize = 4;
  MaxStoresPerMemmove = MaxStoresPerMemmoveOptSize
    = MaxStoresPerMemcpy = MaxStoresPerMemcpyOptSize = 2;

  // We have target-specific dag combine patterns for the following nodes:
  setTargetDAGCombine(ISD::STORE);
  setTargetDAGCombine(ISD::ADD);
  setTargetDAGCombine(ISD::INTRINSIC_VOID);
  setTargetDAGCombine(ISD::INTRINSIC_W_CHAIN);

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
                                     LD->getMemoryVT(), *LD->getMemOperand()))
    return SDValue();

  SDValue Chain = LD->getChain();
  SDValue BasePtr = LD->getBasePtr();
  SDLoc DL(Op);

  if (LD->getAlignment() == 2) {
    SDValue Low = DAG.getExtLoad(ISD::ZEXTLOAD, DL, MVT::i32, Chain, BasePtr,
                                 LD->getPointerInfo(), MVT::i16, Align(2),
                                 LD->getMemOperand()->getFlags());
    SDValue HighAddr = DAG.getNode(ISD::ADD, DL, MVT::i32, BasePtr,
                                   DAG.getConstant(2, DL, MVT::i32));
    SDValue High =
        DAG.getExtLoad(ISD::EXTLOAD, DL, MVT::i32, Chain, HighAddr,
                       LD->getPointerInfo().getWithOffset(2), MVT::i16,
                       Align(2), LD->getMemOperand()->getFlags());
    SDValue HighShifted = DAG.getNode(ISD::SHL, DL, MVT::i32, High,
                                      DAG.getConstant(16, DL, MVT::i32));
    SDValue Result = DAG.getNode(ISD::OR, DL, MVT::i32, Low, HighShifted);
    Chain = DAG.getNode(ISD::TokenFactor, DL, MVT::Other, Low.getValue(1),
                             High.getValue(1));
    SDValue Ops[] = { Result, Chain };
    return DAG.getMergeValues(Ops, DL);
  }

  // Lower to a call to __misaligned_load(BasePtr).
  Type *IntPtrTy = DAG.getDataLayout().getIntPtrType(Context);
  TargetLowering::ArgListTy Args;
  TargetLowering::ArgListEntry Entry;

  Entry.Ty = IntPtrTy;
  Entry.Node = BasePtr;
  Args.push_back(Entry);

  TargetLowering::CallLoweringInfo CLI(DAG);
  CLI.setDebugLoc(DL).setChain(Chain).setLibCallee(
      CallingConv::C, IntPtrTy,
      DAG.getExternalSymbol("__misaligned_load",
                            getPointerTy(DAG.getDataLayout())),
      std::move(Args));

  std::pair<SDValue, SDValue> CallResult = LowerCallTo(CLI);
  SDValue Ops[] = { CallResult.first, CallResult.second };
  return DAG.getMergeValues(Ops, DL);
}

SDValue YCoreTargetLowering::LowerSTORE(SDValue Op, SelectionDAG &DAG) const {
  LLVMContext &Context = *DAG.getContext();
  StoreSDNode *ST = cast<StoreSDNode>(Op);
  assert(!ST->isTruncatingStore() && "Unexpected store type");
  assert(ST->getMemoryVT() == MVT::i32 && "Unexpected store EVT");

  if (allowsMemoryAccessForAlignment(Context, DAG.getDataLayout(),
                                     ST->getMemoryVT(), *ST->getMemOperand()))
    return SDValue();

  SDValue Chain = ST->getChain();
  SDValue BasePtr = ST->getBasePtr();
  SDValue Value = ST->getValue();
  SDLoc dl(Op);

  if (ST->getAlignment() == 2) {
    SDValue Low = Value;
    SDValue High = DAG.getNode(ISD::SRL, dl, MVT::i32, Value,
                               DAG.getConstant(16, dl, MVT::i32));
    SDValue StoreLow =
        DAG.getTruncStore(Chain, dl, Low, BasePtr, ST->getPointerInfo(),
                          MVT::i16, Align(2), ST->getMemOperand()->getFlags());
    SDValue HighAddr = DAG.getNode(ISD::ADD, dl, MVT::i32, BasePtr,
                                   DAG.getConstant(2, dl, MVT::i32));
    SDValue StoreHigh = DAG.getTruncStore(
        Chain, dl, High, HighAddr, ST->getPointerInfo().getWithOffset(2),
        MVT::i16, Align(2), ST->getMemOperand()->getFlags());
    return DAG.getNode(ISD::TokenFactor, dl, MVT::Other, StoreLow, StoreHigh);
  }

  // Lower to a call to __misaligned_store(BasePtr, Value).
  Type *IntPtrTy = DAG.getDataLayout().getIntPtrType(Context);
  TargetLowering::ArgListTy Args;
  TargetLowering::ArgListEntry Entry;

  Entry.Ty = IntPtrTy;
  Entry.Node = BasePtr;
  Args.push_back(Entry);

  Entry.Node = Value;
  Args.push_back(Entry);

  TargetLowering::CallLoweringInfo CLI(DAG);
  CLI.setDebugLoc(dl).setChain(Chain).setCallee(
      CallingConv::C, Type::getVoidTy(Context),
      DAG.getExternalSymbol("__misaligned_store",
                            getPointerTy(DAG.getDataLayout())),
      std::move(Args));

  std::pair<SDValue, SDValue> CallResult = LowerCallTo(CLI);
  return CallResult.second;
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
      // Only arguments passed on the stack should make it here. 
      assert(VA.isMemLoc());
      // Load the argument to a virtual register
      unsigned ObjSize = VA.getLocVT().getSizeInBits()/8;
      if (ObjSize > StackSlotSize) {
        errs() << "LowerFormalArguments Unhandled argument type: "
               << EVT(VA.getLocVT()).getEVTString()
               << "\n";
      }
      // Create the frame index object for this incoming parameter...
      int FI = MFI.CreateFixedObject(ObjSize,
                                     LRSaveSize + VA.getLocMemOffset(),
                                     true);

      // Create the SelectionDAG nodes corresponding to a load
      //from this parameter
      SDValue FIN = DAG.getFrameIndex(FI, MVT::i32);
      ArgIn = DAG.getLoad(VA.getLocVT(), dl, Chain, FIN,
                          MachinePointerInfo::getFixedStack(MF, FI));
    }
    const ArgDataPair ADP = { ArgIn, Ins[i].Flags };
    ArgData.push_back(ADP);
  }

  // 1b. CopyFromReg vararg registers.
  if (isVarArg) {
    // Argument registers
    static const MCPhysReg ArgRegs[] = {
      YCore::R0, YCore::R1, YCore::R2, YCore::R3
    };
    YCoreFunctionInfo *XFI = MF.getInfo<YCoreFunctionInfo>();
    unsigned FirstVAReg = CCInfo.getFirstUnallocated(ArgRegs);
    if (FirstVAReg < array_lengthof(ArgRegs)) {
      int offset = 0;
      // Save remaining registers, storing higher register numbers at a higher
      // address
      for (int i = array_lengthof(ArgRegs) - 1; i >= (int)FirstVAReg; --i) {
        // Create a stack slot
        int FI = MFI.CreateFixedObject(4, offset, true);
        if (i == (int)FirstVAReg) {
          XFI->setVarArgsFrameIndex(FI);
        }
        offset -= StackSlotSize;
        SDValue FIN = DAG.getFrameIndex(FI, MVT::i32);
        // Move argument from phys reg -> virt reg
        Register VReg = RegInfo.createVirtualRegister(&YCore::GRRegsRegClass);
        RegInfo.addLiveIn(ArgRegs[i], VReg);
        SDValue Val = DAG.getCopyFromReg(Chain, dl, VReg, MVT::i32);
        CFRegNode.push_back(Val.getValue(Val->getNumValues() - 1));
        // Move argument from virt reg -> stack
        SDValue Store =
            DAG.getStore(Val.getValue(1), dl, Val, FIN, MachinePointerInfo());
        MemOps.push_back(Store);
      }
    } else {
      // This will point to the next argument passed via stack.
      XFI->setVarArgsFrameIndex(
        MFI.CreateFixedObject(4, LRSaveSize + CCInfo.getNextStackOffset(),
                              true));
    }
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
      unsigned Size = ArgDI->Flags.getByValSize();
      Align Alignment =
          std::max(Align(StackSlotSize), ArgDI->Flags.getNonZeroByValAlign());
      // Create a new object on the stack and copy the pointee into it.
      int FI = MFI.CreateStackObject(Size, Alignment, false);
      SDValue FIN = DAG.getFrameIndex(FI, MVT::i32);
      InVals.push_back(FIN);
      MemOps.push_back(DAG.getMemcpy(
          Chain, dl, FIN, ArgDI->SDV, DAG.getConstant(Size, dl, MVT::i32),
          Alignment, false, false, false, MachinePointerInfo(),
          MachinePointerInfo()));
    } else {
      InVals.push_back(ArgDI->SDV);
    }
  }

  // 4, chain mem ops nodes into a TokenFactor.
  if (!MemOps.empty()) {
    MemOps.push_back(Chain);
    Chain = DAG.getNode(ISD::TokenFactor, dl, MVT::Other, MemOps);
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
    assert(VA.isMemLoc());
    if (isVarArg) {
      report_fatal_error("Can't return value from vararg function in memory");
    }

    int Offset = VA.getLocMemOffset();
    unsigned ObjSize = VA.getLocVT().getSizeInBits() / 8;
    // Create the frame index object for the memory location.
    int FI = MFI.CreateFixedObject(ObjSize, Offset, false);

    // Create a SelectionDAG node corresponding to a store
    // to this memory location.
    SDValue FIN = DAG.getFrameIndex(FI, MVT::i32);
    MemOpChains.push_back(DAG.getStore(
        Chain, dl, OutVals[i], FIN,
        MachinePointerInfo::getFixedStack(DAG.getMachineFunction(), FI)));
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
  case ISD::INTRINSIC_VOID:
    switch (cast<ConstantSDNode>(N->getOperand(1))->getZExtValue()) {
    case Intrinsic::ycore_outt:
    case Intrinsic::ycore_outct:
    case Intrinsic::ycore_chkct: {
      SDValue OutVal = N->getOperand(3);
      // These instructions ignore the high bits.
      if (OutVal.hasOneUse()) {
        unsigned BitWidth = OutVal.getValueSizeInBits();
        APInt DemandedMask = APInt::getLowBitsSet(BitWidth, 8);
        KnownBits Known;
        TargetLowering::TargetLoweringOpt TLO(DAG, !DCI.isBeforeLegalize(),
                                              !DCI.isBeforeLegalizeOps());
        const TargetLowering &TLI = DAG.getTargetLoweringInfo();
        if (TLI.ShrinkDemandedConstant(OutVal, DemandedMask, TLO) ||
            TLI.SimplifyDemandedBits(OutVal, DemandedMask, Known, TLO))
          DCI.CommitTargetLoweringOpt(TLO);
      }
      break;
    }
    case Intrinsic::ycore_setpt: {
      SDValue Time = N->getOperand(3);
      // This instruction ignores the high bits.
      if (Time.hasOneUse()) {
        unsigned BitWidth = Time.getValueSizeInBits();
        APInt DemandedMask = APInt::getLowBitsSet(BitWidth, 16);
        KnownBits Known;
        TargetLowering::TargetLoweringOpt TLO(DAG, !DCI.isBeforeLegalize(),
                                              !DCI.isBeforeLegalizeOps());
        const TargetLowering &TLI = DAG.getTargetLoweringInfo();
        if (TLI.ShrinkDemandedConstant(Time, DemandedMask, TLO) ||
            TLI.SimplifyDemandedBits(Time, DemandedMask, Known, TLO))
          DCI.CommitTargetLoweringOpt(TLO);
      }
      break;
    }
    }
    break;
  case YCoreISD::LADD: {
    SDValue N0 = N->getOperand(0);
    SDValue N1 = N->getOperand(1);
    SDValue N2 = N->getOperand(2);
    ConstantSDNode *N0C = dyn_cast<ConstantSDNode>(N0);
    ConstantSDNode *N1C = dyn_cast<ConstantSDNode>(N1);
    EVT VT = N0.getValueType();

    // canonicalize constant to RHS
    if (N0C && !N1C)
      return DAG.getNode(YCoreISD::LADD, dl, DAG.getVTList(VT, VT), N1, N0, N2);

    // fold (ladd 0, 0, x) -> 0, x & 1
    if (N0C && N0C->isZero() && N1C && N1C->isZero()) {
      SDValue Carry = DAG.getConstant(0, dl, VT);
      SDValue Result = DAG.getNode(ISD::AND, dl, VT, N2,
                                   DAG.getConstant(1, dl, VT));
      SDValue Ops[] = { Result, Carry };
      return DAG.getMergeValues(Ops, dl);
    }

    // fold (ladd x, 0, y) -> 0, add x, y iff carry is unused and y has only the
    // low bit set
    if (N1C && N1C->isZero() && N->hasNUsesOfValue(0, 1)) {
      APInt Mask = APInt::getHighBitsSet(VT.getSizeInBits(),
                                         VT.getSizeInBits() - 1);
      KnownBits Known = DAG.computeKnownBits(N2);
      if ((Known.Zero & Mask) == Mask) {
        SDValue Carry = DAG.getConstant(0, dl, VT);
        SDValue Result = DAG.getNode(ISD::ADD, dl, VT, N0, N2);
        SDValue Ops[] = { Result, Carry };
        return DAG.getMergeValues(Ops, dl);
      }
    }
  }
  break;
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
    SDValue Chain = ST->getChain();

    unsigned StoreBits = ST->getMemoryVT().getStoreSizeInBits();
    assert((StoreBits % 8) == 0 &&
           "Store size in bits must be a multiple of 8");
    unsigned Alignment = ST->getAlignment();

    if (LoadSDNode *LD = dyn_cast<LoadSDNode>(ST->getValue())) {
      if (LD->hasNUsesOfValue(1, 0) && ST->getMemoryVT() == LD->getMemoryVT() &&
        LD->getAlignment() == Alignment &&
        !LD->isVolatile() && !LD->isIndexed() &&
        Chain.reachesChainWithoutSideEffects(SDValue(LD, 1))) {
        bool isTail = isInTailCallPosition(DAG, ST, Chain);
        return DAG.getMemmove(Chain, dl, ST->getBasePtr(), LD->getBasePtr(),
                              DAG.getConstant(StoreBits / 8, dl, MVT::i32),
                              Align(Alignment), false, isTail,
                              ST->getPointerInfo(), LD->getPointerInfo());
      }
    }
    break;
  }
  }
  return SDValue();
}
