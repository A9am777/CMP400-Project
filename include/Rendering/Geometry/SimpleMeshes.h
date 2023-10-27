#pragma once
#include "Mesh.h"
#include "GeometryStructs.h"

namespace Haboob
{
  class SimpleCubeMesh : public Mesh<VertexType>
  {
    public:
    SimpleCubeMesh() = default;

    HRESULT build(ID3D11Device* device);
  };
}