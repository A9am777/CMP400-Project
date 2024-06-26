#include "Rendering/Shaders/ShaderManager.h"
#include "Rendering/Geometry/MeshRendererImpl.h"
#include "Rendering/Shaders/RaymarchVolumeShader.h"

#include <cmath>

#undef min
#undef max

namespace Haboob
{
  RaymarchVolumeShader::RaymarchVolumeShader()
  {
    computeShader = new Shader(Shader::Type::Compute, L"Raymarch/MarchVolume");
    bsmComputeShader = new Shader(Shader::Type::Compute, L"Raymarch/BeerShadowMarchVolume");
    mirrorComputeShader = new Shader(Shader::Type::Compute, L"Raymarch/MirrorMarchTexture");
    frontRayVisibilityPixelShader = new Shader(Shader::Type::Pixel, L"Raymarch/FrontFacingRayVisibility");
    backRayVisibilityPixelShader = new Shader(Shader::Type::Pixel, L"Raymarch/BackFacingRayVisibility");

    shouldUpscale = true;
    renderTarget = nullptr;
    boundingBox = nullptr;
    buildSpectralMatrices();
  }

  RaymarchVolumeShader::~RaymarchVolumeShader()
  {
    delete computeShader; computeShader = nullptr;
    delete bsmComputeShader; bsmComputeShader = nullptr;
    delete mirrorComputeShader; mirrorComputeShader = nullptr;
    delete frontRayVisibilityPixelShader; frontRayVisibilityPixelShader = nullptr;
    delete backRayVisibilityPixelShader; backRayVisibilityPixelShader = nullptr;
  }

  HRESULT RaymarchVolumeShader::initShader(ID3D11Device* device, ShaderManager* manager)
  {
    HRESULT result = S_OK;

    result = computeShader->initShader(device, manager);
    Firebreak(result);
    bsmComputeShader->initShader(device, manager);
    Firebreak(result);
    result = mirrorComputeShader->initShader(device, manager);
    Firebreak(result);
    result = frontRayVisibilityPixelShader->initShader(device, manager);
    Firebreak(result);
    result = backRayVisibilityPixelShader->initShader(device, manager);
    Firebreak(result);

    // Create the raymarch info buffer
    {
      D3D11_BUFFER_DESC marchBufferDesc;
      marchBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
      marchBufferDesc.ByteWidth = sizeof(ComprehensiveBufferInfo);
      marchBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
      marchBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
      marchBufferDesc.MiscFlags = 0;
      marchBufferDesc.StructureByteStride = 0;
      result = device->CreateBuffer(&marchBufferDesc, NULL, marchBuffer.ReleaseAndGetAddressOf());
      Firebreak(result);
    }

    // Create the density volume sampler
    {
      D3D11_SAMPLER_DESC volumeSamplerDesc;
      ZeroMemory(&volumeSamplerDesc, sizeof(D3D11_SAMPLER_DESC));

      // Important for automatically rejecting samples outside of bounds
      volumeSamplerDesc.AddressU = volumeSamplerDesc.AddressV = volumeSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
      volumeSamplerDesc.BorderColor[0] = volumeSamplerDesc.BorderColor[1] = volumeSamplerDesc.BorderColor[2] = volumeSamplerDesc.BorderColor[3] = 0.f;

      volumeSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
      volumeSamplerDesc.MipLODBias = 0.0f;
      volumeSamplerDesc.MaxAnisotropy = 1;
      volumeSamplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
      volumeSamplerDesc.MinLOD = 0;
      volumeSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
      
      result = device->CreateSamplerState(&volumeSamplerDesc, marchSamplerState.ReleaseAndGetAddressOf());
      Firebreak(result);
    }

    // Create a pixel (point) sampler
    {
      D3D11_SAMPLER_DESC pixelSamplerDesc;
      ZeroMemory(&pixelSamplerDesc, sizeof(D3D11_SAMPLER_DESC));

      pixelSamplerDesc.AddressU = pixelSamplerDesc.AddressV = pixelSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;

      pixelSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
      pixelSamplerDesc.MipLODBias = 0.0f;
      pixelSamplerDesc.MaxAnisotropy = 1;
      pixelSamplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
      pixelSamplerDesc.MinLOD = 0;
      pixelSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

      result = device->CreateSamplerState(&pixelSamplerDesc, pixelSamplerState.ReleaseAndGetAddressOf());
      Firebreak(result);
    }

    // Create the masking blend state (for ray cull optimisations)
    {
      D3D11_BLEND_DESC blendDesc;
      blendDesc.AlphaToCoverageEnable = false;
      blendDesc.IndependentBlendEnable = false;

      auto& renderTargetBlend = blendDesc.RenderTarget[0];
      {
        renderTargetBlend = D3D11_RENDER_TARGET_BLEND_DESC();
        renderTargetBlend.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_ALPHA;
        renderTargetBlend.BlendEnable = true;
        renderTargetBlend.BlendOp = D3D11_BLEND_OP_ADD;
        renderTargetBlend.BlendOpAlpha = D3D11_BLEND_OP_ADD;
        renderTargetBlend.SrcBlend = D3D11_BLEND_ONE;
        renderTargetBlend.DestBlend = D3D11_BLEND_ZERO;
        renderTargetBlend.SrcBlendAlpha = D3D11_BLEND_ONE;
        renderTargetBlend.DestBlendAlpha = D3D11_BLEND_ZERO;
      }
      result = device->CreateBlendState(&blendDesc, frontRayBlend.ReleaseAndGetAddressOf());
      Firebreak(result);

      renderTargetBlend.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL & ~D3D11_COLOR_WRITE_ENABLE_RED;

      result = device->CreateBlendState(&blendDesc, backRayBlend.ReleaseAndGetAddressOf());
      Firebreak(result);
    }

    return result;
  }

