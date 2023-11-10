#include "Rendering/Textures/RenderTarget.h"

namespace Haboob
{
  D3D11_TEXTURE2D_DESC RenderTarget::defaultTextureDesc{ 
    255,
    255,
    4,
    1,
    DXGI_FORMAT_R32G32B32A32_FLOAT,
    {1, 0},
    D3D11_USAGE_DEFAULT,
    D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS,
    0,
    0 
  };

  float RenderTarget::defaultBlack[4] = { .0f, .0f, .0f, .0f };
  float RenderTarget::defaultWhite[4] = { 1.f, 1.f, 1.f, 1.f };
  float RenderTarget::defaultRed[4] = { 1.f, .1f, .1f, 1.f };

  RenderCopyShader RenderTarget::copyShader;

  RenderTarget::RenderTarget()
  {
    textureViewport.TopLeftX = textureViewport.TopLeftY = .0f;
    textureViewport.Width = 255;
    textureViewport.Height = 255;
    textureViewport.MinDepth = .0f;
    textureViewport.MaxDepth = 1.f;
  }

  void RenderTarget::setTarget(ID3D11DeviceContext* context, ID3D11DepthStencilView* depthStencil)
  {
    context->OMSetRenderTargets(1, textureTarget.GetAddressOf(), depthStencil);
    context->RSSetViewports(1, &textureViewport);
  }

  void RenderTarget::clear(ID3D11DeviceContext* context, float colour[4])
  {
    context->ClearRenderTargetView(textureTarget.Get(), colour);
  }

  HRESULT RenderTarget::create(ID3D11Device* device, UInt width, UInt height, const D3D11_TEXTURE2D_DESC& textureDesc)
  {
    HRESULT result = S_OK;

    // Create the texture
    {
      D3D11_TEXTURE2D_DESC scaledTextureDesc = textureDesc;
      scaledTextureDesc.Width = width;
      scaledTextureDesc.Height = height;

      result = device->CreateTexture2D(&scaledTextureDesc, nullptr, texture.ReleaseAndGetAddressOf());
      Firebreak(result);
    }

    // Create the viewport
    {
      ZeroMemory(&textureViewport, sizeof(D3D11_VIEWPORT));

      textureViewport.Width = float(width);
      textureViewport.Height = float(height);
      textureViewport.MinDepth = .0f;
      textureViewport.MaxDepth = 1.f;
    }

    // Create the target
    {
      D3D11_RENDER_TARGET_VIEW_DESC renderTargetDesc;
      ZeroMemory(&renderTargetDesc, sizeof(D3D11_RENDER_TARGET_VIEW_DESC));

      renderTargetDesc.Format = textureDesc.Format;
      renderTargetDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

      result = device->CreateRenderTargetView(texture.Get(), &renderTargetDesc, textureTarget.ReleaseAndGetAddressOf());
      Firebreak(result);
    }

    // Create the shader view
    {
      D3D11_SHADER_RESOURCE_VIEW_DESC shaderViewDesc;
      ZeroMemory(&shaderViewDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));

      shaderViewDesc.Format = textureDesc.Format;
      shaderViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
      shaderViewDesc.Texture2D.MipLevels = 1;

      result = device->CreateShaderResourceView(texture.Get(), &shaderViewDesc, textureShaderView.ReleaseAndGetAddressOf());
      Firebreak(result);
    }

    // Create the compute unordered access view
    {
      D3D11_UNORDERED_ACCESS_VIEW_DESC computeAccessDesc;
      ZeroMemory(&computeAccessDesc, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC));
      computeAccessDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
      computeAccessDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
      computeAccessDesc.Texture2D.MipSlice = 0;

      result = device->CreateUnorderedAccessView(texture.Get(), &computeAccessDesc, computeAccessView.ReleaseAndGetAddressOf());
      Firebreak(result);
    }

    return result;
  }
  HRESULT RenderTarget::resize(ID3D11Device* device, UInt width, UInt height)
  {
    if (!texture) { return E_INVALIDARG; }

    D3D11_TEXTURE2D_DESC textureDesc;
    texture->GetDesc(&textureDesc);

    return create(device, width, height, textureDesc);
  }
  void RenderTarget::renderFrom(ID3D11DeviceContext* context)
  {
    copyShader.setViewport(textureViewport);
    copyShader.bindShader(context);
    
    ID3D11ShaderResourceView* textureView = textureShaderView.Get();
    context->PSSetShaderResources(0, 1, &textureView);
    
    copyShader.render(context);

    textureView = nullptr;
    context->PSSetShaderResources(0, 1, &textureView);
  }
}
