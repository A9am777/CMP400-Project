#include "Rendering/Scene/FreeCam.h"

namespace Haboob
{
  FreeCam::FreeCam()
  {
    position = {.0f, .0f, .0f };
    eulerAngles = { .0f, .0f, .0f };

    mouseSensitivity = .1f;
    lookRate = XM_PIDIV2;
    moveRate = 1.f;
  }

  void FreeCam::update(float dt, WSTR::VKeys* keys, WSTR::MousePointer* mouse)
  {
    // Resolve instantaneous rotation
    if (mouse->isMoved() && keys->isKeyDown(VK_RBUTTON))
    {
      const float fullLookRate = lookRate * mouseSensitivity * dt;
      eulerAngles.x += float(mouse->getDX()) * fullLookRate;
      eulerAngles.y += float(mouse->getDY()) * fullLookRate;
    }

    // Resolve local axes
    XMMATRIX directionMat = XMMatrixRotationRollPitchYaw(eulerAngles.y, eulerAngles.x, eulerAngles.z);
    XMVECTOR forward = XMVectorSet(.0f, .0f, 1.f, 1.f);
    XMVECTOR up = XMVectorSet(.0f, 1.f, 0.f, 1.f);

    forward = XMVector3TransformCoord(forward, directionMat);
    up = XMVector3TransformCoord(up, directionMat);
    XMVECTOR right = XMVector3Cross(up, forward);

    // Resolve translation
    XMVECTOR positionLoad = XMLoadFloat3(&position);
    {
      // This intermediate will store input translation before being converted and decomposed
      XMFLOAT3 relativeTranslation = { .0f, .0f, .0f };

      if (keys->isKeyDown('W') || keys->isKeyDown(VK_UP))
      {
        relativeTranslation.z += 1.f;
      }

      if (keys->isKeyDown('A') || keys->isKeyDown(VK_LEFT))
      {
        relativeTranslation.x += -1.f;
      }

      if (keys->isKeyDown('S') || keys->isKeyDown(VK_DOWN))
      {
        relativeTranslation.z += -1.f;
      }

      if (keys->isKeyDown('D') || keys->isKeyDown(VK_RIGHT))
      {
        relativeTranslation.x += 1.f;
      }

      if (keys->isKeyDown('Q') || keys->isKeyDown(VK_OEM_COMMA))
      {
        relativeTranslation.y += 1.f;
      }

      if (keys->isKeyDown('E') || keys->isKeyDown(VK_OEM_PERIOD))
      {
        relativeTranslation.y += -1.f;
      }

      XMVECTOR translationLoad = XMLoadFloat3(&relativeTranslation);
      translationLoad = XMVector3Normalize(translationLoad); // Only a set velocity is allowed
      translationLoad = translationLoad * moveRate * dt; // Frame velocity to distance

      // Decompose and add to position
      positionLoad = positionLoad + right * XMVectorGetX(translationLoad);
      positionLoad = positionLoad + up * XMVectorGetY(translationLoad);
      positionLoad = positionLoad + forward * XMVectorGetZ(translationLoad);

      // Echo position back
      XMStoreFloat3(&position, positionLoad);
    }

    // Finally, resolve the view matrix
    setView(XMMatrixLookToLH(positionLoad, forward, up));
  }
}