  void RaymarchVolumeShader::updateSharedBuffers(ID3D11DeviceContext* context)
  {
    marchInfo.outputHorizontalStep = 1.f / float(rayTarget.getWidth());
    marchInfo.outputVerticalStep = 1.f / float(rayTarget.getHeight());
    boundingBox->buildTransform();
    marchInfo.localVolumeTransform = XMMatrixInverse(nullptr, boundingBox->getTransform());
    marchInfo.volumeSize = boundingBox->getScale();

    // Update the march buffer
    {
      D3D11_MAPPED_SUBRESOURCE mapped;
      HRESULT result = context->Map(marchBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
      ComprehensiveBufferInfo* mappedBuffer = (ComprehensiveBufferInfo*)mapped.pData;
      std::memcpy(&mappedBuffer->marchVolumeInfo, &marchInfo, sizeof(MarchVolumeDispatchInfo));
      std::memcpy(&mappedBuffer->opticalInfo, &opticsInfo, sizeof(BasicOptics));
      context->Unmap(marchBuffer.Get(), 0);
    }
  }

  void RaymarchVolumeShader::bindShader(ID3D11DeviceContext* context, ID3D11ShaderResourceView* densityTexResource)
  {
    computeShader->bindShader(context);
    ID3D11UnorderedAccessView* accessView = rayTarget.getComputeView();
    context->CSSetUnorderedAccessViews(0, 1, &accessView, 0);
    context->CSSetConstantBuffers(0, 1, cameraBuffer.GetAddressOf());

    context->CSSetConstantBuffers(1, 1, marchBuffer.GetAddressOf());
    context->CSSetConstantBuffers(2, 1, mainLight->getLightBuffer().GetAddressOf());
    context->CSSetConstantBuffers(3, 1, mainLight->getLightPerspectiveBuffer().GetAddressOf());

    context->CSSetSamplers(0, 1, marchSamplerState.GetAddressOf());
    context->CSSetShaderResources(0, 1, &densityTexResource);

    context->CSSetSamplers(1, 1, mainLight->getShadowSampler().GetAddressOf());
    auto shadowTextureView = mainLight->getShaderView();
    context->CSSetShaderResources(1, 1, &shadowTextureView);

    auto bsmTextureView = bsmTarget.getShaderView();
    context->CSSetShaderResources(2, 1, &bsmTextureView);
  }

  void RaymarchVolumeShader::unbindShader(ID3D11DeviceContext* context)
  {
    computeShader->unbindShader(context);

    void* nullpo[4] = { nullptr, nullptr, nullptr, nullptr };
    context->CSSetUnorderedAccessViews(0, 1, (ID3D11UnorderedAccessView**)&nullpo, 0);
    context->CSSetConstantBuffers(0, 4, (ID3D11Buffer**)&nullpo);
    context->CSSetSamplers(0, 2, (ID3D11SamplerState**)&nullpo);
    context->CSSetShaderResources(0, 3, (ID3D11ShaderResourceView**)&nullpo);
  }

  void RaymarchVolumeShader::mirror(ID3D11DeviceContext* context)
  {
    auto& copyShader = RenderTarget::copyShader;
    
    mirrorComputeShader->bindShader(context);

    ID3D11UnorderedAccessView* textureComputeView = renderTarget->getComputeView();
    context->CSSetUnorderedAccessViews(0, 1, &textureComputeView, 0);

    ID3D11SamplerState* sampler = copyShader.getSampler().Get();
    context->CSSetSamplers(0, 1, &sampler);

    ID3D11ShaderResourceView* rayResourceTexture = rayTarget.getShaderView();
    context->CSSetShaderResources(0, 1, &rayResourceTexture);

    context->CSSetConstantBuffers(1, 1, marchBuffer.GetAddressOf());

    mirrorComputeShader->dispatch(context, 1 + renderTarget->getWidth() / 8, 1 + renderTarget->getHeight() / 8);

    mirrorComputeShader->unbindShader(context);

    void* nullpo = nullptr;
    context->CSSetUnorderedAccessViews(0, 1, (ID3D11UnorderedAccessView**)&nullpo, 0);
    context->CSSetConstantBuffers(1, 1, (ID3D11Buffer**)&nullpo);
    context->CSSetSamplers(0, 1, (ID3D11SamplerState**)&nullpo);
    context->CSSetShaderResources(0, 1, (ID3D11ShaderResourceView**)&nullpo);
  }

  void RaymarchVolumeShader::optimiseRays(DisplayDevice& device, MeshRenderer<VertexType>& renderer, GBuffer& gbuffer, XMVECTOR& cameraPosition)
  {
    auto context = device.getContext().Get();
    renderer.bind(context);

    bool cameraWithin = false;

    // Determine if the camera is within a regular bounding sphere
    {
      XMVECTOR cameraLocals = XMVector3Transform(cameraPosition, marchInfo.localVolumeTransform);
      float cameraRadius = XMVectorGetX(XMVector3Length(cameraLocals));
      cameraWithin = cameraRadius <= sqrt(3) * .5f + .03f;
    }

    // -ve values signal "not visible" or "fragment component not updated"
    float rayClearColour[4] = { cameraWithin ? .0f : -1.f, -1.f, .0f, 1.f };
    rayTarget.clear(context, rayClearColour);
    rayTarget.setTarget(context, device.getDepthBuffer());
    context->OMSetBlendState(frontRayBlend.Get(), nullptr, ~0);

    boundingBox->setVisible(true);
    device.setDepthEnabled(true, false);

    if (!cameraWithin)
    {
      // Render front face
      frontRayVisibilityPixelShader->bindShader(context);
      renderer.draw(context, boundingBox);
    }
    
    // Render back face
    {
      // Prepare to roll back to the previous raster state
      auto previousRasterState = device.getRasterState();
      
      context->OMSetBlendState(backRayBlend.Get(), nullptr, ~0);

      // Cull front face and render the backface as a 'mask'
      device.setCullBackface(false); 
      device.setCull(true);
      device.setDepthEnabled(false, false);

      // Need to read the depth so either the bounds max or depth is rendered!
      ID3D11ShaderResourceView* textureView = gbuffer.getNormalDepthTarget().getShaderView();
      context->PSSetShaderResources(0, 1, &textureView);
      context->PSSetSamplers(0, 1, pixelSamplerState.GetAddressOf());
      backRayVisibilityPixelShader->bindShader(context);

      renderer.draw(context, boundingBox);

      void* nullpo = nullptr;
      context->PSSetSamplers(0, 1, (ID3D11SamplerState**)&nullpo);
      context->PSSetShaderResources(0, 1, (ID3D11ShaderResourceView**)&nullpo);

      device.setRasterState(previousRasterState);
      device.setDepthEnabled(true);
    }

    renderer.unbind(context);

    boundingBox->setVisible(false);
    context->OMSetBlendState(nullptr, nullptr, ~0);
    device.setBackBufferTarget();
  }

  HRESULT RaymarchVolumeShader::createTextures(ID3D11Device* device, UInt width, UInt height)
  {
    // Currently no need to use custom texture descriptions
    HRESULT result = S_OK;

    result = rayTarget.create(device, width, height);
    Firebreak(result);

    result = bsmTarget.create(device, width, height);
    Firebreak(result);

    return result;
  }

  HRESULT RaymarchVolumeShader::resizeTextures(ID3D11Device* device, UInt width, UInt height)
  {
    // Currently no need to use custom texture descriptions
    HRESULT result = S_OK;

    result = rayTarget.resize(device, width, height);
    Firebreak(result);

    result = bsmTarget.resize(device, width, height);
    Firebreak(result);

    return result;
  }

  void RaymarchVolumeShader::bindSoftShadowMap(ID3D11DeviceContext* context, ID3D11ShaderResourceView* densityTexResource)
  {
    bsmComputeShader->bindShader(context);
    ID3D11UnorderedAccessView* accessViews[2] = { rayTarget.getComputeView(), bsmTarget.getComputeView() };
    context->CSSetUnorderedAccessViews(0, 2, accessViews, 0);
    context->CSSetConstantBuffers(0, 1, mainLight->getLightPerspectiveBuffer().GetAddressOf());

    context->CSSetConstantBuffers(1, 1, marchBuffer.GetAddressOf());
    context->CSSetConstantBuffers(2, 1, mainLight->getLightBuffer().GetAddressOf());

    context->CSSetSamplers(0, 1, marchSamplerState.GetAddressOf());
    context->CSSetShaderResources(0, 1, &densityTexResource);
  }

  void RaymarchVolumeShader::unbindSoftShadowMap(ID3D11DeviceContext* context)
  {
    computeShader->unbindShader(context);

    void* nullpo[3] = { nullptr, nullptr, nullptr };
    context->CSSetUnorderedAccessViews(0, 2, (ID3D11UnorderedAccessView**)nullpo, 0);
    context->CSSetConstantBuffers(0, 3, (ID3D11Buffer**)&nullpo);
    context->CSSetSamplers(0, 1, (ID3D11SamplerState**)&nullpo);
    context->CSSetShaderResources(0, 1, (ID3D11ShaderResourceView**)&nullpo);
  }

  void RaymarchVolumeShader::generateSoftShadowMap(ID3D11DeviceContext* context) const
  {
    static constexpr UInt groupSize = 16;

    // Render with a quarter of rays if upscaling
    XMUINT2 rayCount = shouldUpscale ? XMUINT2(bsmTarget.getWidth() >> 1, bsmTarget.getHeight() >> 1) : XMUINT2(bsmTarget.getWidth(), bsmTarget.getHeight());
    // Divide rays into groups plus an extra padding group
    bsmComputeShader->dispatch(context, 1 + rayCount.x / groupSize, 1 + rayCount.y / groupSize);
  }

  void RaymarchVolumeShader::render(ID3D11DeviceContext* context) const
  {
    static constexpr UInt groupSize = 16;

    // Render with a quarter of rays if upscaling
    XMUINT2 rayCount = shouldUpscale ? XMUINT2(rayTarget.getWidth() >> 1, rayTarget.getHeight() >> 1) : XMUINT2(rayTarget.getWidth(), rayTarget.getHeight());
    // Divide rays into groups plus an extra padding group
    computeShader->dispatch(context, 1 + rayCount.x / groupSize, 1 + rayCount.y / groupSize);
  }

  void RaymarchVolumeShader::buildSpectralMatrices()
  {
    // Hermit-Gauss quadrature of order n=4
    float hermitGaussAbscissas[4] = { 0.524647623275f, -0.524647623275f, 1.650680123886f, -1.650680123886f };
    float hermitGaussWeights[4] = { 0.804914090006f, 0.804914090006f, 0.0813128354473f, 0.0813128354473f };

    // Substitution is in order
    auto logAbscissasAdjust = [](float abscissas, float distributionCoefficients[4]) -> float {
      return (exp(abscissas / distributionCoefficients[1]) - distributionCoefficients[3]) / distributionCoefficients[2];
    };
    auto linearAbscissasAdjust = [](float abscissas, float distributionCoefficients[4]) -> float {
      return (abscissas - distributionCoefficients[3]) / distributionCoefficients[2];
    };
    auto logWeight = [](float abscissas, float distributionCoefficients[4]) -> float {
      return abs(exp(abscissas / distributionCoefficients[1]) / (distributionCoefficients[1] * distributionCoefficients[2]));
    };
    auto linearWeight = [](float abscissas, float distributionCoefficients[4]) -> float {
      return 1.f / distributionCoefficients[2];
    };

    auto& optics = getOpticsInfo();
    for (size_t wavelet = 0; wavelet < 4; ++wavelet)
    {
      float* distribution = nullptr;
      switch (wavelet)
      {
        case 0: distribution = redMajorCIECoefficients;
        break;
        case 1: distribution = greenCIECoefficients;
        break;
        case 2: distribution = blueCIECoefficients;
        break;
        case 3: distribution = redMinorCIECoefficients;
        break;
      }

      for (size_t term = 0; term < 4; ++term)
      {
        if (wavelet == 1) // Green = linear
        {
          optics.spectralWeights.m[wavelet][term] = hermitGaussWeights[term] * linearWeight(hermitGaussAbscissas[term], distribution) * distribution[0];
          optics.spectralWavelengths.m[wavelet][term] = linearAbscissasAdjust(hermitGaussAbscissas[term], distribution);
        }
        else // Everything else = logarithmic
        {
          optics.spectralWeights.m[wavelet][term] = hermitGaussWeights[term] * logWeight(hermitGaussAbscissas[term], distribution) * distribution[0];
          optics.spectralWavelengths.m[wavelet][term] = logAbscissasAdjust(hermitGaussAbscissas[term], distribution);
        }
      }
    }

    // This matrix transforms from the CIEXYZ components to display independent linear RGB (note: column1 = column4)
    XMMATRIX spectralToRGB = XMMatrixSet(
      3.2406, -1.5372, -.4986, 3.2406,
      -.9689, 1.8758, .0415, -.9689,
      .0557, -.2040, 1.0570, .0557,
      .0, .0, .0, .0);
    spectralToRGB = XMMatrixTranspose(spectralToRGB);
    XMStoreFloat4x4(&optics.spectralToRGB, spectralToRGB);
  }

  VolumeGenerationShader::VolumeGenerationShader()
  {
    generateVolumeShader = new Shader(Shader::Type::Compute, L"TestShaders/TestFormHaboob", true);
  }

  VolumeGenerationShader::~VolumeGenerationShader()
  {
    delete generateVolumeShader; generateVolumeShader = nullptr;
  }
  HRESULT VolumeGenerationShader::initShader(ID3D11Device* device, ShaderManager* manager)
  {
    HRESULT result = S_OK;

    result = generateVolumeShader->initShader(device, manager);
    Firebreak(result);

    // Create the volume info buffer
    {
      D3D11_BUFFER_DESC volumeBufferDesc;
      volumeBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
      volumeBufferDesc.ByteWidth = sizeof(VolumeInfo);
      volumeBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
      volumeBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
      volumeBufferDesc.MiscFlags = 0;
      volumeBufferDesc.StructureByteStride = 0;
      result = device->CreateBuffer(&volumeBufferDesc, NULL, volumeInfoBuffer.ReleaseAndGetAddressOf());
      Firebreak(result);
    }

    return result;
  }

  HRESULT VolumeGenerationShader::rebuild(ID3D11Device* device)
  {
    HRESULT result = S_OK;

    // Create the texture
    D3D11_TEXTURE3D_DESC volumeTextureDesc;
    {
      ZeroMemory(&volumeTextureDesc, sizeof(D3D11_TEXTURE3D_DESC));

      volumeTextureDesc.Width = volumeInfo.size.x;
      volumeTextureDesc.Height = volumeInfo.size.y;
      volumeTextureDesc.Depth = volumeInfo.size.z;
      volumeTextureDesc.MipLevels = std::min(volumeTextureDesc.Width, volumeTextureDesc.Height);
      volumeTextureDesc.MipLevels = std::min(volumeTextureDesc.MipLevels, volumeTextureDesc.Depth);
      volumeTextureDesc.MipLevels = std::log2(volumeTextureDesc.MipLevels);
      volumeTextureDesc.Format = DXGI_FORMAT_R11G11B10_FLOAT;
      volumeTextureDesc.Usage = D3D11_USAGE_DEFAULT;
      volumeTextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
      volumeTextureDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

      result = device->CreateTexture3D(&volumeTextureDesc, nullptr, texture.ReleaseAndGetAddressOf());
      Firebreak(result);
    }

    // Create the target
    {
      D3D11_RENDER_TARGET_VIEW_DESC renderTargetDesc;
      ZeroMemory(&renderTargetDesc, sizeof(D3D11_RENDER_TARGET_VIEW_DESC));

      renderTargetDesc.Format = volumeTextureDesc.Format;
      renderTargetDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
      renderTargetDesc.Texture3D.FirstWSlice = 0;
      renderTargetDesc.Texture3D.MipSlice = 0;
      renderTargetDesc.Texture3D.WSize = volumeTextureDesc.Depth;

      result = device->CreateRenderTargetView(texture.Get(), &renderTargetDesc, textureTarget.ReleaseAndGetAddressOf());
      Firebreak(result);
    }

    // Create the shader view
    {
      D3D11_SHADER_RESOURCE_VIEW_DESC shaderViewDesc;
      ZeroMemory(&shaderViewDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));

      shaderViewDesc.Format = volumeTextureDesc.Format;
      shaderViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
      shaderViewDesc.Texture3D.MipLevels = volumeTextureDesc.MipLevels;

      result = device->CreateShaderResourceView(texture.Get(), &shaderViewDesc, textureShaderView.ReleaseAndGetAddressOf());
      Firebreak(result);
    }

    // Create the compute unordered access view
    {
      D3D11_UNORDERED_ACCESS_VIEW_DESC computeAccessDesc;
      ZeroMemory(&computeAccessDesc, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC));
      computeAccessDesc.Format = volumeTextureDesc.Format;
      computeAccessDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
      computeAccessDesc.Texture3D.FirstWSlice = 0;
      computeAccessDesc.Texture3D.MipSlice = 0;
      computeAccessDesc.Texture3D.WSize = volumeTextureDesc.Depth;

      result = device->CreateUnorderedAccessView(texture.Get(), &computeAccessDesc, computeAccessView.ReleaseAndGetAddressOf());
      Firebreak(result);
    }

