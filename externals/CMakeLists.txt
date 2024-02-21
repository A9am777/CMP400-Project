# CMakeGym
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/CMakeGym")
# Libs
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/WinSTR")
set(CATCH_INSTALL_DOCS OFF CACHE BOOL "")
set(CATCH_INSTALL_EXTRAS OFF CACHE BOOL "")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/catch2")
set(ARGS_BUILD_EXAMPLE OFF CACHE BOOL "")
set(ARGS_BUILD_UNITTESTS OFF CACHE BOOL "")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/args")

# Naked Libs
include(${CMAKE_CURRENT_LIST_DIR}/ImGui.cmake)