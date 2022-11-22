//===-- YCoreInstrFormats.td - YCore Instruction Formats ---*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Instruction format superclass
//===----------------------------------------------------------------------===//
class InstYCore<int sz, dag outs, dag ins, string asmstr, list<dag> pattern>
    : Instruction {
  field bits<32> Inst;

  let Namespace = "YCore";
  dag OutOperandList = outs;
  dag InOperandList = ins;
  let AsmString   = asmstr;
  let Pattern = pattern;
  let Size = sz;
  field bits<32> SoftFail = 0;
}

// YCore pseudo instructions format
class PseudoInstYCore<dag outs, dag ins, string asmstr, list<dag> pattern>
   : InstYCore<0, outs, ins, asmstr, pattern> {
  let isPseudo = 1;
}

//===----------------------------------------------------------------------===//
// Instruction formats
//===----------------------------------------------------------------------===//

class _F3R<bits<5> opc, dag outs, dag ins, string asmstr, list<dag> pattern>
    : InstYCore<2, outs, ins, asmstr, pattern> {
  let Inst{15-11} = opc;
  let DecoderMethod = "Decode3RInstruction";
}

class _FL3R<bits<9> opc, dag outs, dag ins, string asmstr, list<dag> pattern>
    : InstYCore<4, outs, ins, asmstr, pattern> {
  let Inst{31-27} = opc{8-4};
  let Inst{26-20} = 0b1111110;
  let Inst{19-16} = opc{3-0};

  let Inst{15-11} = 0b11111;
  let DecoderMethod = "DecodeL3RInstruction";
}

class _F2RUS<bits<5> opc, dag outs, dag ins, string asmstr, list<dag> pattern>
    : InstYCore<2, outs, ins, asmstr, pattern> {
  let Inst{15-11} = opc;
  let DecoderMethod = "Decode2RUSInstruction";
}

class _FRU6<bits<6> opc, dag outs, dag ins, string asmstr, list<dag> pattern>
    : InstYCore<2, outs, ins, asmstr, pattern> {
  bits<4> a;
  bits<6> b;

  let Inst{15-10} = opc;
  let Inst{9-6} = a;
  let Inst{5-0} = b;
}

class _FU6<bits<10> opc, dag outs, dag ins, string asmstr, list<dag> pattern>
    : InstYCore<2, outs, ins, asmstr, pattern> {
  bits<6> a;

  let Inst{15-6} = opc;
  let Inst{5-0} = a;
}

class _FLU6<bits<10> opc, dag outs, dag ins, string asmstr, list<dag> pattern>
    : InstYCore<4, outs, ins, asmstr, pattern> {
  bits<16> a;

  let Inst{31-22} = opc;
  let Inst{21-16} = a{5-0};
  let Inst{15-10} = 0b111100;
  let Inst{9-0} = a{15-6};
}
