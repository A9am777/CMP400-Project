#pragma once

#include "RenderTarget.h"
namespace Haboob
{
  // Reinhard tone mapping from HDR to LDR
  class ToneMapShader : Shader
  {
    public:
    ToneMapShader() : Shader(Shader::Type::Compute, L"Lighting/ToneMap") {}

    HRESULT initShader(ID3D11Device* device, const ShaderManager* manager);
    void bindShader(ID3D11DeviceContext* context, ID3D11UnorderedAccessView* uav);
    void bindShader(ID3D11DeviceContext* context) = delete;
    void unbindShader(ID3D11DeviceContext* context);

    inline void setGamma(float newGamma) { toneInfo.gamma = newGamma; }
    inline void setExposure(float newExposure) { toneInfo.exposure = newExposure; }

    private:
    struct TonePack
    {
      float gamma = .0f;
      float exposure = .0f;
      XMFLOAT2 padding;
    };
    TonePack toneInfo;
    ComPtr<ID3D11Buffer> toneBuffer;
  };

  // Renders additive lighting over a GBuffer to a lit colour buffer
  class LightPassShader : public Shader
  {
    public:
    LightPassShader() : Shader(Shader::Type::Compute, L"Lighting/DeferredLightPass") {}

    void bindShader(ID3D11DeviceContext* context, ID3D11Buffer* lightbuffer);
    void bindShader(ID3D11DeviceContext* context) = delete;
    void unbindShader(ID3D11DeviceContext* context);
  };

  // GBuffer deferred render target
  class GBuffer
  {
    public:
    GBuffer();

    void setTargets(ID3D11DeviceContext* context, ID3D11DepthStencilView* depthStencil = nullptr);
    void clear(ID3D11DeviceContext* context);

    HRESULT create(ID3D11Device* device, UInt width, UInt height);
    HRESULT resize(ID3D11Device* device, UInt width, UInt height);

    // Renders from the GBuffer into the lit buffer
    void lightPass(ID3D11DeviceContext* context, ID3D11Buffer* lightbuffer);
    // Tone maps the lit buffer
    void finalLitPass(ID3D11DeviceContext* context);
    // Renders to another target using the lit texture
    void renderFromLit(ID3D11DeviceContext* context);

    inline RenderTarget& getLitColourTarget() { return litColourTarget; }

    inline float& getGamma() { return gamma; }
    inline float& getExposure() { return exposure; }

    static ToneMapShader toneMapShader;
    static LightPassShader lightShader;
    private:
    RenderTarget diffuseTarget; // colour(r, g, b), unused
    RenderTarget normalDepthTarget; // normal(nx, ny, nz), depth
    RenderTarget litColourTarget; // colour(r, g, b), alpha
    float gamma;
    float exposure;
  };
}