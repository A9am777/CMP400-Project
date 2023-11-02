#pragma once
#include "Mesh.h"
#include "GeometryStructs.h"
#include "Data/Defs.h"

namespace Haboob
{
  class SimpleCubeMesh : public Mesh<VertexType>
  {
    public:
    SimpleCubeMesh() = default;

    HRESULT build(ID3D11Device* device);
  };

  class SimplePlaneMesh : public Mesh<VertexType>
  {
    public:
    SimplePlaneMesh() = default;

    HRESULT build(ID3D11Device* device);
  };

  class SimpleSphereMesh : public Mesh<VertexType>
  {
    public:
    SimpleSphereMesh() = default;

    HRESULT build(ID3D11Device* device, ULong segments = 32, ULong rings = 32);

    private:
    inline ULong surfaceCoordToIndice(ULong x, ULong y, ULong segments) const { return y * (segments + 1) + x; }
    XMFLOAT3 sphericalToCartesian(float x, float y) const;
  };
}