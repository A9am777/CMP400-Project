#include "Window/HaboobWindow.h"

#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.h>

#include "Rendering/Scene/SceneStructs.h"

namespace Haboob
{
  HaboobWindow::HaboobWindow() : imgui{ nullptr }, fps{.0f}
  {
    // Controls
    mainCamera.getMoveRate() = 6.f;
    mainCamera.getPosition() = { -8.42f, .93f, -1.41f };
    mainCamera.getAngles() = { 1.36f, .1f, .0f };

    // Main rendering params
    mainRasterMode = DisplayDevice::RASTER_STATE_DEFAULT;
    gbuffer.getGamma() = .2f;
    gbuffer.getExposure() = 1.3f;
    dirLightPack.diffuse = { 3.96f, 3.92f, 3.14f };
    dirLightPack.ambient = { 0.96f, 0.92f, 0.14f };
    dirLightPack.direction = { -1.f, .25f, .0f, 1.f };

    // Raymarch params
    {
      auto& opticsInfo = raymarchShader.getOpticsInfo();

      opticsInfo.anisotropicForwardTerms = { .735f, .732f, .651f, .735f };
      opticsInfo.anisotropicBackwardTerms = { -.735f, -.732f, -.651f, -.735f };
      opticsInfo.phaseBlendWeightTerms = { .09f, .09f, .09f, .09f, };
      opticsInfo.scatterAngstromExponent = 3.1f;

      opticsInfo.absorptionAngstromExponent = 2.1f;
      opticsInfo.powderCoefficient = .1f;
      opticsInfo.attenuationFactor = 21.1f;
    }

    raymarchShader.getMarchInfo().iterations = 26;

    // Produce standalone shaders
    deferredVertexShader = new Shader(Shader::Type::Vertex, L"Raster/DeferredMeshShaderV", true);
    deferredPixelShader = new Shader(Shader::Type::Pixel, L"Raster/DeferredMeshShaderP", true);
  }
  HaboobWindow::~HaboobWindow()
  {
    delete deferredVertexShader;
    delete deferredPixelShader;
  }

  void HaboobWindow::onStart()
  {
    Window::onStart();
    createD3D();
    imguiStart();

    shaderManager.setRootDir(CURRENT_DIRECTORY + L"/..");
    shaderManager.setShaderDir(L"shaders"); // TODO: this can be a program param

    {
      auto dev = device.getDevice().Get();
      gbuffer.create(dev, getWidth(), getHeight());

      // Initialise all shaders
      {
        deferredVertexShader->initShader(dev, &shaderManager);
        deferredPixelShader->initShader(dev, &shaderManager);
        raymarchShader.initShader(dev, &shaderManager);
        haboobVolume.initShader(dev, &shaderManager);
        RenderTarget::copyShader.initShader(dev, &shaderManager);
        GBuffer::toneMapShader.initShader(dev, &shaderManager);
        GBuffer::lightShader.initShader(dev, &shaderManager);
      }

      shaderManager.bakeMacros(dev);
      
      // Generate assets
      {
        sphereMesh.build(dev);
        cubeMesh.build(dev);
        planeMesh.build(dev);

        haboobVolume.rebuild(dev);
        haboobVolume.render(device.getContext().Get());
      }

      // Create buffers
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

        renderGUI();

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
      device.resizeBackBuffer(getWidth(), getHeight());
    }

    adjustProjection();

    // Adjust the window rect
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

    mainCamera.update(dt, &keys, &mouse);
  }

  void HaboobWindow::update(float dt)
  {
    fps = 1.f / dt;

    // Mirror some shader env macros
    shaderManager.setMacro("MACRO_MANAGED", "1"); // Signal program is taking control

    // If macros changed, recompile shaders!
    shaderManager.bakeMacros(device.getDevice().Get());
  }

