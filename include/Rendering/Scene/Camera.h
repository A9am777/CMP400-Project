#pragma once
#include "Rendering/D3DCore.h"
#include "SceneStructs.h"

namespace Haboob
{
  class Camera
  {
    public:
    Camera();

    inline void setWorld(const XMMATRIX& matrix) { transformations.worldMatrix = matrix; }
    inline void setProjection(const XMMATRIX& matrix) { transformations.projectionMatrix = matrix; }
    inline void setView(const XMMATRIX& matrix) { transformations.viewMatrix = matrix; }
    inline const XMMATRIX& getWorld() const { return transformations.worldMatrix; }
    inline const XMMATRIX& getProjection() const { return transformations.projectionMatrix; }
    inline const XMMATRIX& getView() const { return transformations.viewMatrix; }

    // Directly copies the optimised matrices to the address
    void putPack(void* dest) const;

    private:
    CameraPack transformations;
  };
}