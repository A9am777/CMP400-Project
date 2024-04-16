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
    static constexpr UInt targetCount = 3;
    ID3D11RenderTargetView* targets[targetCount] = { diffuseTarget.getRenderTarget(), normalDepthTarget.getRenderTarget(), worldPositionTarget.getRenderTarget()};
    D3D11_VIEWPORT viewports[targetCount] = { diffuseTarget.getViewport(), normalDepthTarget.getViewport(), worldPositionTarget.getViewport() };

    context->OMSetRenderTargets(targetCount, targets, depthStencil);
    context->RSSetViewports(targetCount, viewports);
  }

  void GBuffer::clear(ID3D11DeviceContext* context)
  {
    static float normalClearColour[4] = { .0f, .0f, -1.f, 1.f };

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
    result = worldPositionTarget.create(device, width, height);
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
    result = worldPositionTarget.resize(device, width, height);
    Firebreak(result);

    result = litColourTarget.resize(device, width, height);
    Firebreak(result);

    return result;
  }

  void GBuffer::lightPass(ID3D11DeviceContext* context, ID3D11Buffer* lightbuffer, ID3D11Buffer* lightCameraBuffer, ID3D11ShaderResourceView* lightShadowMap, ID3D11ShaderResourceView* beerShadowMap, ID3D11SamplerState* shadowSampler, ID3D11Buffer* marchBuffer)
  {
    ID3D11ShaderResourceView* textureResources[5] = { diffuseTarget.getShaderView(), normalDepthTarget.getShaderView(), worldPositionTarget.getShaderView(), lightShadowMap, beerShadowMap};
    ID3D11UnorderedAccessView* outTarget = litColourTarget.getComputeView();
    
    context->CSSetSamplers(0, 1, &shadowSampler);
    context->CSSetShaderResources(0, 5, textureResources);
    context->CSSetUnorderedAccessViews(0, 1, &outTarget, nullptr);
    context->CSSetConstantBuffers(2, 1, &marchBuffer);
    lightShader.bindShader(context, lightbuffer, lightCameraBuffer);
      Shader::dispatch(context, 1 + litColourTarget.getWidth() / 8, 1 + litColourTarget.getHeight() / 8);
    lightShader.unbindShader(context);

    std::memset(textureResources, 0, sizeof(textureResources));
    outTarget = nullptr;
    shadowSampler = nullptr;
    marchBuffer = nullptr;
    context->CSSetSamplers(0, 1, &shadowSampler);
    context->CSSetShaderResources(0, 5, textureResources);
    context->CSSetUnorderedAccessViews(0, 1, &outTarget, nullptr);
    context->CSSetConstantBuffers(2, 1, &marchBuffer);
  }

  void GBuffer::finalLitPass(ID3D11DeviceContext* context)
  {
    toneMapShader.setGamma(gamma);
    toneMapShader.setExposure(exposure);

    toneMapShader.bindShader(context, litColourTarget.getComputeView());
      Shader::dispatch(context, 1 + litColourTarget.getWidth() / 8, 1 + litColourTarget.getHeight() / 8);
    toneMapShader.unbindShader(context);
  }

  void GBuffer::renderFromLit(ID3D11DeviceContext* context)
  {
    litColourTarget.renderFrom(context);
  }

  HRESULT GBuffer::capture(const std::wstring& path, ID3D11DeviceContext* context)
  {
    return DirectX::SaveDDSTextureToFile(context, litColourTarget.getTexture(), path.c_str());
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

  void LightPassShader::bindShader(ID3D11DeviceContext* context, ID3D11Buffer* lightbuffer, ID3D11Buffer* lightCameraBuffer)
  {
    Shader::bindShader(context);
    context->CSSetConstantBuffers(0, 1, &lightbuffer);
    context->CSSetConstantBuffers(1, 1, &lightCameraBuffer);
  }

  void LightPassShader::unbindShader(ID3D11DeviceContext* context)
  {
    Shader::unbindShader(context);

    void* nullpo = nullptr;
    context->CSSetConstantBuffers(0, 1, (ID3D11Buffer**)&nullpo);
  }
}
