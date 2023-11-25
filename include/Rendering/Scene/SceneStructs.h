#pragma once
#include "Rendering/D3DCore.h"

namespace Haboob
{
  struct CameraPack
  {
    XMMATRIX worldMatrix;
    XMMATRIX viewMatrix;
    XMMATRIX projectionMatrix;
    XMMATRIX inverseProjectionMatrix;
    XMMATRIX inverseViewProjectionMatrix;
  };
}