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

    shaderManager.setRootDir(CURRENT_DIRECTORY + L"/..");
    shaderManager.setShaderDir(L"shaders"); // TODO: this can be a program param

    // TODO: TEST
    testShader = new Shader(Shader::Type::Pixel, L"TestShaders/GParticles");
    testShader->initShader(&device, &shaderManager);
  }

  void HaboobWindow::main()
  {
    // Determine the time since last frame
    float dt = 1.f;
    {
      Clock::time_point frameStart = Clock::now();
      dt = std::chrono::duration<float, Precision>(frameStart - lastFrame).count() / float(Precision::den);
      lastFrame = frameStart;
    }

    // Handle input
    {
      input();
      keys.clearPresses();
      mouse.clearEvents();
    }

    // Update
    update(dt);

    // Handle rendering
    {
      imguiFrameBegin();
      device.clearBackBuffer();

      device.setBackBufferTarget();
      render();

      device.setBackBufferTarget();
      imguiFrameEnd();
      device.swapBuffer();
    }
  }

  void HaboobWindow::onEnd()
  {
    imguiEnd();
  }

  void HaboobWindow::onResize()
  {
    if (device.getContext())
    {
      if (device.resizeBackBuffer(getWidth(), getHeight()) != S_OK)
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

  void HaboobWindow::update(float dt)
  {

  }

  void HaboobWindow::render()
  {
    renderTestGUI();
  }

  void HaboobWindow::createD3D()
  {
    device.create(D3D11_CREATE_DEVICE_BGRA_SUPPORT, { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 });
    device.makeSwapChain(wHandle);
    device.makeBackBuffer();
  }

  void HaboobWindow::imguiStart()
  {
    if (imgui = ImGui::CreateContext())
    {
      ImGui::SetCurrentContext(imgui);
      assert(ImGui_ImplWin32_Init(wHandle));
      assert(ImGui_ImplDX11_Init(device.getDevice().Get(), device.getContext().Get()));
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
      ImGui::Text("Hello World");
    }
    ImGui::End();
  }
}