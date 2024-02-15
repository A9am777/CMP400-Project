#pragma once

#include <Window.h>
#include <VKeys.h>
#include <MousePointer.h>
#include <imgui.h>
#include "Rendering/D3DCore.h"
#include "Rendering/DisplayDevice.h"
#include "Rendering/Shaders/ShaderManager.h"
#include "Rendering/Geometry/SimpleMeshes.h"
#include "Rendering/Scene/FreeCam.h"
#include "Rendering/Lighting/LightStructs.h"
#include "Rendering/Textures/RenderTarget.h"
#include "Rendering/Shaders/RaymarchVolumeShader.h"
#include "Rendering/Textures/GBuffer.h"

#ifdef NV_PERF_ENABLE_INSTRUMENTATION
#include "NvPerfReportGeneratorD3D11.h"
#endif

#ifdef NV_PERF_ENABLE_INSTRUMENTATION
#include "nvperf_host_impl.h"
#include "NvPerfHudRenderer.h"
#include "NvPerfD3D11.h"
#include "NvPerfHudImPlotRenderer.h"
#include "NvPerfPeriodicSamplerGpu.h"
#endif

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

    // Strictly for testing purposes - forces the camera buffer to be resent
    void redoCameraBuffer(ID3D11DeviceContext* context);

    void renderBegin();
    void renderOverlay(); // Raymarch environment
    void renderMirror(); // Copies from the gbuffer to the back buffer after applying post process effects

    void imguiStart();
    void imguiEnd();
    void imguiFrameBegin();
    void imguiFrameEnd();
    void imguiFrameResize();

    void hudShenanigans();

    void renderGUI();

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
    DirectionalLightPack dirLightPack;
    FreeCam mainCamera;
    
    Shader* deferredVertexShader;
    Shader* deferredPixelShader;

    // Main rendering environment
    GBuffer gbuffer;
    RaymarchVolumeShader raymarchShader;
    ShaderManager shaderManager;
    
    // Reusable buffers
    ComPtr<ID3D11Buffer> cameraBuffer;
    ComPtr<ID3D11Buffer> lightBuffer;
    private:
    Clock::time_point lastFrame;

    // ImGui
    float fps;
    float spherePos[3] = {.0f, .0f, 2.5f};
    UInt mainRasterMode;
    ImGuiContext* imgui;

    #ifdef NV_PERF_ENABLE_INSTRUMENTATION
    nv::perf::profiler::ReportGeneratorD3D11 g_nvperf;
    NVPW_Device_ClockStatus g_clockStatus = NVPW_DEVICE_CLOCK_STATUS_UNKNOWN; // Used to restore clock state when exiting
    const ULONGLONG g_warmupTicks = 500u; /* milliseconds */
    ULONGLONG g_startTicks = 0u;
    ULONGLONG g_currentTicks = 0u;
    nv::perf::sampler::GpuPeriodicSampler m_sampler;
    nv::perf::hud::HudDataModel m_hudDataModel;
    nv::perf::hud::HudImPlotRenderer m_hudRenderer;
    #endif
  };
}