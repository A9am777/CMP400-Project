#pragma once
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Textures/RenderTarget.h"
#include "Rendering/Geometry/MeshRenderer.h"
#include <Rendering/Textures/GBuffer.h>
#include <Rendering/Scene/Camera.h>
#include <Rendering/Lighting/LightSource.h>

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
    XMFLOAT4X4 spectralToRGB; // CIEXYZ to linear RGB

    XMFLOAT4 ambientFraction;
    float referenceWavelength = 0.743f; // The wavelength which the Angstrom exponent is in relation to
    UINT flagApplyBeer = 1; // Controls whether to apply Beer-Lambert attenuation
    UINT flagApplyHG = 1; // Controls whether to apply the HG phase function
    UINT flagApplySpectral = 1; // Controls whether to integrate over several wavelengths
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
      float roofGradient = -3.58f;
      float exponentialRate = 8.36f;
      float exponentialScale = .22f;
      float rOffset = .36f;
      float noseHeight = .19f;
      float blendHeight = .87f;
      float blendRate = 1.45f;
      float padding = .0f;
    };

    struct HaboobDistribution
    {
      float falloffScale = .22f;
      float heightScale = 5.52f;
      float heightExponent = .74f;
      float angleRange = 4.33f;
      float anglePower = 3.26f;
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
      float wackyPower = 5.1f;
      float wackyScale = .31f;

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

    void updateSharedBuffers(ID3D11DeviceContext* context);

    // Mirror from the intermediate to the target buffer
    void mirror(ID3D11DeviceContext* context);

    // TODO: TEMP
    void optimiseRays(DisplayDevice& device, MeshRenderer<VertexType>& renderer, GBuffer& gbuffer, XMVECTOR& cameraPosition);

    // Renders the Beer Shadow Map
    void bindSoftShadowMap(ID3D11DeviceContext* context, ID3D11ShaderResourceView* densityTexResource);
    void generateSoftShadowMap(ID3D11DeviceContext* context) const;
    void unbindSoftShadowMap(ID3D11DeviceContext* context);

    HRESULT createTextures(ID3D11Device* device, UInt width, UInt height);
    HRESULT resizeTextures(ID3D11Device* device, UInt width, UInt height);

    void render(ID3D11DeviceContext* context) const;

    inline void setTarget(RenderTarget* target) { renderTarget = target; }
    inline void setCameraBuffer(ComPtr<ID3D11Buffer> buffer) { cameraBuffer = buffer; }
    inline void setLightSource(Light* lightSource) { mainLight = lightSource; }
    inline void setBox(MeshInstance<VertexType>* boxInstance) { boundingBox = boxInstance; }
    inline void setShouldUpscale(bool upscale) { shouldUpscale = upscale; }

    inline MeshInstance<VertexType>* getBox() { return boundingBox; }
    inline MarchVolumeDispatchInfo& getMarchInfo() { return marchInfo; }
    inline BasicOptics& getOpticsInfo() { return opticsInfo; }
    inline ID3D11ShaderResourceView* getBSMResource() { return bsmTarget.getShaderView(); }
    inline ID3D11Buffer* getMarchBuffer() { return marchBuffer.Get(); }

    void buildSpectralMatrices();

    private:
    typedef MeshInstance<VertexType> MeshInstance;

    // Optimisations
    MeshInstance* boundingBox;
    ComPtr<ID3D11BlendState> frontRayBlend;
    ComPtr<ID3D11BlendState> backRayBlend;
    ComPtr<ID3D11SamplerState> pixelSamplerState;
    Shader* frontRayVisibilityPixelShader;
    Shader* backRayVisibilityPixelShader;
    bool shouldUpscale;

    // Intermediates
    RenderTarget rayTarget; // Used to store ray information between stages
    RenderTarget bsmTarget; // Used to store ray information between stages
    Shader* mirrorComputeShader;
    Shader* bsmComputeShader;

    // Main
    Shader* computeShader;
    RenderTarget* renderTarget;
    
    // Buffers
    MarchVolumeDispatchInfo marchInfo;
    BasicOptics opticsInfo;
    ComPtr<ID3D11SamplerState> marchSamplerState;
    ComPtr<ID3D11Buffer> marchBuffer;
    ComPtr<ID3D11Buffer> cameraBuffer;
    Light* mainLight;

    // Coefficients of CIE functions in order {scale, exponentScale, wavelengthScale, wavelengthOffset}
    float redMinorCIECoefficients[4] = { 0.39800f, 35.35534f, 0.78895f, 0.56223f };
    float redMajorCIECoefficients[4] = { 1.13200f, 15.29706f, -1.07599f, 1.79960f };
    float greenCIECoefficients[4] = { 1.01100f, 1.f, 12.2602f, -8.52237f };
    float blueCIECoefficients[4] = { 2.06000f, 5.65685f, 4.43459f, -1.47339f };
  };
}