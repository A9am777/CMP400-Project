#pragma once

#include <Window.h>
#include <VKeys.h>
#include <MousePointer.h>
#include <imgui.h>
#include "Rendering/D3DCore.h"
#include "Rendering/RDevice.h"

namespace Haboob
{
  class HaboobWindow : public WSTR::Window
  {
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
    virtual void render(float dt);

    protected:
    DVF::RDevice device;

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

    private:
    ImGuiContext* imgui;
  };
}