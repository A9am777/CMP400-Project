#include "Rendering/Shaders/ShaderManager.h"
#include "Rendering/Textures/GBuffer.h"

namespace Haboob
{
  ToneMapShader GBuffer::toneMapShader;
  LightPassShader GBuffer::lightShader;

  GBuffer::GBuffer() : gamma{.25f}, exposure{.5f}
  {

  }

  void GBuffer::setTargets(ID3D11DeviceContext* context, ID3D11DepthStencilView* depthStencil)
  {
    static constexpr UInt targetCount = 2;
    ID3D11RenderTargetView* targets[2] = { diffuseTarget.getRenderTarget(), normalDepthTarget.getRenderTarget() };
    D3D11_VIEWPORT viewports[2] = { diffuseTarget.getViewport(), normalDepthTarget.getViewport() };

    context->OMSetRenderTargets(targetCount, targets, depthStencil);
    context->RSSetViewports(targetCount, viewports);
  }

  void GBuffer::clear(ID3D11DeviceContext* context)
  {
    static float normalClearColour[4] = { .0f, .0f, -1.f, .0f };

    diffuseTarget.clear(context, RenderTarget::defaultBlack);
    normalDepthTarget.clear(context, normalClearColour);
    litColourTarget.clear(context, RenderTarget::defaultBlack);
  }

  HRESULT GBuffer::create(ID3D11Device* device, UInt width, UInt height)
  {
    // Currently no need to use custom texture descriptions however
    // any reasonable application will be selective about GBuffer construction
    HRESULT result = S_OK;

    result = diffuseTarget.create(device, width, height);
    Firebreak(result);
    result = normalDepthTarget.create(device, width, height);
    Firebreak(result);

    result = litColourTarget.create(device, width, height);
    Firebreak(result);

    return result;
  }

  HRESULT GBuffer::resize(ID3D11Device* device, UInt width, UInt height)
  {
    HRESULT result = S_OK;

    result = diffuseTarget.resize(device, width, height);
    Firebreak(result);
    result = normalDepthTarget.resize(device, width, height);
    Firebreak(result);

    result = litColourTarget.resize(device, width, height);
    Firebreak(result);

    return result;
  }

  void GBuffer::lightPass(ID3D11DeviceContext* context, ID3D11Buffer* lightbuffer)
  {
    ID3D11ShaderResourceView* textureResources[2] = { diffuseTarget.getShaderView(), normalDepthTarget.getShaderView() };
    ID3D11UnorderedAccessView* outTarget = litColourTarget.getComputeView();
    
    context->CSSetShaderResources(0, 2, textureResources);
    context->CSSetUnorderedAccessViews(0, 1, &outTarget, nullptr);
    lightShader.bindShader(context, lightbuffer);
      Shader::dispatch(context, litColourTarget.getWidth(), litColourTarget.getHeight());
    lightShader.unbindShader(context);

    std::memset(textureResources, 0, sizeof(textureResources));
    outTarget = nullptr;
    context->CSSetShaderResources(0, 2, textureResources);
    context->CSSetUnorderedAccessViews(0, 1, &outTarget, nullptr);
  }

  void GBuffer::finalLitPass(ID3D11DeviceContext* context)
  {
    toneMapShader.setGamma(gamma);
    toneMapShader.setExposure(exposure);

    toneMapShader.bindShader(context, litColourTarget.getComputeView());
      Shader::dispatch(context, litColourTarget.getWidth(), litColourTarget.getHeight());
    toneMapShader.unbindShader(context);
  }

  void GBuffer::renderFromLit(ID3D11DeviceContext* context)
  {
    litColourTarget.renderFrom(context);
  }

