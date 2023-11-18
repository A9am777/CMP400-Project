#include "Window/HaboobWindow.h"

#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.h>

#include "Rendering/Scene/SceneStructs.h"

namespace Haboob
{
  HaboobWindow::HaboobWindow() : imgui{ nullptr }
  {
    camTest.getMoveRate() = 6.f;
    gbuffer.getGamma() = .2f;
    gbuffer.getExposure() = 2.2f;
    dirLightPack.diffuse = { 4.96f, 4.92f, 4.14f };
    dirLightPack.direction = { -1.f, .5f, .0f, 1.f };
    raymarchShader.getOpticsInfo().colourHGScatter = { .7f, .73f, .65f };
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


    camTest.getPosition().y = 1.5f;
    // TODO: TEST
    {
      ID3D11Device* dev = device.getDevice().Get();

      mainRender.create(dev, getWidth(), getHeight());
      gbuffer.create(dev, getWidth(), getHeight());

      testVertexShader = new Shader(Shader::Type::Vertex, L"Raster/DeferredMeshShaderV");
      testPixelShader = new Shader(Shader::Type::Pixel, L"Raster/DeferredMeshShaderP");
      testComputeShader = new Shader(Shader::Type::Compute, L"TestShaders/TestComputeScreenDraw");
      testVertexShader->initShader(dev, &shaderManager);
      testPixelShader->initShader(dev, &shaderManager);
      testComputeShader->initShader(dev, &shaderManager);
      raymarchShader.initShader(dev, &shaderManager);
      haboobVolume.initShader(dev, &shaderManager);
      
      sphereMesh.build(dev);
      cubeMesh.build(dev);
      planeMesh.build(dev);

      haboobVolume.rebuild(dev);
      haboobVolume.render(device.getContext().Get());

      RenderTarget::copyShader.initShader(dev, &shaderManager);
      GBuffer::toneMapShader.initShader(dev, &shaderManager);
      GBuffer::lightShader.initShader(dev, &shaderManager);
    }

    // TEST
    {
      // Camera buffer
      D3D11_BUFFER_DESC bufferDesc;
      HRESULT result = S_OK; // unused
      bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
      bufferDesc.ByteWidth = sizeof(CameraPack);
      bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
      bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
      bufferDesc.MiscFlags = 0;
      bufferDesc.StructureByteStride = 0;
      result = device.getDevice()->CreateBuffer(&bufferDesc, NULL, cameraBuffer.ReleaseAndGetAddressOf());

      // Light buffer
      bufferDesc.ByteWidth = sizeof(DirectionalLightPack);
      result = device.getDevice()->CreateBuffer(&bufferDesc, NULL, lightBuffer.ReleaseAndGetAddressOf());
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
      device.setBackBufferTarget(); // Safety
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
    fps = 1.f / dt;
  }

  void HaboobWindow::render()
  {
    ID3D11DeviceContext* context = device.getContext().Get();

    // TEST
    {
      testVertexShader->bindShader(context);
      testPixelShader->bindShader(context);

      context->VSSetConstantBuffers(0, 1, cameraBuffer.GetAddressOf());
      context->PSSetConstantBuffers(0, 1, lightBuffer.GetAddressOf());

      camTest.setWorld(XMMatrixIdentity() * XMMatrixTranslation(spherePos[0], spherePos[1], spherePos[2]));
      redoCameraBuffer(context);

      sphereMesh.useBuffers(context);
      sphereMesh.draw(context);

      camTest.setWorld(XMMatrixScaling(5.f, 5.f, 1.f) * XMMatrixLookToLH(XMVectorZero(), XMVectorSet(.0f, 1.f, .0f, 1.f), XMVectorSet(.0f, .0f, -1.f, 1.f)) * XMMatrixTranslation(.0f, -1.f, .0f));
      redoCameraBuffer(context);

      planeMesh.useBuffers(context);
      planeMesh.draw(context);

      camTest.setWorld(XMMatrixTranslation(-2.f, 1.5f, .0f));
      redoCameraBuffer(context);

      cubeMesh.useBuffers(context);
      cubeMesh.draw(context);

      testVertexShader->unbindShader(context);
      testPixelShader->unbindShader(context);
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

    gbuffer.resize(device.getDevice().Get(), getWidth(), getHeight());

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

  void HaboobWindow::redoCameraBuffer(ID3D11DeviceContext* context)
  {
    // Camera data
    {
      D3D11_MAPPED_SUBRESOURCE mapped;
      HRESULT result = context->Map(cameraBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
      camTest.putPack(mapped.pData);
      context->Unmap(cameraBuffer.Get(), 0);
    }
  }

  void HaboobWindow::renderBegin()
  {
    // Render to the main target using vanilla settings
    device.setRasterState(static_cast<DisplayDevice::RasterFlags>(mainRasterMode));
    device.setDepthEnabled(true);
    gbuffer.clear(device.getContext().Get());
    gbuffer.setTargets(device.getContext().Get(), device.getDepthBuffer());
  }

  void HaboobWindow::renderOverlay()
  {
    ID3D11DeviceContext* context = device.getContext().Get();

    // Light data
    {
      D3D11_MAPPED_SUBRESOURCE mapped;
      HRESULT result = context->Map(lightBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
      std::memcpy(mapped.pData, &dirLightPack, sizeof(DirectionalLightPack));

      XMVECTOR vec = XMLoadFloat4(&dirLightPack.direction);
      vec = XMVector3Normalize(vec);
      XMStoreFloat4(&reinterpret_cast<DirectionalLightPack*>(mapped.pData)->direction, vec);

      context->Unmap(lightBuffer.Get(), 0);
    }
    gbuffer.lightPass(context, lightBuffer.Get());

    raymarchShader.setCameraBuffer(cameraBuffer);
    raymarchShader.setLightBuffer(lightBuffer);
    raymarchShader.setTarget(&gbuffer.getLitColourTarget());

    raymarchShader.bindShader(context);
    raymarchShader.render(context);
    raymarchShader.unbindShader(context);
  }

  void HaboobWindow::renderMirror()
  {
    device.clearBackBuffer();
    device.setBackBufferTarget();
    device.setRasterState(DisplayDevice::RasterFlags::RASTER_STATE_DEFAULT);
    device.setDepthEnabled(false);
    RenderTarget::copyShader.setProjectionMatrix(device.getOrthoMatrix());

    auto context = device.getContext().Get();
    gbuffer.finalLitPass(context);
    gbuffer.renderFromLit(context);
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
      ImGui::Text("fps: %f", fps);
      ImGui::Text("Hello World");
      ImGui::DragFloat3("Sphere Pos", spherePos, 1.f, -10.f, 10.f);

      if (ImGui::CollapsingHeader("Camera"))
      {
        ImGui::DragFloat3("Camera Pos", &camTest.getPosition().x, 1.f, -10.f, 10.f);
        ImGui::DragFloat2("Camera Rot", &camTest.getAngles().x, XM_PI * .01f, -XM_PI, XM_PI);
        ImGui::DragFloat("Camera Speed", &camTest.getMoveRate());
        ImGui::DragFloat("Camera Look Speed", &camTest.getMouseSensitivity());
      }

      if (ImGui::CollapsingHeader("Raster State"))
      {
        ImGui::CheckboxFlags("Solid/Wireframe", &mainRasterMode, (UInt)DisplayDevice::RASTER_FLAG_SOLID);
        ImGui::CheckboxFlags("Cull", &mainRasterMode, (UInt)DisplayDevice::RASTER_FLAG_CULL);
        ImGui::CheckboxFlags("Backface/Frontface", &mainRasterMode, (UInt)DisplayDevice::RASTER_FLAG_BACK);
      }

      if (ImGui::CollapsingHeader("Light"))
      {
        ImGui::DragFloat3("Light direction", &dirLightPack.direction.x);
        ImGui::DragFloat3("Light colour", &dirLightPack.diffuse.x);
        ImGui::DragFloat("Gamma", &gbuffer.getGamma());
        ImGui::DragFloat("Exposure", &gbuffer.getExposure());
      }

      if (ImGui::CollapsingHeader("Haboob"))
      {
        auto& volumeInfo = haboobVolume.getVolumeInfo();
        ImGui::DragInt3("Haboob Resolution X", (int*)&volumeInfo.size, .1f, 0, 1024);
        if (ImGui::Button("Regen Haboob"))
        {
          haboobVolume.rebuild(device.getDevice().Get());
          haboobVolume.render(device.getContext().Get());
        }
      }

      if (ImGui::CollapsingHeader("Raymarch"))
      {
        auto& marchInfo = raymarchShader.getMarchInfo();
        ImGui::DragFloat("Initial step size", &marchInfo.initialZStep);
        ImGui::DragFloat("Step size", &marchInfo.marchZStep);
        ImGui::DragInt("Step count", (int*)&marchInfo.iterations, .1f, 0, 100);
      }

      if (ImGui::CollapsingHeader("Optics"))
      {
        auto& opticsInfo = raymarchShader.getOpticsInfo();
        ImGui::DragFloat("Beer attenuation factor", &opticsInfo.attenuationFactor);
        ImGui::DragFloat3("Henyey-Greenstein phase coefficients", &opticsInfo.colourHGScatter.x, .01f, .0f, 1.f);
        ImGui::DragFloat("Density Coefficient", &opticsInfo.densityCoefficient);
        ImGui::CheckboxFlags("Apply beer", &opticsInfo.flagApplyBeer, ~0);
        ImGui::CheckboxFlags("Apply HG", &opticsInfo.flagApplyHG, ~0);
      }
    }
    ImGui::End();
  }
}