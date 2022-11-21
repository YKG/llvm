add_llvm_component_group(YCore)

set(LLVM_TARGET_DEFINITIONS YCore.td)

tablegen(LLVM YCoreGenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM YCoreGenCallingConv.inc -gen-callingconv)
tablegen(LLVM YCoreGenDAGISel.inc -gen-dag-isel)
tablegen(LLVM YCoreGenDisassemblerTables.inc -gen-disassembler)
tablegen(LLVM YCoreGenInstrInfo.inc -gen-instr-info)
tablegen(LLVM YCoreGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM YCoreGenSubtargetInfo.inc -gen-subtarget)

add_public_tablegen_target(YCoreCommonTableGen)

add_llvm_target(YCoreCodeGen
  YCoreAsmPrinter.cpp
  YCoreFrameLowering.cpp
  YCoreInstrInfo.cpp
  YCoreISelDAGToDAG.cpp
  YCoreISelLowering.cpp
  YCoreLowerThreadLocal.cpp
  YCoreMachineFunctionInfo.cpp
  YCoreMCInstLower.cpp
  YCoreRegisterInfo.cpp
  YCoreSubtarget.cpp
  YCoreTargetMachine.cpp
  YCoreTargetObjectFile.cpp
  YCoreSelectionDAGInfo.cpp

  LINK_COMPONENTS
  Analysis
  AsmPrinter
  CodeGen
  Core
  MC
  SelectionDAG
  Support
  Target
  TransformUtils
  YCoreDesc
  YCoreInfo

  ADD_TO_COMPONENT
  YCore
  )

add_subdirectory(Disassembler)
add_subdirectory(MCTargetDesc)
add_subdirectory(TargetInfo)
