#pragma once

#include "Data/Defs.h"
#include "Rendering/Scene/Camera.h"
#include "Rendering/Scene/FreeCam.h"
#include "Rendering/Lighting/LightStructs.h"

#include <cstring>

namespace Haboob
{
  // Refers to a directional light source in the scope of this application
  class Light
  {
    public:
    Light();

    HRESULT create(ID3D11Device* device, UInt width, UInt height);
    HRESULT resize(ID3D11Device* device, UInt width, UInt height);
    
    void setTarget(ID3D11DeviceContext* context);
    HRESULT rebuildLightBuffers(ID3D11DeviceContext* context);
    ComPtr<ID3D11Buffer>& getLightPerspectiveBuffer() { return cameraBuffer; }
    ComPtr<ID3D11Buffer>& getLightBuffer() { return lightBuffer; }
    ComPtr<ID3D11SamplerState>& getShadowSampler() { return sampler; }
    DirectionalLightPack& getLightData() { return lightPack; }

    inline ID3D11ShaderResourceView* getShaderView() { return mapShaderView.Get(); }
    inline ID3D11Texture2D* getDepthTexture() { return depthMapTexture.Get(); }

    inline XMFLOAT3& getRenderPosition() { return renderPosition; }
    inline XMFLOAT4& getForward() { return lightPack.direction; }
    inline Camera& getCamera() { return camera; }
    void updateCameraView();

    private:
    ComPtr<ID3D11Texture2D> depthMapTexture;
    ComPtr<ID3D11DepthStencilView> mapBufferView;
    ComPtr<ID3D11ShaderResourceView> mapShaderView;
    D3D11_VIEWPORT viewport;
    float nearZ;
    float farZ;

    Camera camera;
    XMFLOAT3 renderPosition;

    ComPtr<ID3D11SamplerState> sampler;
    ComPtr<ID3D11Buffer> cameraBuffer;
    ComPtr<ID3D11Buffer> lightBuffer;
    DirectionalLightPack lightPack;
  };
}