  HRESULT GBuffer::capture(ID3D11Device* device, ID3D11DeviceContext* context, std::vector<Byte>& buffer, UInt& width, UInt& height)
  {
    HRESULT result = S_OK;

    auto texture = litColourTarget.getTexture();

    ComPtr<ID3D11Texture2D> stageTexture;
    {
      D3D11_TEXTURE2D_DESC stageTextureDesc;
      texture->GetDesc(&stageTextureDesc);

      width = stageTextureDesc.Width;
      height = stageTextureDesc.Height;

      stageTextureDesc.MipLevels = 1;
      stageTextureDesc.BindFlags = 0;
      stageTextureDesc.Usage = D3D11_USAGE_STAGING;
      stageTextureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

      result = device->CreateTexture2D(&stageTextureDesc, NULL, stageTexture.ReleaseAndGetAddressOf());
    }
    Firebreak(result);

    D3D11_BOX sourceRegion;
    sourceRegion.left = 0;
    sourceRegion.right = width;
    sourceRegion.top = 0;
    sourceRegion.bottom = height;
    sourceRegion.front = 0;
    sourceRegion.back = 1;

    context->CopySubresourceRegion(stageTexture.Get(), 0, 0, 0, 0, texture, 0, &sourceRegion);

    D3D11_MAPPED_SUBRESOURCE mapped;
    result = context->Map(stageTexture.Get(), 0, D3D11_MAP_READ, 0, &mapped);

    if (mapped.pData)
    {
      UInt rowSize = width * 8;
      buffer.resize(rowSize * height);

      // Copy the image row by row (D3D uses padding)
      Byte* srcProgress = (Byte*)mapped.pData;
      Byte* dstProgress = buffer.data();
      for (size_t i = 0; i < height; ++i)
      {
        std::memcpy(dstProgress, srcProgress, rowSize);

        dstProgress += rowSize;
        srcProgress += mapped.RowPitch;
      }
    }

    context->Unmap(stageTexture.Get(), 0);

    return result;
  }

  HRESULT ToneMapShader::initShader(ID3D11Device* device, ShaderManager* manager)
  {
    HRESULT result = S_OK;
    
    result = Shader::initShader(device, manager);
    Firebreak(result);

    // Create the tone map buffer
    {
      D3D11_BUFFER_DESC toneBufferDesc;
      toneBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
      toneBufferDesc.ByteWidth = sizeof(TonePack);
      toneBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
      toneBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
      toneBufferDesc.MiscFlags = 0;
      toneBufferDesc.StructureByteStride = 0;
      result = device->CreateBuffer(&toneBufferDesc, NULL, toneBuffer.ReleaseAndGetAddressOf());
      Firebreak(result);
    }

    return result;
  }

  void ToneMapShader::bindShader(ID3D11DeviceContext* context, ID3D11UnorderedAccessView* uav)
  {
    Shader::bindShader(context);
    context->CSSetUnorderedAccessViews(0, 1, &uav, 0);

    // Update tone data
    {
      D3D11_MAPPED_SUBRESOURCE mapped;
      HRESULT result = context->Map(toneBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
      std::memcpy(mapped.pData, &toneInfo, sizeof(TonePack));
      context->Unmap(toneBuffer.Get(), 0);
    }

    context->CSSetConstantBuffers(0, 1, toneBuffer.GetAddressOf());
  }

  void ToneMapShader::unbindShader(ID3D11DeviceContext* context)
  {
    Shader::unbindShader(context);

    void* nullpo = nullptr;
    context->CSSetUnorderedAccessViews(0, 1, (ID3D11UnorderedAccessView**)&nullpo, 0);
    context->CSSetConstantBuffers(0, 1, (ID3D11Buffer**)&nullpo);
  }

  void LightPassShader::bindShader(ID3D11DeviceContext* context, ID3D11Buffer* lightbuffer)
  {
    Shader::bindShader(context);
    context->CSSetConstantBuffers(0, 1, &lightbuffer);
  }

  void LightPassShader::unbindShader(ID3D11DeviceContext* context)
  {
    Shader::unbindShader(context);

    void* nullpo = nullptr;
    context->CSSetConstantBuffers(0, 1, (ID3D11Buffer**)&nullpo);
  }
}
