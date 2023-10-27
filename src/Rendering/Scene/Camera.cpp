#include "Rendering/Scene/Camera.h"

namespace Haboob
{
  Camera::Camera()
  {
    transformations.worldMatrix = XMMatrixIdentity();
    transformations.projectionMatrix = XMMatrixIdentity();
    transformations.viewMatrix = XMMatrixIdentity();
  }

  void Camera::putPack(void* dest) const
  {
    memcpy(dest, &transformations, sizeof(CameraPack));
  }
}