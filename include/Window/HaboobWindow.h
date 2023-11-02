#pragma once

#include <Window.h>
#include <VKeys.h>
#include <MousePointer.h>
#include <imgui.h>
#include "Rendering/D3DCore.h"
#include "Rendering/DisplayDevice.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Geometry/SimpleMeshes.h"
#include "Rendering/Scene/FreeCam.h"
#include "Rendering/Textures/RenderTarget.h"

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
    DisplayDevice device;

    WSTR::VKeys keys;
    WSTR::MousePointer mouse;

    void createD3D();
    void adjustProjection();

    LRESULT customRoutine(UINT message, WPARAM wParam, LPARAM lParam) override;

    void renderBegin();
    void renderOverlay(); // Raymarch environment
    void renderMirror(); // Copies from mainRender to the back buffer

    void imguiStart();
    void imguiEnd();
    void imguiFrameBegin();
    void imguiFrameEnd();
    void imguiFrameResize();

    void renderTestGUI();

    SimpleSphereMesh testMesh;
    Shader* testVertexShader;
    Shader* testPixelShader;
    Shader* testComputeShader;
    ShaderManager shaderManager;

    // TEST
    XMMATRIX projectionMatrix;
    XMMATRIX worldMatrix;
    XMMATRIX viewMatrix;
    FreeCam camTest;
    RenderTarget mainRender;
    ComPtr<ID3D11Buffer> cameraBuffer;
    private:
    Clock::time_point lastFrame;

    // ImGui
    float cubePos[3] = {.0f, .0f, 2.5f};
    UInt mainRasterMode;
    ImGuiContext* imgui;
  };
}