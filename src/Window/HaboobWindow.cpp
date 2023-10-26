#include "Window/HaboobWindow.h"

#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.h>

namespace Haboob
{
  HaboobWindow::HaboobWindow() : imgui{ nullptr }
  {

  }
  HaboobWindow::~HaboobWindow()
  {

  }

  void HaboobWindow::onStart()
  {
    Window::onStart();
    createD3D();
    imguiStart();
  }

  void HaboobWindow::main()
  {
    imguiFrameBegin();
    device.clearBackBuffer();

    device.setBackBufferTarget();
    render(0.0f);
    device.setBackBufferTarget();
    imguiFrameEnd();
    keys.clearPresses();
    mouse.clearEvents();

    device.swapBuffer();
  }

  void HaboobWindow::onEnd()
  {
    imguiEnd();
  }

  void HaboobWindow::onResize()
  {
    if (device.getContext())
    {
      if (device.resize(getWidth(), getHeight()) != S_OK)
      {
        std::cout << "UH \n";
      }
    }

    
    RECT rect;
    GetClientRect(wHandle, &rect);
    InvalidateRect(wHandle, &rect, TRUE);

    imguiFrameResize();
  }

  void HaboobWindow::onKeyDown(char vKey)
  {
    keys.keyDown(vKey);
    imguiKeyDown(vKey);
  }

  void HaboobWindow::onKeyUp(char vKey)
  {
    keys.keyUp(vKey);
    imguiKeyUp(vKey);
  }

  void HaboobWindow::onMouseMove(short x, short y)
  {
    mouse.onMouseMove(x, y);
    imguiMouseMove(x, y);
  }

  void HaboobWindow::onMove()
  {
    imguiMove();
  }

  void HaboobWindow::input()
  {
    if (ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard) { return; }


  }

  void HaboobWindow::render(float dt)
  {
    renderTestGUI();
  }

  void HaboobWindow::createD3D()
  {
    device.create();
    device.makeSwapChain(wHandle);
    device.makeBackBuffer();
  }

  void HaboobWindow::imguiStart()
  {
    if (imgui = ImGui::CreateContext())
    {
      ImGui::SetCurrentContext(imgui);
      assert(ImGui_ImplWin32_Init(wHandle));
      assert(ImGui_ImplDX11_Init(device.getDevice(), device.getContext()));
      ImGui::GetIO().Fonts->AddFontDefault();
      ImGui::GetIO().Fonts->Build();
      ImGui_ImplDX11_CreateDeviceObjects();
    }
  }

  void HaboobWindow::imguiEnd()
  {
    if (imgui)
    {
      ImGui_ImplWin32_Shutdown();
      ImGui_ImplDX11_Shutdown();
      ImGui::DestroyContext(imgui);
    }
  }

  void HaboobWindow::imguiFrameBegin()
  {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
  }

  void HaboobWindow::imguiFrameEnd()
  {
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
  }

  void HaboobWindow::imguiFrameResize()
  {
    if (imgui)
    {
      ImVec2 wSize = ImVec2{ float(getWidth()), float(getHeight()) };
      ImGui::SetNextWindowSize(wSize);
      ImGui::GetIO().DisplaySize = wSize;

      ImGui_ImplDX11_InvalidateDeviceObjects();
    }
  }

  void HaboobWindow::imguiKeyDown(char vKey)
  {
    switch (vKey)
    {
      case VK_LBUTTON:
        ImGui::GetIO().AddMouseButtonEvent(0, true);
        break;
    }
  }

  void HaboobWindow::imguiKeyUp(char vKey)
  {
    switch (vKey)
    {
      case VK_LBUTTON:
        ImGui::GetIO().AddMouseButtonEvent(0, false);
        break;
    }
  }

  void HaboobWindow::imguiMouseMove(short x, short y)
  {
    if (imgui)
    {
      ImGui::GetIO().MousePos = { float(x), float(y) };
    }
  }

  void HaboobWindow::imguiMove()
  {
    if (imgui)
    {
      ImGui::SetNextWindowPos(ImVec2{ float(getX()), float(getY()) });
    }
  }

  void HaboobWindow::renderTestGUI()
  {
    if (ImGui::Begin("DEBUGWINDOW", nullptr, ImGuiWindowFlags_::ImGuiWindowFlags_None))
    {
      ImGui::Text("oof dddddddddddddddddd");
    }
    ImGui::End();
  }
}