#pragma once
#include "Rendering/D3DCore.h"
#include "SceneStructs.h"

namespace Haboob
{
  class Camera
  {
    public:
    Camera();

    void setProjection(XMMATRIX& matrix);
    void setView(XMMATRIX& matrix);
    inline void setWorld(const XMMATRIX& matrix) { transformations.worldMatrix = matrix; }
    inline const XMMATRIX& getWorld() const { return transformations.worldMatrix; }
    inline const XMMATRIX& getProjection() const { return transformations.projectionMatrix; }
    inline const XMMATRIX& getView() const { return transformations.viewMatrix; }

    // Directly copies the optimised matrices to the address
    void putPack(void* dest) const;

    private:
    void recomputeInverseViewProjection();

    CameraPack transformations;
  };
}