  void HaboobWindow::render()
  {
    ID3D11DeviceContext* context = device.getContext().Get();

    // Render the opaque scene in a dirty way
    deferredVertexShader->bindShader(context);
    deferredPixelShader->bindShader(context);
    {
      context->VSSetConstantBuffers(0, 1, cameraBuffer.GetAddressOf());
      context->PSSetConstantBuffers(0, 1, lightBuffer.GetAddressOf());

      mainCamera.setWorld(XMMatrixIdentity() * XMMatrixTranslation(spherePos[0], spherePos[1], spherePos[2]));
      redoCameraBuffer(context);

      sphereMesh.useBuffers(context);
      sphereMesh.draw(context);

      mainCamera.setWorld(XMMatrixScaling(5.f, 5.f, 1.f) * XMMatrixLookToLH(XMVectorZero(), XMVectorSet(.0f, 1.f, .0f, 1.f), XMVectorSet(.0f, .0f, -1.f, 1.f)) * XMMatrixTranslation(.0f, -1.f, .0f));
      redoCameraBuffer(context);

      planeMesh.useBuffers(context);
      planeMesh.draw(context);

      mainCamera.setWorld(XMMatrixTranslation(-2.f, 1.5f, .0f));
      redoCameraBuffer(context);

      cubeMesh.useBuffers(context);
      cubeMesh.draw(context);
    }
    deferredVertexShader->unbindShader(context);
    deferredPixelShader->unbindShader(context);
  }

  void HaboobWindow::createD3D()
  {
    device.create(D3D11_CREATE_DEVICE_BGRA_SUPPORT);
    device.makeSwapChain(wHandle);
    device.resizeBackBuffer(getWidth(), getHeight());
    device.makeStates();

    mainRasterMode = static_cast<UInt>(device.getRasterState());
    adjustProjection();
  }

