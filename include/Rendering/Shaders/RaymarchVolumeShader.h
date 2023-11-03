#pragma once
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Textures/RenderTarget.h"

namespace Haboob
{
 struct MarchVolumeDispatchInfo
  {
    float outputHorizontalStep = 1.f; // View step per horizontal thread
    float outputVerticalStep = 1.f; // View step per vertical thread
    float initialZStep = 0.f; // Distance to jump in Z
    float marchZStep = .1f; // Distance to jump in Z
    UINT iterations = 10; // Number of volume steps to take
    XMFLOAT3 padding;
  };

  class RaymarchVolumeShader
  {
    public:
    RaymarchVolumeShader();
    ~RaymarchVolumeShader();

    HRESULT initShader(ID3D11Device* device, const ShaderManager* manager);
    void bindShader(ID3D11DeviceContext* context);
    void unbindShader(ID3D11DeviceContext* context);

    void render(ID3D11DeviceContext* context) const;

    inline void setTarget(RenderTarget* target) { renderTarget = target; }
    inline void setCameraBuffer(ComPtr<ID3D11Buffer> buffer) { cameraBuffer = buffer; }
    inline void setLightBuffer(ComPtr<ID3D11Buffer> buffer) { lightBuffer = buffer; }

    inline MarchVolumeDispatchInfo& getMarchInfo() { return marchInfo; }

    private:
    Shader* computeShader;
    RenderTarget* renderTarget;
    MarchVolumeDispatchInfo marchInfo;
    ComPtr<ID3D11Buffer> marchBuffer;
    ComPtr<ID3D11Buffer> cameraBuffer;
    ComPtr<ID3D11Buffer> lightBuffer;
  };
}