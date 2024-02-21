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
  };
}