  void HaboobWindow::adjustProjection()
  {
    gbuffer.resize(device.getDevice().Get(), getWidth(), getHeight());

    // Setup the projection matrix.
    float fov = (float)XM_PIDIV4;
    float screenAspect = float(getWidth()) / float(getHeight());

    float nearZ = .1f;
    float farZ = 100.f;
    mainCamera.setProjection(XMMatrixPerspectiveFovLH(fov, screenAspect, nearZ, farZ));
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
      mainCamera.putPack(mapped.pData);
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
    auto context = device.getContext().Get();

    // Light data
    {
      D3D11_MAPPED_SUBRESOURCE mapped;
      HRESULT result = context->Map(lightBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
      std::memcpy(mapped.pData, &dirLightPack, sizeof(DirectionalLightPack));

      // Normalise direction before sending
      XMVECTOR vec = XMLoadFloat4(&dirLightPack.direction);
      vec = XMVector3Normalize(vec);
      XMStoreFloat4(&reinterpret_cast<DirectionalLightPack*>(mapped.pData)->direction, vec);

      context->Unmap(lightBuffer.Get(), 0);
    }
    // Basic lit pass
    gbuffer.lightPass(context, lightBuffer.Get());

    raymarchShader.setCameraBuffer(cameraBuffer);
    raymarchShader.setLightBuffer(lightBuffer);
    raymarchShader.setTarget(&gbuffer.getLitColourTarget());

    // Raymarch!
    raymarchShader.bindShader(context, haboobVolume.getShaderView());
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
      ImGui_ImplWin32_Init(wHandle);
      ImGui_ImplDX11_Init(device.getDevice().Get(), device.getContext().Get());
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

  void HaboobWindow::renderGUI()
  {
    if (ImGui::Begin("DEBUGWINDOW", nullptr, ImGuiWindowFlags_::ImGuiWindowFlags_None))
    {
      ImGui::Text("FPS: %f", fps);
      ImGui::Text("Hello World");
      ImGui::DragFloat3("Sphere Pos", spherePos, 1.f, -10.f, 10.f);

      if (ImGui::CollapsingHeader("Camera"))
      {
        ImGui::DragFloat3("Camera Pos", &mainCamera.getPosition().x, 1.f, -10.f, 10.f);
        ImGui::DragFloat2("Camera Rot", &mainCamera.getAngles().x, XM_PI * .01f, -XM_PI, XM_PI);
        ImGui::DragFloat("Camera Speed", &mainCamera.getMoveRate());
        ImGui::DragFloat("Camera Look Speed", &mainCamera.getMouseSensitivity());
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
        ImGui::DragFloat3("Light ambient", &dirLightPack.ambient.x);
        ImGui::DragFloat("Gamma", &gbuffer.getGamma());
        ImGui::DragFloat("Exposure", &gbuffer.getExposure());
      }

      if (ImGui::CollapsingHeader("Haboob"))
      {
        auto& volumeInfo = haboobVolume.getVolumeInfo();
        ImGui::DragInt3("Haboob Resolution", (int*)&volumeInfo.size, .1f, 0, 1024);

        ImGui::DragScalarN("Haboob Seed", ImGuiDataType_U32, &volumeInfo.seed, 4);

        ImGui::DragFloat("Haboob World Size", &volumeInfo.worldSize, .1f, 0, 10.f);
        ImGui::DragFloat("Haboob Octaves", &volumeInfo.octaves, .1f, .1f, 8.1f);
        ImGui::DragFloat("Haboob Fractional Gap", &volumeInfo.fractionalGap, .0f, 0, 10.f);
        ImGui::DragFloat("Haboob Fractional Increment", &volumeInfo.fractionalIncrement, .0f, 0, 10.f);

        ImGui::DragFloat("Haboob FBM Offset", &volumeInfo.fbmOffset, .0f, 0, 10.f);
        ImGui::DragFloat("Haboob FBM Scale", &volumeInfo.fbmScale, .0f, 0, 10.f);

        ImGui::DragFloat("Haboob Wacky Power (tm)", &volumeInfo.wackyPower, .0f, 0, 10.f);
        ImGui::DragFloat("Haboob Wacky Scale (tm)", &volumeInfo.wackyScale, .0f, 0, 10.f);

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
        ImGui::CheckboxFlags("Use manual step", &marchInfo.flagManualMarch, ~0);
      }

      if (ImGui::CollapsingHeader("Optics"))
      {
        auto& opticsInfo = raymarchShader.getOpticsInfo();
        
        ImGui::Text("Phase");
        ImGui::DragFloat4("Henyey-Greenstein anisotropy (f)", &opticsInfo.anisotropicForwardTerms.x);
        ImGui::DragFloat4("Henyey-Greenstein anisotropy (b)", &opticsInfo.anisotropicBackwardTerms.x);
        ImGui::DragFloat4("Phase blend", &opticsInfo.phaseBlendWeightTerms.x);
        ImGui::DragFloat("Scattering angstrom exponent", &opticsInfo.scatterAngstromExponent);
        
        ImGui::Text("Transmission");
        ImGui::DragFloat("Absorption angstrom exponent", &opticsInfo.absorptionAngstromExponent);
        ImGui::DragFloat("Beers Powder coefficient", &opticsInfo.powderCoefficient);
        ImGui::DragFloat("Attenuation scale", &opticsInfo.attenuationFactor);
        ImGui::DragFloat("Attenuation efEeEe", &opticsInfo.referenceWavelength);
        
        ImGui::Text("Flags");
        ImGui::CheckboxFlags("Apply beer", &opticsInfo.flagApplyBeer, ~0);
        shaderManager.setMacro("APPLY_BEER", std::to_string(opticsInfo.flagApplyBeer));
        ImGui::CheckboxFlags("Apply HG", &opticsInfo.flagApplyHG, ~0);
        shaderManager.setMacro("APPLY_HG", std::to_string(opticsInfo.flagApplyHG));
        ImGui::CheckboxFlags("Apply Spectral", &opticsInfo.flagApplySpectral, ~0);
        shaderManager.setMacro("APPLY_SPECTRAL", std::to_string(opticsInfo.flagApplySpectral));
      }
    }
    ImGui::End();
  }
}