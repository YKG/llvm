add_llvm_component_library(LLVMYCoreDesc
  YCoreInstPrinter.cpp
  YCoreMCTargetDesc.cpp
  YCoreMCAsmInfo.cpp

  LINK_COMPONENTS
  MC
  Support
  YCoreInfo

  ADD_TO_COMPONENT
  YCore
  )
