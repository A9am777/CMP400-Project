#include "Window/HaboobWindow.h"

#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.h>

#include "Rendering/Scene/SceneStructs.h"

namespace Haboob
{
  HaboobWindow::HaboobWindow() : imgui{ nullptr }, tcyCtx{ nullptr }, fps{ .0f }
  {
    setupDefaults();

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

    shaderManager.setRootDir(CURRENT_DIRECTORY + L"/..");
    shaderManager.setShaderDir(L"shaders"); // TODO: this can be a program param

    if (exportPathFlag && exportPathFlag->HasFlag() && exportPathFlag->Matched())
    {
      std::string exportSmallPath = exportPathFlag->Get();
      exportLocation = CURRENT_DIRECTORY + L"/../" + std::wstring(exportSmallPath.begin(), exportSmallPath.end());
    }

    createD3D();
    imguiStart();

    // Reflect discrete camera orbit to continuous
    if (cameraOrbit && orbitDiscreteProgress)
    {
      orbitProgress = float(orbitDiscreteProgress) * orbitStep;
    }

    tcyCtx = TracyD3D11Context(device.getDevice().Get(), device.getContext().Get());
    {
      auto dev = device.getDevice().Get();
      gbuffer.create(dev, requiredWidth, requiredHeight);
      raymarchShader.createIntermediate(dev, requiredWidth, requiredHeight);
      light.create(dev, 1024, 1024);

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

      scene.init(dev, &shaderManager);
      scene.setCamera(&mainCamera);

      shaderManager.bakeMacros(dev);
      
      // Generate assets
      {
        sphereMesh.build(dev, 128, 128);
        cubeMesh.build(dev);
        planeMesh.build(dev);
        scene.addMesh("Sphere", &sphereMesh);
        scene.addMesh("Cube", &cubeMesh);
        scene.addMesh("Plane", &planeMesh);

        haboobVolume.rebuild(dev);
        haboobVolume.render(device.getContext().Get());
        raymarchShader.getMarchInfo().texelDensity = float(haboobVolume.getVolumeInfo().size.x);
      }

      // Set up scene objects
      {
        typedef MeshInstance<VertexType> Instance;
        
        Instance* instance = new Instance(&sphereMesh);
        instance->getPosition() = { .0f, .0f, 2.5f };
        scene.addObject(instance);

        instance = new Instance(&planeMesh);
        instance->getRotation() = { .707f, .0f, .0f, .707f };
        instance->getPosition() = { .0f, -1.f, .0f };
        instance->getScale() = { 5.f, 5.f, 1.f };
        scene.addObject(instance);

        instance = new Instance(&cubeMesh);
        instance->getPosition() = { -2.f, 1.5f, .0f };
        scene.addObject(instance);

        // Haboob volume
        instance = new Instance(&sphereMesh);
        instance->getPosition() = { .0f, .0f, .0f };
        instance->getScale() = { 3.f, 3.f, 3.f };
        scene.addObject(instance);
        raymarchShader.setBox(instance);
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
      TracyD3D11Zone(tcyCtx, "D3DFrame");
      
      imguiFrameBegin();
      renderBegin();

        render();
        device.setBackBufferTarget(); // Safety
        
        renderOverlay();
        renderMirror();

        renderGUI();

      imguiFrameEnd();
      if (showWindow) { device.swapBuffer(); }
    }

    FrameMark;
    TracyD3D11Collect(tcyCtx);

    if (outputFrame)
    {
      exportFrame();
    }

    if (exitAfterFrame)
    {
      open = false;
    }
  }

  void HaboobWindow::onEnd()
  {
    imguiEnd();
    TracyD3D11Destroy(tcyCtx);

    // Ugly hack because Tracy will always block and the only
    // functionality required is graceful *existing* socket closure
    if (!TracyIsConnected)
    {
      TerminateProcess(GetCurrentProcess(), 0);
    }
  }

  void HaboobWindow::onResize()
  {
    if (!dynamicResolution) { return; }

    requiredWidth = getWidth();
    requiredHeight = getHeight();

    if (device.getContext())
    {
      device.resizeBackBuffer(requiredWidth, requiredHeight);
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
    ZoneNamed(Input, true);

    if (ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard) { return; }

    mainCamera.update(dt, &keys, &mouse);

    if (keys.isKeyPress('G'))
    {
      showGUI = !showGUI;
    }
  }

  void HaboobWindow::update(float dt)
  {
    ZoneNamed(Update, true);

    fps = 1.f / dt;

    // Mirror some shader env macros
    shaderManager.setMacro("MACRO_MANAGED", "1"); // Signal program is taking control

    {
      auto& marchInfo = raymarchShader.getMarchInfo();
      shaderManager.setMacro("MARCH_STEP_COUNT", std::to_string(marchInfo.iterations));
    }

    {
      auto& opticsInfo = raymarchShader.getOpticsInfo();
      shaderManager.setMacro("APPLY_BEER", std::to_string(opticsInfo.flagApplyBeer));
      shaderManager.setMacro("APPLY_HG", std::to_string(opticsInfo.flagApplyHG));
      shaderManager.setMacro("APPLY_SPECTRAL", std::to_string(opticsInfo.flagApplySpectral));
      shaderManager.setMacro("APPLY_CONE_TRACE", std::to_string(coneTrace));
      shaderManager.setMacro("APPLY_UPSCALE", std::to_string(upscaleTracing));
      shaderManager.setMacro("MARCH_MANUAL", std::to_string(manualMarch));
    }

    {
      shaderManager.setMacro("SHOW_DENSITY", std::to_string(showDensity));
      shaderManager.setMacro("SHOW_ANGSTROM", std::to_string(showAngstrom));
      shaderManager.setMacro("SHOW_SAMPLE_LEVEL", std::to_string(showSampleLevel));
      shaderManager.setMacro("SHOW_MASK", std::to_string(showMasks));
      shaderManager.setMacro("SHOW_RAY_TRAVEL", std::to_string(showRayTravel));
    }

    shaderManager.setMacro("SHADOW_EXPONENT", std::to_string(25.));

    // If macros changed, recompile shaders!
    shaderManager.bakeMacros(device.getDevice().Get());

    // Orbit the camera on the fixed path (overwrites input!)
    cameraOrbitStep(dt);

    // Re-render the haboob on demand
    if (renderHaboob)
    {
      haboobVolume.rebuild(device.getDevice().Get());
      haboobVolume.render(device.getContext().Get());
      raymarchShader.getMarchInfo().texelDensity = float(haboobVolume.getVolumeInfo().size.x);
    }

    raymarchShader.getBox()->setVisible(showBoundingBoxes);
    raymarchShader.setShouldUpscale(upscaleTracing);
  }

  void HaboobWindow::createD3D()
  {
    device.create(D3D11_CREATE_DEVICE_BGRA_SUPPORT);
    device.makeSwapChain(wHandle);
    device.resizeBackBuffer(requiredWidth, requiredHeight);
    device.makeStates();

    mainRasterMode = static_cast<UInt>(device.getRasterState());
    adjustProjection();
  }

  void HaboobWindow::adjustProjection()
  {
    gbuffer.resize(device.getDevice().Get(), requiredWidth, requiredHeight);
    raymarchShader.resizeIntermediate(device.getDevice().Get(), requiredWidth, requiredHeight);

    // Setup the projection matrix.
    float fov = (float)XM_PIDIV4;
    float screenAspect = float(requiredWidth) / float(requiredHeight);

    float nearZ = .1f;
    float farZ = 100.f;
    mainCamera.setProjection(XMMatrixPerspectiveFovLH(fov, screenAspect, nearZ, farZ));

    // Determine pixel radius
    {
      float heightDivisions = 2.f / requiredHeight;
      float zSpread = std::tanf(fov * .5f);
      float zStepRadius = zSpread * heightDivisions * std::sqrtf(2.f); // Spread of the divisions as a radius sqrt(2)

      auto& marchInfo = raymarchShader.getMarchInfo();
      marchInfo.pixelRadius = zStepRadius * nearZ;
      marchInfo.pixelRadiusDelta = zStepRadius;
    }
  }

  LRESULT HaboobWindow::customRoutine(UINT message, WPARAM wParam, LPARAM lParam)
  {
    return ImGui_ImplWin32_WndProcHandler(wHandle, message, wParam, lParam);
  }

  void HaboobWindow::renderBegin()
  {
    TracyD3D11Zone(tcyCtx, "D3DFrameBegin");
    ZoneNamed(RenderBegin, true);

    // Render to the main target using vanilla settings
    device.setRasterState(static_cast<DisplayDevice::RasterFlags>(mainRasterMode));
    device.setDepthEnabled(true);
    light.updateCameraView();
    light.rebuildLightBuffers(device.getContext().Get());
    device.clearBackBuffer();
    gbuffer.clear(device.getContext().Get());
    gbuffer.setTargets(device.getContext().Get(), device.getDepthBuffer());
  }

  void HaboobWindow::render()
  {
    TracyD3D11Zone(tcyCtx, "D3DFrameScene");
    ZoneNamed(RenderScene, true);

    ID3D11DeviceContext* context = device.getContext().Get();

    // Shadowmap pass
    light.rebuildLightBuffers(device.getContext().Get());
    light.setTarget(context);
    scene.setCamera(&light.getCamera());
    scene.draw(context, false);

    // Full pass
    scene.setCamera(&mainCamera);
    scene.rebuildCameraBuffer(context);
    gbuffer.setTargets(device.getContext().Get(), device.getDepthBuffer());
    
    
    if (renderScene)
    {
      context->PSSetConstantBuffers(0, 1, light.getLightBuffer().GetAddressOf());
      scene.draw(context);
    }
  }

  void HaboobWindow::renderOverlay()
  {
    TracyD3D11Zone(tcyCtx, "D3DFrameOverlay");
    ZoneNamed(RenderOverlay, true);

    auto context = device.getContext().Get();

    // Basic lit pass
    gbuffer.lightPass(context, light.getLightBuffer().Get(), light.getLightPerspectiveBuffer().Get(), light.getShaderView(), light.getShadowSampler().Get());

    // Initial raymarch optimisation passes
    raymarchShader.optimiseRays(device, scene.getMeshRenderer(), gbuffer, XMLoadFloat3(&mainCamera.getPosition()));

    // Set up requirements for the proper pass
    scene.setCamera(&mainCamera);
    raymarchShader.setCameraBuffer(scene.getCameraBuffer());
    raymarchShader.setLightBuffer(light.getLightBuffer());
    raymarchShader.setTarget(&gbuffer.getLitColourTarget());

    // Raymarch!
    raymarchShader.bindShader(context, haboobVolume.getShaderView());
    raymarchShader.render(context);
    raymarchShader.unbindShader(context);
  }

  void HaboobWindow::renderMirror()
  {
    TracyD3D11Zone(tcyCtx, "D3DFrameMirror");
    ZoneNamed(RenderMirror, true);

    auto context = device.getContext().Get();

    // Set up states for copying
    device.setRasterState(DisplayDevice::RasterFlags::RASTER_STATE_DEFAULT);
    device.setDepthEnabled(false);
    RenderTarget::copyShader.setProjectionMatrix(device.getOrthoMatrix());

    // Copy from the raymarch texture output to the lit buffer
    raymarchShader.mirror(context);

    // Copy from lit buffer to backbuffer
    device.clearBackBuffer();
    device.setBackBufferTarget();
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
      ImVec2 dSize = ImVec2{ float(requiredWidth), float(requiredHeight) };
      ImGui::SetNextWindowSize(wSize);
      ImGui::GetIO().DisplaySize = dSize;

      ImGui_ImplDX11_InvalidateDeviceObjects();
    }
  }

  void HaboobWindow::renderGUI()
  {
    if (!showGUI) { return; }

    if (ImGui::Begin("DEBUGWINDOW", nullptr, ImGuiWindowFlags_::ImGuiWindowFlags_None))
    {
      ImGui::Text("FPS: %f", fps);
      ImGui::Text("Hello World");

      if (env)
      {
        env->getRoot().imguiGUIShow();
      }

      scene.imguiSceneTree();
    }
    ImGui::End();
  }

  HRESULT HaboobWindow::exportFrame()
  {
    return gbuffer.capture(exportLocation, device.getContext().Get());
  }

  void HaboobWindow::cameraOrbitStep(float dtConsidered)
  {
    if (!cameraOrbit) { return; }

    // Pan around a circular orbit utilising parametric eq of circle
    // Note: frame locked for consistent testing results

    XMVECTOR lookAtLoad = XMLoadFloat3(&orbitLookAt);
    XMVECTOR axisLoad = XMLoadFloat3(&orbitAxis);
    
    // Revolve
    float xAxisMag = orbitRadius * std::cos(orbitProgress);
    float yAxisMag = orbitRadius * std::sin(orbitProgress);

    // Determine coordinate system
    axisLoad = XMVector3Normalize(axisLoad);
    XMVECTOR localXAxis = XMVector3Cross(XMVectorSet(.5f, .5f, .5f, 1.f), axisLoad);
    localXAxis = XMVector3Normalize(localXAxis);
    XMVECTOR localYAxis = XMVector3Cross(localXAxis, axisLoad);
    localYAxis = XMVector3Normalize(localYAxis);

    // Compute camera look at and position
    XMVECTOR cameraPosition = XMVectorAdd(XMVectorScale(localXAxis, xAxisMag), XMVectorScale(localYAxis, yAxisMag));
    cameraPosition = XMVectorAdd(axisLoad, cameraPosition);
    XMVECTOR cameraForward = XMVectorSubtract(lookAtLoad, cameraPosition);

    // Best to directly overwrite to avoid euler angle shenanigans
    XMVECTOR up = XMVectorSet(.0f, 1.f, 0.f, 1.f);
    XMStoreFloat3(&mainCamera.getPosition(), cameraPosition);
    mainCamera.setView(XMMatrixLookToLH(cameraPosition, cameraForward, up));

    // Only progress w.r.t. delta if we are looking (otherwise a testing-stable environment is necessary)
    orbitProgress += showWindow ? orbitStep * dtConsidered : orbitStep;
  }

  void HaboobWindow::setupDefaults()
  {
    // Important configs
    exportLocation = L"test.dds";
    showWindow = true;
    dynamicResolution = true;
    outputFrame = false;
    exitAfterFrame = false;
    showGUI = true;
    requiredWidth = 256;
    requiredHeight = 256;

    // Controls
    mainCamera.getMoveRate() = 6.f;
    mainCamera.getPosition() = { -8.42f, .93f, -1.41f };
    mainCamera.getAngles() = { 1.36f, .1f, .0f };

    // Camera orbit (frame locked)
    cameraOrbit = false;
    orbitLookAt = { 0, 0, 0 };
    orbitAxis = { .0f, 1.f, 1.f };
    orbitRadius = 10.f;
    orbitStep = .011f;
    orbitProgress = .0f;
    orbitDiscreteProgress = 0;

    // Render toggles
    showDensity = false;
    showAngstrom = false;
    showSampleLevel = false;
    renderHaboob = false;
    renderScene = true;
    coneTrace = true;
    upscaleTracing = true;
    manualMarch = false;
    showBoundingBoxes = false;
    showMasks = false;
    showRayTravel = false;

    // Main rendering params
    mainRasterMode = DisplayDevice::RASTER_STATE_DEFAULT;
    gbuffer.getGamma() = .2f;
    gbuffer.getExposure() = 1.3f;

    {
      auto& lightPack = light.getLightData();
      lightPack.diffuse = { 3.96f, 3.92f, 3.14f };
      lightPack.ambient = { 0.96f, 0.92f, 0.14f };
      lightPack.direction = { -1.f, .25f, .0f, 1.f };
    }

    // Raymarch params
    {
      auto& opticsInfo = raymarchShader.getOpticsInfo();

      opticsInfo.anisotropicForwardTerms = { .735f, .732f, .651f, .735f };
      opticsInfo.anisotropicBackwardTerms = { -.735f, -.732f, -.651f, -.735f };
      opticsInfo.phaseBlendWeightTerms = { .2f, .2f, .2f, .2f, };
      opticsInfo.scatterAngstromExponent = 9.1f;

      opticsInfo.ambientFraction = { .8f, .8f, .8f, .8f, };
      opticsInfo.absorptionAngstromExponent = 34.1f;
      opticsInfo.powderCoefficient = .035f;
      opticsInfo.attenuationFactor = 24.f;
    }

    raymarchShader.getMarchInfo().iterations = 26;
  }

  void HaboobWindow::setupEnv(Environment* environment)
  {
    env = environment;

    auto& root = env->getRoot();
    auto& argRoot = *root.getArgGroup();

    {
      auto testGroup = (new EnvironmentGroup(new args::Group(argRoot, "Profiling"), false))->setName("Profiler");
      root.addChildGroup(testGroup);

      // Flags
      testGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Bool,
        new args::ValueFlag<bool>(*testGroup->getArgGroup(), "ShowWindow", "Toggles window display", { "sw" }), &showWindow)));
      testGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Bool,
        new args::ValueFlag<bool>(*testGroup->getArgGroup(), "OutputFrame", "Requests a frame to be saved to file", { "of" }), &outputFrame)));
      testGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Bool,
        new args::ValueFlag<bool>(*testGroup->getArgGroup(), "ExitFrame", "Exits after a single frame", { "eaf" }), &exitAfterFrame)));
      testGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Bool,
        new args::ValueFlag<bool>(*testGroup->getArgGroup(), "ShowGUI", "Toggles the GUI", { "sg" }), &showGUI)));
      testGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Int,
        new args::ValueFlag<int>(*testGroup->getArgGroup(), "Width", "The display width", { "w" }), &requiredWidth)));
      testGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Int,
        new args::ValueFlag<int>(*testGroup->getArgGroup(), "Height", "The display width", { "h" }), &requiredHeight)));
      testGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Bool,
        new args::ValueFlag<bool>(*testGroup->getArgGroup(), "DynamicResolution", "Should resolution be dynamic", { "dr" }), &dynamicResolution)));

      exportPathFlag = new args::ValueFlag<std::string>(*testGroup->getArgGroup(), "Output", "The output path", { "o" });
      testGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Symbolic, exportPathFlag)));

      // Await the external profiler before continuing
      testGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Symbolic, new args::ActionFlag(*testGroup->getArgGroup(), "AwaitProfiler", "The application should pause until the profiler connects", { "ap" }, [=]()
        {
          std::cout << "Awaiting Tracy connection... \n";
          while (!TracyIsConnected)
          {
            Sleep(1);
          }
          std::cout << "Hello Tracy! \n";
        }))));
    }

    {
      auto cameraGroup = (new EnvironmentGroup(new args::Group(argRoot, "Camera")))->setName("Camera");
      root.addChildGroup(cameraGroup);

      cameraGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float3, nullptr, &mainCamera.getPosition().x))
        ->setName("Camera Pos")
        ->setGUISettings(1.f, -10.f, 10.f));
      cameraGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float2, nullptr, &mainCamera.getAngles().x))
        ->setName("Camera Rot")
        ->setGUISettings(XM_PI * .01f, -XM_PI, XM_PI));
      cameraGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float, nullptr, &mainCamera.getMoveRate()))
        ->setName("Camera Speed")
        ->setGUISettings(1.f, .0f, .0f));
      cameraGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float, nullptr, &mainCamera.getMouseSensitivity()))
        ->setName("Camera Look Speed")
        ->setGUISettings(1.f, .0f, .0f));

      {
        auto orbitGroup = (new EnvironmentGroup(new args::Group(argRoot, "Orbit")))->setName("Orbit");
        cameraGroup->addChildGroup(orbitGroup);

        orbitGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Bool,
          new args::ValueFlag<bool>(*orbitGroup->getArgGroup(), "ShouldOrbit", "Toggles camera orbiting", { "so" }), &cameraOrbit))
          ->setName("Should Orbit"));
        orbitGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float,
          new args::ValueFlag<float>(*orbitGroup->getArgGroup(), "OrbitRadius", "Camera orbit radius", { "or" }), &orbitRadius))
          ->setName("Orbit Radius")
          ->setGUISettings(.1f, .0f, 10.f));
        orbitGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float,
          new args::ValueFlag<float>(*orbitGroup->getArgGroup(), "OrbitStep", "Camera orbit step per frame", { "os" }), &orbitStep))
          ->setName("Orbit Step")
          ->setGUISettings(.01f, -5.f, 5.f));
        orbitGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float3, nullptr, &orbitLookAt))
          ->setName("Orbit Look At")
          ->setGUISettings(.1f, -10.f, 10.f));
        orbitGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float3, nullptr, &orbitAxis))
          ->setName("Orbit Axis")
          ->setGUISettings(.1f, -10.f, 10.f));

        // Hidden
        orbitGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float,
          new args::ValueFlag<float>(*orbitGroup->getArgGroup(), "OrbitProgress", "Camera orbit progression (float)", { "opf" }), &orbitProgress, false))
          ->setName("Orbit Progress")
          ->setGUISettings(.01f, .0f, 10.f));
        orbitGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Int,
          new args::ValueFlag<UInt>(*orbitGroup->getArgGroup(), "OrbitDiscreteProgress", "Camera orbit progression (int)", { "opi" }), &orbitDiscreteProgress, false))
          ->setName("Orbit Discrete Progress")
          ->setGUISettings(.01f, 0, 100));
      }
    }

    {
      auto rasterGroup = (new EnvironmentGroup(new args::Group(argRoot, "Raster State")))->setName("Raster State");
      root.addChildGroup(rasterGroup);

      rasterGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Flags,
        new args::ValueFlag<UInt>(*rasterGroup->getArgGroup(), "RasterSolid", "If the rasteriser should render solid or wireframe", { "sw" }),
        &mainRasterMode))
        ->setName("Solid/Wireframe")
        ->setGUISettings((UInt)DisplayDevice::RASTER_FLAG_SOLID));
      rasterGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Flags,
        new args::ValueFlag<UInt>(*rasterGroup->getArgGroup(), "RasterCull", "If the rasteriser should cull at all", { "cl" }),
        &mainRasterMode))
        ->setName("Cull")
        ->setGUISettings((UInt)DisplayDevice::RASTER_FLAG_CULL));
      rasterGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Flags,
        new args::ValueFlag<UInt>(*rasterGroup->getArgGroup(), "RasterBackface", "The rasteriser should prefer backface culling", { "bf" }),
        &mainRasterMode))
        ->setName("Backface/Frontface")
        ->setGUISettings((UInt)DisplayDevice::RASTER_FLAG_BACK));
    }

    {
      auto renderToggleGroup = (new EnvironmentGroup(new args::Group(argRoot, "Render Toggles")))->setName("Render Toggles");
      root.addChildGroup(renderToggleGroup);

      renderToggleGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Bool,
        new args::ValueFlag<bool>(*renderToggleGroup->getArgGroup(), "ShowDensity", "If the renderer should output density", { "sd" }),
        &showDensity))
        ->setName("Show Density"));
      renderToggleGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Bool,
        new args::ValueFlag<bool>(*renderToggleGroup->getArgGroup(), "ShowAngstrom", "If the renderer should output the angstrom exponent", { "sa" }),
        &showAngstrom))
        ->setName("Show Angstrom"));
      renderToggleGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Bool,
        new args::ValueFlag<bool>(*renderToggleGroup->getArgGroup(), "ShowSampleLevel", "If the renderer should output the sample level range", { "ssl" }),
        &showSampleLevel))
        ->setName("Show Sample Level"));
      renderToggleGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Bool,
        new args::ValueFlag<bool>(*renderToggleGroup->getArgGroup(), "RenderScene", "If the renderer should render the scene to the GBuffer", { "rs" }),
        &renderScene))
        ->setName("Render Scene"));
      renderToggleGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Bool, nullptr, &renderHaboob))
        ->setName("Render Haboob"));
      renderToggleGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Bool,
        new args::ValueFlag<bool>(*renderToggleGroup->getArgGroup(), "ShowBoundingBoxes", "If the renderer should display all bounding boxes", { "sbb" }),
        &showBoundingBoxes))
        ->setName("Show Bounding Boxes"));
      renderToggleGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Bool,
        new args::ValueFlag<bool>(*renderToggleGroup->getArgGroup(), "ShowMasks", "If the renderer should display raymarch masks", { "smk" }),
        &showMasks))
        ->setName("Show Masks"));
      renderToggleGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Bool,
        new args::ValueFlag<bool>(*renderToggleGroup->getArgGroup(), "ShowRayTravel", "If the renderer should display the distance rays have travelled", { "srt" }),
        &showRayTravel))
        ->setName("Show Ray Travel"));
    }

    {
      auto lightGroup = (new EnvironmentGroup(new args::Group(argRoot, "Light")))->setName("Light");
      root.addChildGroup(lightGroup);

      auto& lightPack = light.getLightData();

      lightGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float3, nullptr, &lightPack.direction.x))
        ->setName("Light direction")
        ->setGUISettings(.01f, .0f, .0f));
      lightGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float3, nullptr, &lightPack.diffuse.x))
        ->setName("Light colour")
        ->setGUISettings(.1f, .0f, .0f));
      lightGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float3, nullptr, &lightPack.ambient.x))
        ->setName("Light ambient")
        ->setGUISettings(.1f, .0f, .0f));
      lightGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float, nullptr, &gbuffer.getGamma()))
        ->setName("Gamma")
        ->setGUISettings(.1f, .0f, .0f));
      lightGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float, nullptr, &gbuffer.getExposure()))
        ->setName("Exposure")
        ->setGUISettings(.1f, .0f, .0f));
    }

    {
      auto haboobGroup = (new EnvironmentGroup(new args::Group(argRoot, "HaboobGen")))->setName("Haboob");
      root.addChildGroup(haboobGroup);

      auto& volumeInfo = haboobVolume.getVolumeInfo();
      haboobGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Int3, nullptr, &volumeInfo.size))
        ->setName("Haboob Resolution")
        ->setGUISettings(1.f, 0, 1024));
      haboobGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::UInt4, nullptr, &volumeInfo.seed))
        ->setName("Haboob Seed"));
      haboobGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float, nullptr, &volumeInfo.worldSize))
        ->setName("Haboob World Size")
        ->setGUISettings(1.f, .0f, 10.f));

      haboobGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float, nullptr, &volumeInfo.octaves))
        ->setName("Haboob Octaves Size")
        ->setGUISettings(1.f, .1f, 8.1f));
      haboobGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float, nullptr, &volumeInfo.fractionalGap))
        ->setName("Haboob Fractional Gap")
        ->setGUISettings(1.f, .0f, 10.f));
      haboobGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float, nullptr, &volumeInfo.fractionalIncrement))
        ->setName("Haboob Fractional Increment")
        ->setGUISettings(1.f, .0f, 10.f));

      haboobGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float, nullptr, &volumeInfo.fbmOffset))
        ->setName("Haboob FBM Offset")
        ->setGUISettings(1.f, .0f, 10.f));
      haboobGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float, nullptr, &volumeInfo.fbmScale))
        ->setName("Haboob FBM Scale")
        ->setGUISettings(1.f, .0f, 10.f));

      haboobGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float, nullptr, &volumeInfo.wackyPower))
        ->setName("Haboob Wacky Power (tm)")
        ->setGUISettings(1.f, .0f, 10.f));
      haboobGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float, nullptr, &volumeInfo.wackyScale))
        ->setName("Haboob Wacky Scale (tm)")
        ->setGUISettings(1.f, .0f, 10.f));

      {
        auto haboobShapeGroup = (new EnvironmentGroup(new args::Group(argRoot, "HaboobShape")))->setName("HaboobShape");
        haboobGroup->addChildGroup(haboobShapeGroup);

        // Radial
        haboobShapeGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float, nullptr, &volumeInfo.radial.roofGradient))
          ->setName("Haboob Radial Roof Grad")
          ->setGUISettings(.01f, -10.f, 10.f));
        haboobShapeGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float, nullptr, &volumeInfo.radial.exponentialRate))
          ->setName("Haboob Radial Exponential Rate")
          ->setGUISettings(.01f, -10.f, 10.f));
        haboobShapeGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float, nullptr, &volumeInfo.radial.exponentialScale))
          ->setName("Haboob Radial Exponential Scale")
          ->setGUISettings(.01f, -10.f, 10.f));
        haboobShapeGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float, nullptr, &volumeInfo.radial.rOffset))
          ->setName("Haboob Radial Offset")
          ->setGUISettings(.01f, -10.f, 10.f));
        haboobShapeGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float, nullptr, &volumeInfo.radial.noseHeight))
          ->setName("Haboob Radial Nose Height")
          ->setGUISettings(.01f, -10.f, 10.f));
        haboobShapeGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float, nullptr, &volumeInfo.radial.blendHeight))
          ->setName("Haboob Radial Blend Height")
          ->setGUISettings(.01f, -10.f, 10.f));
        haboobShapeGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float, nullptr, &volumeInfo.radial.blendRate))
          ->setName("Haboob Radial Blend Rate")
          ->setGUISettings(.01f, -10.f, 10.f));

        // Distribution
        haboobShapeGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float, nullptr, &volumeInfo.distribution.falloffScale))
          ->setName("Haboob Dist. Falloff Scale")
          ->setGUISettings(.01f, -10.f, 10.f));
        haboobShapeGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float, nullptr, &volumeInfo.distribution.heightScale))
          ->setName("Haboob Dist. Height Scale")
          ->setGUISettings(.01f, -10.f, 10.f));
        haboobShapeGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float, nullptr, &volumeInfo.distribution.heightExponent))
          ->setName("Haboob Dist. Height Exponent")
          ->setGUISettings(.01f, -10.f, 10.f));
        haboobShapeGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float, nullptr, &volumeInfo.distribution.angleRange))
          ->setName("Haboob Dist. Angle Range")
          ->setGUISettings(.01f, -10.f, 10.f));
        haboobShapeGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float, nullptr, &volumeInfo.distribution.anglePower))
          ->setName("Haboob Dist. Angle Power")
          ->setGUISettings(.01f, -10.f, 10.f));
      }
    }

    {
      auto raymarchGroup = (new EnvironmentGroup(new args::Group(argRoot, "Raymarch")))->setName("Raymarch");
      root.addChildGroup(raymarchGroup);

      auto& marchInfo = raymarchShader.getMarchInfo();
      raymarchGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float, nullptr, &marchInfo.initialZStep))
        ->setName("Initial Step Size")
        ->setGUISettings(1.f, .0f, 100.f));
      raymarchGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float, nullptr, &marchInfo.marchZStep))
        ->setName("Step size")
        ->setGUISettings(1.f, .0f, 100.f));
      raymarchGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Int, 
        new args::ValueFlag<UInt>(*raymarchGroup->getArgGroup(), "SampleCount", "The number of samples per ray", { "it" }),
        &marchInfo.iterations))
        ->setName("Step count")
        ->setGUISettings(1.f, 0, 100));
      raymarchGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Bool, nullptr, &manualMarch))
        ->setName("Use manual step"));
      raymarchGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float, nullptr, &marchInfo.pixelRadius))
        ->setName("Pixel radius")
        ->setGUISettings(.0001f, .0f, 1.f));
      raymarchGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float, nullptr, &marchInfo.pixelRadiusDelta))
        ->setName("Pixel radius z-delta")
        ->setGUISettings(.0001f, .0f, 1.f));
    }

    {
      auto opticsGroup = (new EnvironmentGroup(new args::Group(argRoot, "Optics")))->setName("Optics");
      root.addChildGroup(opticsGroup);

      auto& opticsInfo = raymarchShader.getOpticsInfo();

      // Phase
      opticsGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float4, nullptr, &opticsInfo.anisotropicForwardTerms.x))
        ->setName("Henyey-Greenstein anisotropy (f)")
        ->setGUISettings(.1f, .0f, 2.f));
      opticsGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float4, nullptr, &opticsInfo.anisotropicBackwardTerms.x))
        ->setName("Henyey-Greenstein anisotropy (b)")
        ->setGUISettings(.1f, .0f, 2.f));
      opticsGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float4, nullptr, &opticsInfo.phaseBlendWeightTerms.x))
        ->setName("Phase blend")
        ->setGUISettings(.01f, .0f, 1.f));
      opticsGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float, nullptr, &opticsInfo.scatterAngstromExponent))
        ->setName("Scattering angstrom exponent")
        ->setGUISettings(1.f, .0f, 100.f));

      // Transmission
      opticsGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float, nullptr, &opticsInfo.absorptionAngstromExponent))
        ->setName("Absorption angstrom exponent")
        ->setGUISettings(1.f, .0f, 100.f));
      opticsGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float, nullptr, &opticsInfo.powderCoefficient))
        ->setName("Beers Powder coefficient")
        ->setGUISettings(.001f, .0f, 1.f));
      opticsGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float, nullptr, &opticsInfo.attenuationFactor))
        ->setName("Attenuation scale")
        ->setGUISettings(1.f, .0f, 100.f));
      opticsGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Float4, nullptr, &opticsInfo.ambientFraction.x))
        ->setName("Ambient fraction")
        ->setGUISettings(.01f, .0f, 1.f));

      // Flags
      opticsGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Flags, nullptr, &opticsInfo.flagApplyBeer))
        ->setName("Apply beer")
        ->setGUISettings(~0));
      opticsGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Flags, nullptr, &opticsInfo.flagApplyHG))
        ->setName("Apply HG")
        ->setGUISettings(~0));
      opticsGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Flags, nullptr, &opticsInfo.flagApplySpectral))
        ->setName("Apply Spectral")
        ->setGUISettings(~0));
      opticsGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Bool, 
        new args::ValueFlag<bool>(*opticsGroup->getArgGroup(), "ApplyConeTrace", "If cone tracing should be used for anti-aliasing", { "uct" }), 
        &coneTrace))
        ->setName("Apply Cone Tracing"));
      opticsGroup->addVariable((new EnvironmentVariable(EnvironmentVariable::Type::Bool,
        new args::ValueFlag<bool>(*opticsGroup->getArgGroup(), "ApplyUpscaleTrace", "If upscaling should be used for raymarching", { "uqt" }),
        &upscaleTracing))
        ->setName("Apply Upscale Tracing"));
    }
  }
  void HaboobWindow::show()
  {
    if (showWindow)
    {
      Window::show();
    }
    else
    {
      std::cout << "Display window hidden\n";
    }
  }
}