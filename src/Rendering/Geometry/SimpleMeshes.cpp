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
      {{scale, -scale, -scale}, {1.f, .0f}, {.0f, .0f, -1.f}},
      {{scale, scale, -scale}, {1.f, 1.f}, {.0f, .0f, -1.f}},
      {{-scale, scale, -scale}, {.0f, 1.f}, {.0f, .0f, -1.f}},
      // Bottom
      {{scale, -scale, -scale}, {.0f, .0f}, {.0f, -1.f, 0.f}},
      {{-scale, -scale, -scale}, {1.f, .0f}, {.0f, -1.f, 0.f}},
      {{-scale, -scale, scale}, {1.f, 1.f}, {.0f, -1.f, 0.f}},
      {{scale, -scale, scale}, {.0f, 1.f}, {.0f, -1.f, 0.f}},
      // Right
      {{scale, -scale, -scale}, {.0f, .0f}, {1.f, 0.f, 0.f}},
      {{scale, -scale, scale}, {1.f, .0f}, {1.f, 0.f, 0.f}},
      {{scale, scale, scale}, {1.f, 1.f}, {1.f, 0.f, 0.f}},
      {{scale, scale, -scale}, {.0f, 1.f}, {1.f, 0.f, 0.f}},
      // Front
      {{-scale, -scale, scale}, {.0f, .0f}, {.0f, .0f, 1.f}},
      {{scale, -scale, scale}, {1.f, .0f}, {.0f, .0f, 1.f}},
      {{scale, scale, scale}, {1.f, 1.f}, {.0f, .0f, 1.f}},
      {{-scale, scale, scale}, {.0f, 1.f}, {.0f, .0f, 1.f}},
      // Top
      {{scale, scale, -scale}, {.0f, .0f}, {.0f, 1.f, 0.f}},
      {{-scale, scale, -scale}, {1.f, .0f}, {.0f, 1.f, 0.f}},
      {{-scale, scale, scale}, {1.f, 1.f}, {.0f, 1.f, 0.f}},
      {{scale, scale, scale}, {.0f, 1.f}, {.0f, 1.f, 0.f}},
      // Left
      {{-scale, -scale, -scale}, {.0f, .0f}, {-1.f, 0.f, 0.f}},
      {{-scale, -scale, scale}, {1.f, .0f}, {-1.f, 0.f, 0.f}},
      {{-scale, scale, scale}, {1.f, 1.f}, {-1.f, 0.f, 0.f}},
      {{-scale, scale, -scale}, {.0f, 1.f}, {-1.f, 0.f, 0.f}}
    };

    static const std::vector<ULong> indices =
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
  HRESULT SimplePlaneMesh::build(ID3D11Device* device)
  {
    static const float scale = .5f;
    static const std::vector<VertexType> vertices =
    {
      {{-scale, -scale, .0f}, {.0f, 1.f}, {.0f, .0f, -1.f}},
      {{scale, -scale, .0f}, {1.f, 1.f}, {.0f, .0f, -1.f}},
      {{scale, scale, .0f}, {1.f, .0f}, {.0f, .0f, -1.f}},
      {{-scale, scale, .0f}, {.0f, .0f}, {.0f, .0f, -1.f}}
    };

    static const std::vector<ULong> indices =
    {
      0, 1, 2,
      2, 3, 0
    };

    return buildBuffers(device, vertices, indices);
  }


  HRESULT SimpleSphereMesh::build(ID3D11Device* device, ULong segments, ULong rings)
  {
    const float radius = .5f * sqrt(3.f); // Fit to a box
    const float xStep = 1.f / (float)segments;
    const float yStep = 1.f / (float)rings;
    const float xAngularStep = XM_2PI / (float)segments;
    const float yAngularStep = XM_PI / (float)rings;

    std::vector<VertexType> vertices;
    for (ULong y = 0; y <= rings; ++y)
    {
      for (ULong x = 0; x <= segments; ++x)
      {
        float xF = float(x);
        float yF = float(y);

        VertexType vertex;
        // Normal is just the normalised position
        vertex.normal = sphericalToCartesian(xAngularStep * xF, yAngularStep * yF);
        vertex.position = { vertex.normal.x * radius, vertex.normal.y * radius, vertex.normal.z * radius };
        vertex.texture = {xStep * xF, 1.f - yStep * yF };

        vertices.push_back(vertex);
      }
    }

    // Build indice list from surface coordinate space
    std::vector<ULong> indices;
    for (ULong x = 0; x < segments; ++x)
    {
      for (ULong y = 0; y < rings; ++y)
      {
        // Right triangle
        indices.push_back(surfaceCoordToIndice(x + 1, y + 1, segments));
        indices.push_back(surfaceCoordToIndice(x + 1, y, segments));
        indices.push_back(surfaceCoordToIndice(x, y, segments));

        // Left triangle
        indices.push_back(surfaceCoordToIndice(x, y, segments));
        indices.push_back(surfaceCoordToIndice(x, y + 1, segments));
        indices.push_back(surfaceCoordToIndice(x + 1, y + 1, segments));
      }
    }

    return buildBuffers(device, vertices, indices);
  }

  XMFLOAT3 SimpleSphereMesh::sphericalToCartesian(float x, float y) const
  {
    const float sinDelta = sinf(y); // Cache result that is used twice
    return XMFLOAT3{ cosf(x) * sinDelta, cosf(y), sinf(x) * sinDelta }; // To Cartesian!
  }
}