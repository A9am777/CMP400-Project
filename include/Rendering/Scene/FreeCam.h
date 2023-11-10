#pragma once
#include "Rendering/Scene/Camera.h"
#include <VKeys.h>
#include <MousePointer.h>

namespace Haboob
{
  class FreeCam : public Camera
  {
    public:
    FreeCam();

    void update(float dt, WSTR::VKeys* keys, WSTR::MousePointer* mouse);

    inline XMFLOAT3& getPosition() { return position; }
    inline XMFLOAT3& getAngles() { return eulerAngles; }
    inline float& getMoveRate() { return moveRate; }
    inline float& getMouseSensitivity() { return mouseSensitivity; }

    private:
    float mouseSensitivity;
    float lookRate; // ms^-1
    float moveRate; // rad s^-1
    XMFLOAT3 position;
    XMFLOAT3 eulerAngles; // Prefered over quaternion for user control (yaw, pitch, roll)
  };
}