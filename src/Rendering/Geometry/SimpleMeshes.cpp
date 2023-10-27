#include "Rendering/Geometry/SimpleMeshes.h"

namespace Haboob
{
  HRESULT SimpleCubeMesh::build(ID3D11Device* device)
  {
    static const float scale = .5f;
    static const std::vector<VertexType> vertices =
    {
      // Back
      {{-scale, -scale, -scale}, {.0f, .0f}, {.0f, .0f, -1.f}},
      {{scale, -scale, -scale}, {.0f, .0f}, {.0f, .0f, -1.f}},
      {{scale, scale, -scale}, {.0f, .0f}, {.0f, .0f, -1.f}},
      {{-scale, scale, -scale}, {.0f, .0f}, {.0f, .0f, -1.f}},
      // Bottom
      {{-scale, -scale, -scale}, {.0f, .0f}, {.0f, -1.f, 0.f}},
      {{scale, -scale, -scale}, {.0f, .0f}, {.0f, -1.f, 0.f}},
      {{scale, -scale, scale}, {.0f, .0f}, {.0f, -1.f, 0.f}},
      {{-scale, -scale, scale}, {.0f, .0f}, {.0f, -1.f, 0.f}},
      // Right
      {{scale, -scale, -scale}, {.0f, .0f}, {1.f, 0.f, 0.f}},
      {{scale, -scale, scale}, {.0f, .0f}, {1.f, 0.f, 0.f}},
      {{scale, scale, scale}, {.0f, .0f}, {1.f, 0.f, 0.f}},
      {{scale, scale, -scale}, {.0f, .0f}, {1.f, 0.f, 0.f}},
      // Front
      {{-scale, -scale, scale}, {.0f, .0f}, {.0f, .0f, 1.f}},
      {{scale, -scale, scale}, {.0f, .0f}, {.0f, .0f, 1.f}},
      {{scale, scale, scale}, {.0f, .0f}, {.0f, .0f, 1.f}},
      {{-scale, scale, scale}, {.0f, .0f}, {.0f, .0f, 1.f}},
      // Top
      {{-scale, scale, -scale}, {.0f, .0f}, {.0f, 1.f, 0.f}},
      {{scale, scale, -scale}, {.0f, .0f}, {.0f, 1.f, 0.f}},
      {{scale, scale, scale}, {.0f, .0f}, {.0f, 1.f, 0.f}},
      {{-scale, scale, scale}, {.0f, .0f}, {.0f, 1.f, 0.f}},
      // Left
      {{-scale, -scale, -scale}, {.0f, .0f}, {-1.f, 0.f, 0.f}},
      {{-scale, -scale, scale}, {.0f, .0f}, {-1.f, 0.f, 0.f}},
      {{-scale, scale, scale}, {.0f, .0f}, {-1.f, 0.f, 0.f}},
      {{-scale, scale, -scale}, {.0f, .0f}, {-1.f, 0.f, 0.f}}
    };

    static const std::vector<unsigned long> indices =
    {
      // Back
      0, 1, 2,
      2, 3, 0,
      // Bottom
      4, 5, 6,
      6, 7, 4,
      // Right
      8, 9, 10,
      10, 11, 8,
      // Front
      14, 13, 12,
      12, 15, 14,
      // Top
      18, 17, 16,
      16, 19, 18,
      // Left
      22, 21, 20,
      20, 23, 22,
    };

    return buildBuffers(device, vertices, indices);
  }
}