#pragma once
#include "Rendering/D3DCore.h"
#include "SceneStructs.h"

namespace Haboob
{
  class Camera
  {
    public:
    Camera();

    inline void setWorld(const XMMATRIX& matrix) { transformations.worldMatrix = XMMatrixTranspose(matrix); }
    inline void setProjection(const XMMATRIX& matrix) { transformations.projectionMatrix = XMMatrixTranspose(matrix); }
    inline void setView(const XMMATRIX& matrix) { transformations.viewMatrix = XMMatrixTranspose(matrix); }
    inline const XMMATRIX& getWorld() const { return XMMatrixTranspose(transformations.worldMatrix); }
    inline const XMMATRIX& getProjection() const { return XMMatrixTranspose(transformations.projectionMatrix); }
    inline const XMMATRIX& getView() const { return XMMatrixTranspose(transformations.viewMatrix); }

    // Directly copies the optimised matrices to the address
    void putPack(void* dest) const;

    private:
    CameraPack transformations;
  };
}