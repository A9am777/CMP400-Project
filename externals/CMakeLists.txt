# CMakeGym
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/CMakeGym")
# Libs
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/WinSTR")

# Naked Libs
include(${CMAKE_CURRENT_LIST_DIR}/ImGui.cmake)