#include "Window/HaboobWindow.h"

#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.h>

#include "Rendering/Scene/SceneStructs.h"

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
    {
      ID3D11Device* dev = device.getDevice().Get();

      mainRender.create(dev, getWidth(), getHeight());

      testVertexShader = new Shader(Shader::Type::Vertex, L"TestShaders/MeshShaderV");
      testPixelShader = new Shader(Shader::Type::Pixel, L"TestShaders/MeshShaderP");
      testComputeShader = new Shader(Shader::Type::Compute, L"TestShaders/TestComputeScreenDraw");
      testVertexShader->initShader(dev, &shaderManager);
      testPixelShader->initShader(dev, &shaderManager);
      testComputeShader->initShader(dev, &shaderManager);
      testMesh.build(dev);

      RenderTarget::copyShader.initShader(dev, &shaderManager);
    }

    // TEST
    {
      D3D11_BUFFER_DESC cameraBufferDesc;
      HRESULT result = S_OK; // unused
      cameraBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
      cameraBufferDesc.ByteWidth = sizeof(CameraPack);
      cameraBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
      cameraBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
      cameraBufferDesc.MiscFlags = 0;
      cameraBufferDesc.StructureByteStride = 0;
      result = device.getDevice()->CreateBuffer(&cameraBufferDesc, NULL, cameraBuffer.GetAddressOf());
    }

    lastFrame = Clock::now();
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
      input(dt);
      keys.clearPresses();
      mouse.clearEvents();
    }

    // Update
    update(dt);

    // Handle rendering
    {
      imguiFrameBegin();

      renderBegin();
      render();
      renderOverlay();
      renderMirror();

      renderTestGUI();

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
      assert(device.resizeBackBuffer(getWidth(), getHeight()) == S_OK);
    }

    adjustProjection();

    RECT rect;
    GetClientRect(wHandle, &rect);
    InvalidateRect(wHandle, &rect, TRUE);

    imguiFrameResize();
  }

  void HaboobWindow::onKeyDown(unsigned char vKey)
  {
    keys.keyDown(vKey);
  }

  void HaboobWindow::onKeyUp(unsigned char vKey)
  {
    keys.keyUp(vKey);
  }

  void HaboobWindow::onMouseMove(short x, short y)
  {
    mouse.onMouseMove(x, y);
  }

  void HaboobWindow::onMove()
  {

  }

  void HaboobWindow::input(float dt)
  {
    if (ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard) { return; }

    camTest.update(dt, &keys, &mouse);
  }

  void HaboobWindow::update(float dt)
  {
    
  }

  void HaboobWindow::render()
  {
    ID3D11DeviceContext* context = device.getContext().Get();

    // TEST
    {
      testVertexShader->bindShader(context);
      testPixelShader->bindShader(context);

      camTest.setWorld(XMMatrixIdentity() * XMMatrixTranslation(cubePos[0], cubePos[1], cubePos[2]));

      // Camera data
      {
        D3D11_MAPPED_SUBRESOURCE mapped;
        HRESULT result = context->Map(cameraBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        camTest.putPack(mapped.pData);
        context->Unmap(cameraBuffer.Get(), 0);
      }
      context->VSSetConstantBuffers(0, 1, cameraBuffer.GetAddressOf());

      testMesh.useBuffers(context);
      testMesh.draw(context);
    }
  }

  void HaboobWindow::createD3D()
  {
    assert(!FAILED(device.create(D3D11_CREATE_DEVICE_BGRA_SUPPORT)));
    assert(!FAILED(device.makeSwapChain(wHandle)));
    assert(!FAILED(device.resizeBackBuffer(getWidth(), getHeight())));
    assert(!FAILED(device.makeStates()));

    mainRasterMode = static_cast<UInt>(device.getRasterState());
    adjustProjection();
  }

  void HaboobWindow::adjustProjection()
  {
    mainRender.resize(device.getDevice().Get(), getWidth(), getHeight());

    // Setup the projection matrix.
    float fov = (float)XM_PIDIV4;
    float screenAspect = float(getWidth()) / float(getHeight());

    float nearZ = .1f;
    float farZ = 100.f;
    camTest.setProjection(XMMatrixPerspectiveFovLH(fov, screenAspect, nearZ, farZ));
  }

  LRESULT HaboobWindow::customRoutine(UINT message, WPARAM wParam, LPARAM lParam)
  {
    return ImGui_ImplWin32_WndProcHandler(wHandle, message, wParam, lParam);
  }

  void HaboobWindow::renderBegin()
  {
    // Render to the main target using vanilla settings
    device.setRasterState(static_cast<DisplayDevice::RasterFlags>(mainRasterMode));
    device.setDepthEnabled(true);
    mainRender.clear(device.getContext().Get());
    mainRender.setTarget(device.getContext().Get(), device.getDepthBuffer());
  }

  void HaboobWindow::renderOverlay()
  {
    ID3D11DeviceContext* context = device.getContext().Get();
    device.setBackBufferTarget();
    testComputeShader->bindShader(context);
    ID3D11UnorderedAccessView* accessView = mainRender.getComputeView();
    context->CSSetUnorderedAccessViews(0, 1, &accessView, 0);
    testComputeShader->dispatch(context, mainRender.getWidth(), mainRender.getHeight(), 1);
    accessView = nullptr;
    context->CSSetUnorderedAccessViews(0, 1, &accessView, 0);
  }

  void HaboobWindow::renderMirror()
  {
    device.clearBackBuffer();
    device.setBackBufferTarget();
    device.setRasterState(DisplayDevice::RasterFlags::RASTER_STATE_DEFAULT);
    device.setDepthEnabled(false);
    RenderTarget::copyShader.setProjectionMatrix(device.getOrthoMatrix());
    mainRender.renderFrom(device.getContext().Get());
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

  void HaboobWindow::renderTestGUI()
  {
    if (ImGui::Begin("DEBUGWINDOW", nullptr, ImGuiWindowFlags_::ImGuiWindowFlags_None))
    {
      ImGui::Text("Hello World");
      ImGui::DragFloat3("Cube Pos", cubePos, 1.f, -10.f, 10.f);

      if (ImGui::CollapsingHeader("Camera"))
      {
        ImGui::DragFloat3("Camera Pos", &camTest.getPosition().x, 1.f, -10.f, 10.f);
        ImGui::DragFloat2("Camera Rot", &camTest.getAngles().x, XM_PI * .01f, -XM_PI, XM_PI);
      }

      if (ImGui::CollapsingHeader("Raster State"))
      {
        ImGui::CheckboxFlags("Solid/Wireframe", &mainRasterMode, (UInt)DisplayDevice::RASTER_FLAG_SOLID);
        ImGui::CheckboxFlags("Cull", &mainRasterMode, (UInt)DisplayDevice::RASTER_FLAG_CULL);
        ImGui::CheckboxFlags("Backface/Frontface", &mainRasterMode, (UInt)DisplayDevice::RASTER_FLAG_BACK);
      }
    }
    ImGui::End();
  }
}