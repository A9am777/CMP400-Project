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

 struct BasicOptics
 {
   float attenuationFactor = 1.f; // Scales optical depth
   XMFLOAT3 colourHGScatter = {1.f, 1.f, 1.f}; // Per component light scattering for the HG phase function
   UINT flagApplyBeer = ~0; // Controls whether to apply Beer-Lambert attenuation
   UINT flagApplyHG = ~0; // Controls whether to apply the HG phase function
   XMUINT2 padding;
 };

 struct ComprehensiveBufferInfo
 {
   MarchVolumeDispatchInfo marchVolumeInfo;
   BasicOptics opticalInfo;
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
    inline BasicOptics& getOpticsInfo() { return opticsInfo; }

    private:
    Shader* computeShader;
    RenderTarget* renderTarget;
    MarchVolumeDispatchInfo marchInfo;
    BasicOptics opticsInfo;
    ComPtr<ID3D11Buffer> marchBuffer;
    ComPtr<ID3D11Buffer> cameraBuffer;
    ComPtr<ID3D11Buffer> lightBuffer;
  };
}