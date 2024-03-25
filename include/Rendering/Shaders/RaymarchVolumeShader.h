#pragma once
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Textures/RenderTarget.h"
#include "Rendering/Geometry/MeshRenderer.h"
#include <Rendering/Textures/GBuffer.h>
#include <Rendering/Scene/Camera.h>

namespace Haboob
{
  struct MarchVolumeDispatchInfo
  {
    float outputHorizontalStep = 1.f; // View step per horizontal thread
    float outputVerticalStep = 1.f; // View step per vertical thread
    float initialZStep = 0.f; // Distance to jump in Z
    float marchZStep = .1f; // Distance to jump in Z
    
    UINT iterations = 10; // Number of volume steps to take
    float texelDensity; // The density of the volume texture in world space
    float pixelRadius = .001f; // The radius a pixel occupies in world space
    float pixelRadiusDelta = .245f; // The linear change of pixel radius with world depth

    XMMATRIX localVolumeTransform; // Transforms from world space to volume space
    XMFLOAT3 volumeSize; // The scale of the volume in world space
    float volumeSizeW = 1.f;
  };

  struct BasicOptics
  {
    XMFLOAT4 anisotropicForwardTerms; // Forward anisotropic parameters per major light component
    XMFLOAT4 anisotropicBackwardTerms; // Backward anisotropic parameters per major light component
    XMFLOAT4 phaseBlendWeightTerms; // Per component phase blend factor

    float scatterAngstromExponent;
    float absorptionAngstromExponent;
    float attenuationFactor; // Scales optical depth
    float powderCoefficient; // Beers-Powder scaling factor
    XMFLOAT4X4 spectralWavelengths; // Wavelengths to integrate over
    XMFLOAT4X4 spectralWeights; // Spectral integration weights

    XMFLOAT4 ambientFraction;
    UINT flagApplyBeer = 1; // Controls whether to apply Beer-Lambert attenuation
    UINT flagApplyHG = 1; // Controls whether to apply the HG phase function
    UINT flagApplySpectral = 1; // Controls whether to integrate over several wavelengths
    UINT flagUnused = 1;
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
    struct HaboobRadial
    {
      float roofGradient = -3.87f;
      float exponentialRate = 20.f;
      float exponentialScale = .22f;
      float rOffset = .0f;
      float noseHeight = .34f;
      float blendHeight = .22f;
      float blendRate = .25f;
      float padding = .0f;
    };

    struct HaboobDistribution
    {
      float falloffScale = .4f;
      float heightScale = 1.3f;
      float heightExponent = .2f;
      float angleRange = 4.2f;
      float anglePower = 3.f;
      XMFLOAT3 padding;
    };

    struct VolumeInfo
    {
      XMINT3 size = {128, 128, 128}; // Texture size
      UINT padding;

      // Proc gen params
      XMUINT4 seed = { 0x12345, 0xCAFEBABE, 0xDEADBEEF, 0 };

      float worldSize = 5.f;
      float octaves = 3.1f;
      float fractionalGap = 4.4f;
      float fractionalIncrement = 0.99f;

      float fbmOffset = .1f;
      float fbmScale = .4f;
      float wackyPower = .8f;
      float wackyScale = .6f;

      HaboobRadial radial;
      HaboobDistribution distribution;
    };

    VolumeGenerationShader();
    ~VolumeGenerationShader();

    HRESULT initShader(ID3D11Device* device, ShaderManager* manager);
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

    HRESULT initShader(ID3D11Device* device, ShaderManager* manager);
    void bindShader(ID3D11DeviceContext* context, ID3D11ShaderResourceView* densityTexResource);
    void unbindShader(ID3D11DeviceContext* context);

    // Mirror from the intermediate to the target buffer
    void mirror(ID3D11DeviceContext* context);

    // TODO: TEMP
    void optimiseRays(DisplayDevice& device, MeshRenderer<VertexType>& renderer, GBuffer& gbuffer, XMVECTOR& cameraPosition);

    HRESULT createIntermediate(ID3D11Device* device, UInt width, UInt height);
    HRESULT resizeIntermediate(ID3D11Device* device, UInt width, UInt height);

    void render(ID3D11DeviceContext* context) const;

    inline void setTarget(RenderTarget* target) { renderTarget = target; }
    inline void setCameraBuffer(ComPtr<ID3D11Buffer> buffer) { cameraBuffer = buffer; }
    inline void setLightBuffer(ComPtr<ID3D11Buffer> buffer) { lightBuffer = buffer; }
    inline void setBox(MeshInstance<VertexType>* boxInstance) { boundingBox = boxInstance; }
    inline void setShouldUpscale(bool upscale) { shouldUpscale = upscale; }

    inline MeshInstance<VertexType>* getBox() { return boundingBox; }
    inline MarchVolumeDispatchInfo& getMarchInfo() { return marchInfo; }
    inline BasicOptics& getOpticsInfo() { return opticsInfo; }

    void buildSpectralMatrices();

    private:
    typedef MeshInstance<VertexType> MeshInstance;

    // Optimisations
    MeshInstance* boundingBox;
    ComPtr<ID3D11BlendState> additiveBlend;
    ComPtr<ID3D11SamplerState> pixelSamplerState;
    Shader* frontRayVisibilityPixelShader;
    Shader* backRayVisibilityPixelShader;
    bool shouldUpscale;

    // Intermediates
    RenderTarget rayTarget; // Used to store ray information between stages
    Shader* mirrorComputeShader;

    // Main
    Shader* computeShader;
    RenderTarget* renderTarget;
    
    // Buffers
    MarchVolumeDispatchInfo marchInfo;
    BasicOptics opticsInfo;
    ComPtr<ID3D11SamplerState> marchSamplerState;
    ComPtr<ID3D11Buffer> marchBuffer;
    ComPtr<ID3D11Buffer> cameraBuffer;
    ComPtr<ID3D11Buffer> lightBuffer;

    // Coefficients of CIE functions in order {scale, exponentScale, wavelengthScale, wavelengthOffset}
    float redMinorCIECoefficients[4] = { 0.39800f, 35.35534f, 0.78895f, 0.56223f };
    float redMajorCIECoefficients[4] = { 1.13200f, 15.29706f, -1.07599f, 1.79960f };
    float greenCIECoefficients[4] = { 1.01100f, 1.f, 12.2602f, -8.52237f };
    float blueCIECoefficients[4] = { 2.06000f, 5.65685f, 4.43459f, -1.47339f };
  };
}