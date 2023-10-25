#pragma once

#include <Window.h>
#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.h>
#include <imgui.h>

namespace Haboob
{
  class HaboobWindow : public WSTR::Window
  {
    public:
    HaboobWindow();
    ~HaboobWindow();


    private:
  };
}