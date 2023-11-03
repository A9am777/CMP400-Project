#pragma once

namespace Haboob
{
  struct DirectionalLightPack
  {
    XMFLOAT3 diffuse = { 1.f, 1.f, 1.f };
    float padding1;

    XMFLOAT3 ambient;
    float padding2;

    XMFLOAT3 specular;
    float padding3;

    XMFLOAT4 direction = {0.f, -1.f, -1.f, 1.f};
  };
}