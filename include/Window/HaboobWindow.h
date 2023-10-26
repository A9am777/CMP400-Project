#pragma once

#include <Window.h>
#include <VKeys.h>
#include <MousePointer.h>
#include <imgui.h>
#include "Rendering/D3DCore.h"
#include "Rendering/DisplayDevice.h"
#include "Rendering/Shaders/Shader.h"

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
    virtual void onKeyDown(char vKey) override;
    virtual void onKeyUp(char vKey) override;
    virtual void onMouseMove(short x, short y) override;
    virtual void onMove() override;

    virtual void input();
    virtual void update(float dt);
    virtual void render();

    protected:
    DisplayDevice device;

    WSTR::VKeys keys;
    WSTR::MousePointer mouse;

    void createD3D();

    void imguiStart();
    void imguiEnd();
    void imguiFrameBegin();
    void imguiFrameEnd();
    void imguiFrameResize();
    void imguiKeyDown(char vKey);
    void imguiKeyUp(char vKey);
    void imguiMouseMove(short x, short y);
    void imguiMove();

    void renderTestGUI();

    Shader* testShader;
    ShaderManager shaderManager;
    private:
    Clock::time_point lastFrame;

    ImGuiContext* imgui;
  };
}