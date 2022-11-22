//===-- YCoreInstrInfo.td - Target Description for YCore ---*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file describes the YCore instructions in TableGen format.
//
//===----------------------------------------------------------------------===//

// Uses of CP, DP are not currently reflected in the patterns, since
// having a physical register as an operand prevents loop hoisting and
// since the value of these registers never changes during the life of the
// function.

//===----------------------------------------------------------------------===//
// Instruction format superclass.
//===----------------------------------------------------------------------===//

include "YCoreInstrFormats.td"

//===----------------------------------------------------------------------===//
// YCore specific DAG Nodes.
//

def YCoreRetsp : SDNode<"YCoreISD::RETSP", SDTBrind,
                      [SDNPHasChain, SDNPOptInGlue, SDNPMayLoad, SDNPVariadic]>;

def frametoargsoffset : SDNode<"YCoreISD::FRAME_TO_ARGS_OFFSET", SDTIntLeaf,
                               []>;

def SDT_YCoreStwsp    : SDTypeProfile<0, 2, [SDTCisInt<1>]>;
def YCoreStwsp        : SDNode<"YCoreISD::STWSP", SDT_YCoreStwsp,
                               [SDNPHasChain, SDNPMayStore]>;

def SDT_YCoreLdwsp    : SDTypeProfile<1, 1, [SDTCisInt<1>]>;
def YCoreLdwsp        : SDNode<"YCoreISD::LDWSP", SDT_YCoreLdwsp,
                               [SDNPHasChain, SDNPMayLoad]>;

// These are target-independent nodes, but have target-specific formats.
def SDT_YCoreCallSeqStart : SDCallSeqStart<[ SDTCisVT<0, i32>,
                                             SDTCisVT<1, i32> ]>;
def SDT_YCoreCallSeqEnd   : SDCallSeqEnd<[ SDTCisVT<0, i32>,
                                           SDTCisVT<1, i32> ]>;

def callseq_start : SDNode<"ISD::CALLSEQ_START", SDT_YCoreCallSeqStart,
                           [SDNPHasChain, SDNPOutGlue]>;
def callseq_end   : SDNode<"ISD::CALLSEQ_END",   SDT_YCoreCallSeqEnd,
                           [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue]>;

//===----------------------------------------------------------------------===//
// Instruction Pattern Stuff
//===----------------------------------------------------------------------===//

def immUs : PatLeaf<(imm), [{
  return (uint32_t)N->getZExtValue() <= 11;
}]>;

def immU6 : PatLeaf<(imm), [{
  return (uint32_t)N->getZExtValue() < (1 << 6);
}]>;

def immU16 : PatLeaf<(imm), [{
  return (uint32_t)N->getZExtValue() < (1 << 16);
}]>;

// Addressing modes
def ADDRspii : ComplexPattern<i32, 2, "SelectADDRspii", [add, frameindex], []>;

// Address operands
def MEMii : Operand<i32> {
  let MIOperandInfo = (ops i32imm, i32imm);
}

//===----------------------------------------------------------------------===//
// Instruction Class Templates
//===----------------------------------------------------------------------===//

// Three operand short

multiclass F3R_2RUS<bits<5> opc1, bits<5> opc2, string OpcStr, SDNode OpNode> {
  def _3r: _F3R<opc1, (outs GRRegs:$dst), (ins GRRegs:$b, GRRegs:$c),
                !strconcat(OpcStr, " $dst, $b, $c"),
                [(set GRRegs:$dst, (OpNode GRRegs:$b, GRRegs:$c))]>;
  def _2rus : _F2RUS<opc2, (outs GRRegs:$dst), (ins GRRegs:$b, i32imm:$c),
                     !strconcat(OpcStr, " $dst, $b, $c"),
                     [(set GRRegs:$dst, (OpNode GRRegs:$b, immUs:$c))]>;
}

multiclass F3R_2RUS_np<bits<5> opc1, bits<5> opc2, string OpcStr> {
  def _3r: _F3R<opc1, (outs GRRegs:$dst), (ins GRRegs:$b, GRRegs:$c),
                !strconcat(OpcStr, " $dst, $b, $c"), []>;
  def _2rus : _F2RUS<opc2, (outs GRRegs:$dst), (ins GRRegs:$b, i32imm:$c),
                     !strconcat(OpcStr, " $dst, $b, $c"), []>;
}

class F3R<bits<5> opc, string OpcStr, SDNode OpNode> :
  _F3R<opc, (outs GRRegs:$dst), (ins GRRegs:$b, GRRegs:$c),
       !strconcat(OpcStr, " $dst, $b, $c"),
       [(set GRRegs:$dst, (OpNode GRRegs:$b, GRRegs:$c))]>;

class F3R_np<bits<5> opc, string OpcStr> :
  _F3R<opc, (outs GRRegs:$dst), (ins GRRegs:$b, GRRegs:$c),
       !strconcat(OpcStr, " $dst, $b, $c"), []>;
// Three operand long

class FL3R<bits<9> opc, string OpcStr, SDNode OpNode> :
  _FL3R<opc, (outs GRRegs:$dst), (ins GRRegs:$b, GRRegs:$c),
        !strconcat(OpcStr, " $dst, $b, $c"),
        [(set GRRegs:$dst, (OpNode GRRegs:$b, GRRegs:$c))]>;

