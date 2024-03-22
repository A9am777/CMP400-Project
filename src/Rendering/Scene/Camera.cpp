#include "Rendering/Scene/Camera.h"

namespace Haboob
{
  Camera::Camera()
  {
    transformations.worldMatrix             = 
    transformations.projectionMatrix        = 
    transformations.inverseProjectionMatrix =
    transformations.viewMatrix              = XMMatrixIdentity();
  }

  void Camera::setProjection(const XMMATRIX& matrix)
  {
    transformations.projectionMatrix = matrix;
    transformations.inverseProjectionMatrix = XMMatrixInverse(nullptr, matrix);

    recomputeInverseViewProjection();
  }

  void Camera::setView(const XMMATRIX& matrix)
  {
    transformations.viewMatrix = matrix;

    recomputeInverseViewProjection();
  }

  void Camera::putPack(void* dest) const
  {
    memcpy(dest, &transformations, sizeof(CameraPack));
  }

  void Camera::recomputeInverseViewProjection()
  {
    // TODO: view matrix inverse can be computed quickly
    transformations.inverseViewProjectionMatrix = XMMatrixInverse(nullptr, transformations.viewMatrix * transformations.projectionMatrix);
  }
}