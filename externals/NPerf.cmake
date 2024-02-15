include("${CMAKE_CURRENT_LIST_DIR}/NPerf/Samples/cmake/NvPerfConfig.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/NPerf/Samples/cmake/NvPerfUtilityConfig.cmake")

startLib(NPerfHUD)
set(NPerfHUDDir "${CMAKE_CURRENT_LIST_DIR}/NPerf/Samples/NvPerfUtility")
set(NPerfHUDSrcs "/imports/implot-0.13/implot.cpp;/imports/implot-0.13/implot_items.cpp")
set(NPerfHUDHeaderDirs "/imports/implot-0.13;/imports/rapidyaml-0.4.0")

addIncludes(${NPerfHUDDir} NPerfHUDSrcs Sources)
addIncludes(${NPerfHUDDir} NPerfHUDHeaderDirs Headers)

add_library(NPerfHUD STATIC "${INCLUDES}")
bindIncludes(NPerfHUD)

target_link_libraries(NPerfHUD ImGui NvPerfUtilityImportsImGui NvPerfUtilityImportsImPlot NvPerfUtilityImportsRyml)

endLib(NPerfHUD)