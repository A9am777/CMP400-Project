#pragma once

#include <Window.h>
#include <VKeys.h>
#include <MousePointer.h>
#include <imgui.h>
#include "Data/EnvironmentArgs.h"
#include "Rendering/D3DCore.h"
#include "Rendering/DisplayDevice.h"
#include "Rendering/Shaders/ShaderManager.h"
#include "Rendering/Geometry/SimpleMeshes.h"
#include "Rendering/Scene/FreeCam.h"
#include "Rendering/Lighting/LightStructs.h"
#include "Rendering/Textures/RenderTarget.h"
#include "Rendering/Shaders/RaymarchVolumeShader.h"
#include "Rendering/Textures/GBuffer.h"
#include "Rendering/Scene/Scene.h"
#include "Rendering/Lighting/LightSource.h"

#include <tracy/Tracy.hpp>
#include <tracy/TracyD3D11.hpp>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Haboob
{
  class HaboobWindow : public WSTR::Window
  {
    using Clock = std::chrono::steady_clock;
    using Precision = std::micro;

    public:
    HaboobWindow();
    ~HaboobWindow();
    
    void setupEnv(Environment* environment);

    void show();

    virtual void onStart() override;
    virtual void main() override;
    virtual void onEnd() override;
    virtual void onResize() override;
    virtual void onKeyDown(unsigned char vKey) override;
    virtual void onKeyUp(unsigned char vKey) override;
    virtual void onMouseMove(short x, short y) override;
    virtual void onMove() override;

    virtual void input(float dt);
    virtual void update(float dt);
    virtual void render();

    protected:
    void createD3D();
    void adjustProjection();

    LRESULT customRoutine(UINT message, WPARAM wParam, LPARAM lParam) override;

    void setupDefaults();

    // Captures the backbuffer and saves to file
    HRESULT exportFrame(); // Slow

    // Steps the camera orbit
    void cameraOrbitStep(float dtConsidered);

    void renderBegin();
    void renderOverlay(); // Raymarch environment
    void renderMirror(); // Copies from the gbuffer to the back buffer after applying post process effects

    void imguiStart();
    void imguiEnd();
    void imguiFrameBegin();
    void imguiFrameEnd();
    void imguiFrameResize();

    void renderGUI();

    // Environment
    Environment* env;
    tracy::D3D11Ctx* tcyCtx;

    // Top level args
    args::ValueFlag<std::string>* exportPathFlag;
    std::wstring exportLocation;
    bool showWindow; // Window should be displayed
    bool dynamicResolution; // Scale with window?
    bool outputFrame; // Saves image to file
    bool exitAfterFrame; // Exits after the first frame
    bool showGUI;
    int requiredWidth;
    int requiredHeight;

    // Camera orbit (frame locked)
    bool cameraOrbit;
    XMFLOAT3 orbitLookAt;
    XMFLOAT3 orbitAxis;
    float orbitRadius;
    float orbitStep; // In rads
    float orbitProgress; // In rads
    int orbitDiscreteProgress; // Frames passed (external input)

    // Render toggles
    bool showDensity;
    bool showAngstrom;
    bool showSampleLevel;
    bool renderHaboob;
    bool renderScene;
    bool coneTrace;
    bool manualMarch;
    bool showBoundingBoxes;
    bool showMasks;
    bool showRayTravel;

    // Rendering device
    DisplayDevice device;

    // Input handlers
    WSTR::VKeys keys;
    WSTR::MousePointer mouse;

    // Meshes/resources
    SimplePlaneMesh planeMesh;
    SimpleSphereMesh sphereMesh;
    SimpleCubeMesh cubeMesh;
    VolumeGenerationShader haboobVolume;

    // Scene objects
    Scene scene;
    Light light;
    FreeCam mainCamera;
    
    Shader* deferredVertexShader;
    Shader* deferredPixelShader;

    // Main rendering environment
    GBuffer gbuffer;
    RaymarchVolumeShader raymarchShader;
    ShaderManager shaderManager;

    private:
    Clock::time_point lastFrame;

    // ImGui
    float fps;
    UInt mainRasterMode;
    ImGuiContext* imgui;
  };
}