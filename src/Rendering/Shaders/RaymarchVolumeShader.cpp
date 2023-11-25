#include "Rendering/Shaders/RaymarchVolumeShader.h"

namespace Haboob
{
  RaymarchVolumeShader::RaymarchVolumeShader()
  {
    computeShader = new Shader(Shader::Type::Compute, L"Raymarch/MarchVolume");
    renderTarget = nullptr;
  }

  RaymarchVolumeShader::~RaymarchVolumeShader()
  {
    delete computeShader; computeShader = nullptr;
  }

  HRESULT RaymarchVolumeShader::initShader(ID3D11Device* device, const ShaderManager* manager)
  {
    HRESULT result = S_OK;

    result = computeShader->initShader(device, manager);
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

      volumeSamplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
      volumeSamplerDesc.MipLODBias = 0.0f;
      volumeSamplerDesc.MaxAnisotropy = 1;
      volumeSamplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
      volumeSamplerDesc.MinLOD = 0;
      volumeSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
      
      result = device->CreateSamplerState(&volumeSamplerDesc, marchSamplerState.ReleaseAndGetAddressOf());
      Firebreak(result);
    }

    return result;
  }

  void RaymarchVolumeShader::bindShader(ID3D11DeviceContext* context, ID3D11ShaderResourceView* densityTexResource)
  {
    computeShader->bindShader(context);
    ID3D11UnorderedAccessView* accessView = renderTarget->getComputeView();
    context->CSSetUnorderedAccessViews(0, 1, &accessView, 0);
    context->CSSetConstantBuffers(0, 1, cameraBuffer.GetAddressOf());

    // Update and mirror march data
    {
      marchInfo.outputHorizontalStep = 1.f / float(renderTarget->getWidth());
      marchInfo.outputVerticalStep = 1.f / float(renderTarget->getHeight());

      D3D11_MAPPED_SUBRESOURCE mapped;
      HRESULT result = context->Map(marchBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
      ComprehensiveBufferInfo* mappedBuffer = (ComprehensiveBufferInfo*)mapped.pData;
      std::memcpy(&mappedBuffer->marchVolumeInfo, &marchInfo, sizeof(MarchVolumeDispatchInfo));
      std::memcpy(&mappedBuffer->opticalInfo, &opticsInfo, sizeof(BasicOptics));
      context->Unmap(marchBuffer.Get(), 0);
    }

    context->CSSetConstantBuffers(1, 1, marchBuffer.GetAddressOf());
    context->CSSetConstantBuffers(2, 1, lightBuffer.GetAddressOf());

    context->CSSetSamplers(0, 1, marchSamplerState.GetAddressOf());
    context->CSSetShaderResources(0, 1, &densityTexResource);
  }

  void RaymarchVolumeShader::unbindShader(ID3D11DeviceContext* context)
  {
    computeShader->unbindShader(context);

    void* nullpo = nullptr;
    context->CSSetUnorderedAccessViews(0, 1, (ID3D11UnorderedAccessView**)&nullpo, 0);
    context->CSSetConstantBuffers(0, 1, (ID3D11Buffer**)&nullpo);
    context->CSSetConstantBuffers(1, 1, (ID3D11Buffer**)&nullpo);
    context->CSSetConstantBuffers(2, 1, (ID3D11Buffer**)&nullpo);
    context->CSSetSamplers(0, 1, (ID3D11SamplerState**)&nullpo);
    context->CSSetShaderResources(0, 1, (ID3D11ShaderResourceView**)&nullpo);
  }

  void RaymarchVolumeShader::render(ID3D11DeviceContext* context) const
  {
    computeShader->dispatch(context, renderTarget->getWidth(), renderTarget->getHeight());
  }

  VolumeGenerationShader::VolumeGenerationShader()
  {
    generateVolumeShader = new Shader(Shader::Type::Compute, L"TestShaders/TestFormHaboob");
  }

  VolumeGenerationShader::~VolumeGenerationShader()
  {
    delete generateVolumeShader; generateVolumeShader = nullptr;
  }
  HRESULT VolumeGenerationShader::initShader(ID3D11Device* device, const ShaderManager* manager)
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
      volumeTextureDesc.MipLevels = 1;
      volumeTextureDesc.Format = DXGI_FORMAT_R32_FLOAT;
      volumeTextureDesc.Usage = D3D11_USAGE_DEFAULT;
      volumeTextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;

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
      shaderViewDesc.Texture3D.MipLevels = 1;

      result = device->CreateShaderResourceView(texture.Get(), &shaderViewDesc, textureShaderView.ReleaseAndGetAddressOf());
      Firebreak(result);
    }

    // Create the compute unordered access view
    {
      D3D11_UNORDERED_ACCESS_VIEW_DESC computeAccessDesc;
      ZeroMemory(&computeAccessDesc, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC));
      computeAccessDesc.Format = DXGI_FORMAT_R32_FLOAT;
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
    static const int groupSize = 8;
    Shader::dispatch(context, volumeInfo.size.x / groupSize, volumeInfo.size.y / groupSize, volumeInfo.size.z / groupSize);
    unbindShader(context);
    generateVolumeShader->unbindShader(context);
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
