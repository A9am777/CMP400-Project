# CMakeGym
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/CMakeGym")
# Libs
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/WinSTR")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/catch2")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/args")

# Naked Libs
include(${CMAKE_CURRENT_LIST_DIR}/ImGui.cmake)