// U6
multiclass FU6_LU6<bits<10> opc, string OpcStr, SDNode OpNode> {
  def _u6: _FU6<opc, (outs), (ins i32imm:$a), !strconcat(OpcStr, " $a"),
                [(OpNode immU6:$a)]>;
  def _lu6: _FLU6<opc, (outs), (ins i32imm:$a), !strconcat(OpcStr, " $a"),
                  [(OpNode immU16:$a)]>;
}

multiclass FU6_LU6_np<bits<10> opc, string OpcStr> {
  def _u6: _FU6<opc, (outs), (ins i32imm:$a), !strconcat(OpcStr, " $a"), []>;
  def _lu6: _FLU6<opc, (outs), (ins i32imm:$a), !strconcat(OpcStr, " $a"), []>;
}

// Two operand short

class F2R_np<bits<6> opc, string OpcStr> :
  _F2R<opc, (outs GRRegs:$dst), (ins GRRegs:$b),
       !strconcat(OpcStr, " $dst, $b"), []>;

// Two operand long

//===----------------------------------------------------------------------===//
// Pseudo Instructions
//===----------------------------------------------------------------------===//

let Defs = [SP], Uses = [SP] in {
def ADJCALLSTACKDOWN : PseudoInstYCore<(outs), (ins i32imm:$amt, i32imm:$amt2),
                               "# ADJCALLSTACKDOWN $amt, $amt2",
                               [(callseq_start timm:$amt, timm:$amt2)]>;
def ADJCALLSTACKUP : PseudoInstYCore<(outs), (ins i32imm:$amt1, i32imm:$amt2),
                            "# ADJCALLSTACKUP $amt1",
                            [(callseq_end timm:$amt1, timm:$amt2)]>;
}

let isReMaterializable = 1 in
def FRAME_TO_ARGS_OFFSET : PseudoInstYCore<(outs GRRegs:$dst), (ins),
                               "# FRAME_TO_ARGS_OFFSET $dst",
                               [(set GRRegs:$dst, (frametoargsoffset))]>;

def LDWFI : PseudoInstYCore<(outs GRRegs:$dst), (ins MEMii:$addr),
                             "# LDWFI $dst, $addr",
                             [(set GRRegs:$dst, (load ADDRspii:$addr))]>;

def STWFI : PseudoInstYCore<(outs), (ins GRRegs:$src, MEMii:$addr),
                            "# STWFI $src, $addr",
                            [(store GRRegs:$src, ADDRspii:$addr)]>;

// SELECT_CC_* - Used to implement the SELECT_CC DAG operation.  Expanded after
// instruction selection into a branch sequence.
let usesCustomInserter = 1 in {
  def SELECT_CC : PseudoInstYCore<(outs GRRegs:$dst),
                              (ins GRRegs:$cond, GRRegs:$T, GRRegs:$F),
                              "# SELECT_CC PSEUDO!",
                              [(set GRRegs:$dst,
                                 (select GRRegs:$cond, GRRegs:$T, GRRegs:$F))]>;
}

//===----------------------------------------------------------------------===//
// Instructions
//===----------------------------------------------------------------------===//

// Three operand short
defm ADD : F3R_2RUS<0b00010, 0b10010, "add", add>;
defm SUB : F3R_2RUS<0b00011, 0b10011, "sub", sub>;
let hasSideEffects = 0 in {
defm EQ : F3R_2RUS_np<0b00110, 0b10110, "eq">;
def LSS_3r : F3R_np<0b11000, "lss">;
def LSU_3r : F3R_np<0b11001, "lsu">;
}
def AND_3r : F3R<0b00111, "and", and>;
def OR_3r : F3R<0b01000, "or", or>;

let mayLoad=1 in {
def LDW_3r : _F3R<0b01001, (outs GRRegs:$dst),
                  (ins GRRegs:$addr, GRRegs:$offset),
                  "ldw $dst, $addr[$offset]", []>;

def LDW_2rus : _F2RUS<0b00001, (outs GRRegs:$dst),
                      (ins GRRegs:$addr, i32imm:$offset),
                      "ldw $dst, $addr[$offset]", []>;
}

let Uses = [SP] in {
let mayStore=1 in {
def STWSP_ru6 : _FRU6<0b010101, (outs), (ins RRegs:$a, i32imm:$b),
                      "stw $a, sp[$b]",
                      [(YCoreStwsp RRegs:$a, immU6:$b)]>;
}

let mayLoad=1 in {
def LDWSP_ru6 : _FRU6<0b010111, (outs RRegs:$a), (ins i32imm:$b),
                      "ldw $a, sp[$b]",
                      [(set RRegs:$a, (YCoreLdwsp immU6:$b))]>;

}
}

// U6
let Defs = [SP], Uses = [SP] in {
let hasSideEffects = 0 in
defm EXTSP : FU6_LU6_np<0b0111011110, "extsp">;

let mayStore = 1 in
defm ENTSP : FU6_LU6_np<0b0111011101, "entsp">;

let isReturn = 1, isTerminator = 1, mayLoad = 1, isBarrier = 1 in {
defm RETSP : FU6_LU6<0b0111011111, "retsp", YCoreRetsp>;
}
}
