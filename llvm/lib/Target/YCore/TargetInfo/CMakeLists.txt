add_llvm_component_library(LLVMYCoreInfo
  YCoreTargetInfo.cpp

  LINK_COMPONENTS
  MC
  Support

  ADD_TO_COMPONENT
  YCore
  )
