add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/externals")

startProj(Haboobo)
  addIncludesAuto(${CMAKE_CURRENT_LIST_DIR}/src CMFT_SRC_c++ Source)
  addIncludesAuto(${CMAKE_CURRENT_LIST_DIR}/include CMFT_HEADER_c++ Headers)

  add_executable(Haboobo "${INCLUDES}")
  bindIncludes(Haboobo)

  target_link_libraries(Haboobo WSTRCore ImGui)
endProj(Haboobo)

setMain(Haboobo)