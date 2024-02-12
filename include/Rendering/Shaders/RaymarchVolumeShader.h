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
    UINT flagManualMarch = 0; // Whether to attempt to automatically adjust raymarching or use the manual values
    XMFLOAT2 padding;
  };

  struct BasicOptics
  {
    float attenuationFactor = 1.f; // Scales optical depth
    XMFLOAT3 colourHGScatter = { 1.f, 1.f, 1.f }; // Per component light scattering for the HG phase function
    float densityCoefficient = 1.f; // Scaling factor of density
    UINT flagApplyBeer = ~0; // Controls whether to apply Beer-Lambert attenuation
    UINT flagApplyHG = ~0; // Controls whether to apply the HG phase function
    UINT padding;
  };

  struct ComprehensiveBufferInfo
  {
    MarchVolumeDispatchInfo marchVolumeInfo;
    BasicOptics opticalInfo;
  };

  // Generates a Haboob to a 3d texture
  class VolumeGenerationShader
  {
    public:
    struct VolumeInfo
    {
      XMINT3 size = {128, 128, 128}; // Texture size
      UINT padding;

      // Proc gen params
      XMUINT4 seed = { 0x12345, 0xCAFEBABE, 0xDEADBEEF, 0 };

      float worldSize = 5.f;
      float octaves = 3.1f;
      float fractionalGap = 2.4f;
      float fractionalIncrement = 1.01f;

      float fbmOffset = .1f;
      float fbmScale = .4f;
      float wackyPower = .8f;
      float wackyScale = .0f;
    };

    VolumeGenerationShader();
    ~VolumeGenerationShader();

    HRESULT initShader(ID3D11Device* device, const ShaderManager* manager);
    HRESULT rebuild(ID3D11Device* device); // Creates texture from params
    void render(ID3D11DeviceContext* context); // Renders to the texture according to specifications

    inline VolumeInfo& getVolumeInfo() { return volumeInfo; }
    inline ID3D11RenderTargetView* getRenderTarget() { return textureTarget.Get(); }
    inline ID3D11ShaderResourceView* getShaderView() { return textureShaderView.Get(); }
    inline ID3D11UnorderedAccessView* getComputeView() { return computeAccessView.Get(); }

    private:
    void bindShader(ID3D11DeviceContext* context);
    void unbindShader(ID3D11DeviceContext* context);

    Shader* generateVolumeShader;
    ComPtr<ID3D11Texture3D> texture;
    ComPtr<ID3D11RenderTargetView> textureTarget;
    ComPtr<ID3D11ShaderResourceView> textureShaderView;
    ComPtr<ID3D11UnorderedAccessView> computeAccessView;
    ComPtr<ID3D11Buffer> volumeInfoBuffer;
    VolumeInfo volumeInfo;
  };

  // Renders a raymarched 3d volume
  class RaymarchVolumeShader
  {
    public:
    RaymarchVolumeShader();
    ~RaymarchVolumeShader();

    HRESULT initShader(ID3D11Device* device, const ShaderManager* manager);
    void bindShader(ID3D11DeviceContext* context, ID3D11ShaderResourceView* densityTexResource);
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
    ComPtr<ID3D11SamplerState> marchSamplerState;
    ComPtr<ID3D11Buffer> marchBuffer;
    ComPtr<ID3D11Buffer> cameraBuffer;
    ComPtr<ID3D11Buffer> lightBuffer;
  };
}