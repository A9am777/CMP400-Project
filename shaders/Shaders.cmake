include(${CMGym_DIR}/scripts/fileops.cmake)
include(${CMGym_DIR}/platform/win/D3D/hlsl.cmake)

set(HLSL_SHADER_GLOBAL_MODEL_MAJOR 5)
set(HLSL_SHADER_GLOBAL_MODEL_MINOR 0)
getDirFiles(Shaders_Vertex ${CMAKE_CURRENT_LIST_DIR} HLSL_TYPE_VERTEX 1)
getDirFiles(Shaders_Pixel ${CMAKE_CURRENT_LIST_DIR} HLSL_TYPE_PIXEL 1)
getDirFiles(Shaders_Compute ${CMAKE_CURRENT_LIST_DIR} HLSL_TYPE_COMPUTE 1)
getDirFiles(Shaders_HLSLI ${CMAKE_CURRENT_LIST_DIR} HLSL_TYPE_HEADER 1)