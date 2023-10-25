startLib(ImGui)
set(ImGuiDir ${CMAKE_CURRENT_LIST_DIR}/imgui/)
set(ImGuiSrcs backends/imgui_impl_dx11.cpp;backends/imgui_impl_win32.cpp;imgui_tables.cpp;imgui_widgets.cpp;imgui_draw.cpp;imgui.cpp)
set(ImGuiHeaders backends/imgui_impl_dx11.h;backends/imgui_impl_win32.h;imgui.h)
addIncludes(${ImGuiDir} ImGuiHeaders Headers)
addIncludes(${ImGuiDir} ImGuiSrcs Sources)

add_library(ImGui STATIC "${INCLUDES}")
bindIncludes(ImGui)

endLib(ImGui)