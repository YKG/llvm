add_llvm_component_library(LLVMYCoreDisassembler
  YCoreDisassembler.cpp

  LINK_COMPONENTS
  MCDisassembler
  Support
  YCoreInfo

  ADD_TO_COMPONENT
  YCore
  )
