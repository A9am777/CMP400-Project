#pragma once

#include "Rendering/D3DCore.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Geometry/SimpleMeshes.h"
#include "Rendering/Scene/SceneStructs.h"

namespace Haboob
{
  // Copies texels from a texture to a target
  class RenderCopyShader
  {
    public:
    RenderCopyShader();
    ~RenderCopyShader();

    HRESULT initShader(ID3D11Device* device, ShaderManager* manager);
    void bindShader(ID3D11DeviceContext* context);

    void render(ID3D11DeviceContext* context) const;

    void setViewport(const D3D11_VIEWPORT& view);

    inline void setProjectionMatrix(const XMMATRIX& matrix) { matrices.projectionMatrix = matrix; }
    inline void setViewMatrix(const XMMATRIX& matrix) { matrices.viewMatrix = matrix; }
    inline void setWorldMatrix(const XMMATRIX& matrix) { matrices.worldMatrix = matrix; }

    ComPtr<ID3D11SamplerState> getSampler() { return sampler; }

    private:
    Shader* vertexShader;
    Shader* pixelShader;
    SimplePlaneMesh quad;
    CameraPack matrices;
    ComPtr<ID3D11Buffer> cameraBuffer;
    ComPtr<ID3D11SamplerState> sampler;
  };
}