    return result;
  }

  void VolumeGenerationShader::render(ID3D11DeviceContext* context)
  {
    generateVolumeShader->bindShader(context);
    bindShader(context);
    static constexpr UInt groupSize = 8;
    Shader::dispatch(context, volumeInfo.size.x / groupSize, volumeInfo.size.y / groupSize, volumeInfo.size.z / groupSize);
    unbindShader(context);
    generateVolumeShader->unbindShader(context);

    context->GenerateMips(textureShaderView.Get());
  }

  void VolumeGenerationShader::bindShader(ID3D11DeviceContext* context)
  {
    // Update volume buffer
    {
      D3D11_MAPPED_SUBRESOURCE mapped;
      HRESULT result = context->Map(volumeInfoBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
      std::memcpy(mapped.pData, &volumeInfo, sizeof(VolumeInfo));
      context->Unmap(volumeInfoBuffer.Get(), 0);
    }

    context->CSSetConstantBuffers(0, 1, volumeInfoBuffer.GetAddressOf());
    context->CSSetUnorderedAccessViews(0, 1, computeAccessView.GetAddressOf(), 0);
  }

  void VolumeGenerationShader::unbindShader(ID3D11DeviceContext* context)
  {
    void* nullpo = nullptr;

    context->CSSetConstantBuffers(0, 1, (ID3D11Buffer**)&nullpo);
    context->CSSetUnorderedAccessViews(0, 1, (ID3D11UnorderedAccessView**)&nullpo, 0);
  }

}