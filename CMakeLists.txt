add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/externals")

startProj(Haboobo)
  addIncludesAuto(${CMAKE_CURRENT_LIST_DIR}/src CMFT_SRC_c++ Source)
  addIncludesAuto(${CMAKE_CURRENT_LIST_DIR}/include CMFT_HEADER_c++ Headers)
  
  # Exe
  add_executable(Haboobo "${INCLUDES}")
  bindIncludes(Haboobo)
  
  # Shaders
  include("${CMAKE_CURRENT_LIST_DIR}/shaders/Shaders.cmake")
  set(HLSL_SHADER_GLOBAL_WORK_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/shaders")
  defineShaders(Haboobo ${HLSL_TYPE_VERTEX} "${Shaders_Vertex}")
  defineShaders(Haboobo ${HLSL_TYPE_PIXEL} "${Shaders_Pixel}")
  defineShaders(Haboobo ${HLSL_TYPE_COMPUTE} "${Shaders_Compute}")
  defineShaders(Haboobo ${HLSL_TYPE_HEADER} "${Shaders_HLSLI}" 1)

  target_compile_features(Haboobo PUBLIC cxx_std_17)
  target_link_libraries(Haboobo WSTRCore ImGui)
endProj(Haboobo)

setMain(Haboobo)