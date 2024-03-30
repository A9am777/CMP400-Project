#pragma once
#include "Rendering/Lighting/LightSource.h"

namespace Haboob
{
  Light::Light()
  {
    nearZ = .01f;
    farZ = 15.f;
    ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

    renderPosition = { 6.f, 1.f, .0f };
  }

  HRESULT Light::create(ID3D11Device* device, UInt width, UInt height)
  {
    HRESULT result = S_OK;

    // Create buffers
    {
      D3D11_BUFFER_DESC bufferDesc;

      // Create light buffer
      bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
      bufferDesc.ByteWidth = sizeof(DirectionalLightPack);
      bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
      bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
      bufferDesc.MiscFlags = 0;
      bufferDesc.StructureByteStride = 0;
      result = device->CreateBuffer(&bufferDesc, NULL, lightBuffer.ReleaseAndGetAddressOf());

      // Camera buffer
      bufferDesc.ByteWidth = sizeof(CameraPack);
      result = device->CreateBuffer(&bufferDesc, NULL, cameraBuffer.ReleaseAndGetAddressOf());
    }
    Firebreak(result);

    // Create the shadow sampler
    {
      D3D11_SAMPLER_DESC samplerDesc;
      ZeroMemory(&samplerDesc, sizeof(D3D11_SAMPLER_DESC));

      samplerDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
      samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
      samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
      samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
      samplerDesc.MipLODBias = 0.0f;
      samplerDesc.MaxAnisotropy = 1;
      samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
      samplerDesc.MinLOD = 0;
      samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

      // Create the texture sampler state.
      result = device->CreateSamplerState(&samplerDesc, sampler.ReleaseAndGetAddressOf());
      Firebreak(result);
    }

    resize(device, width, height);
    Firebreak(result);

    return result;
  }

  HRESULT Light::resize(ID3D11Device* device, UInt width, UInt height)
  {
    HRESULT result = S_OK;

    if (float(width) == viewport.Width && float(height) == viewport.Height) { return result; }

    // Depth stencil texture
    D3D11_TEXTURE2D_DESC depthBufferDesc;
    {
      ZeroMemory(&depthBufferDesc, sizeof(D3D11_TEXTURE2D_DESC));

      depthBufferDesc.Width = width;
      depthBufferDesc.Height = height;
      depthBufferDesc.MipLevels = 1;
      depthBufferDesc.ArraySize = 1;
      depthBufferDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
      depthBufferDesc.SampleDesc.Count = 1;
      depthBufferDesc.SampleDesc.Quality = 0;
      depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
      depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
      depthBufferDesc.CPUAccessFlags = 0;
      depthBufferDesc.MiscFlags = 0;

      result = device->CreateTexture2D(&depthBufferDesc, NULL, depthMapTexture.ReleaseAndGetAddressOf());
      Firebreak(result);
    }

    // Depth stencil view
    {
      D3D11_DEPTH_STENCIL_VIEW_DESC depthViewDesc;
      ZeroMemory(&depthViewDesc, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));

      depthViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
      depthViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
      depthViewDesc.Texture2D.MipSlice = 0;

      result = device->CreateDepthStencilView(depthMapTexture.Get(), &depthViewDesc, mapBufferView.ReleaseAndGetAddressOf());
      Firebreak(result);
    }

    // Create the shader view
    {
      D3D11_SHADER_RESOURCE_VIEW_DESC shaderViewDesc;
      ZeroMemory(&shaderViewDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));

      shaderViewDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
      shaderViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
      shaderViewDesc.Texture2D.MipLevels = 1;

      result = device->CreateShaderResourceView(depthMapTexture.Get(), &shaderViewDesc, mapShaderView.ReleaseAndGetAddressOf());
      Firebreak(result);
    }

    // Create the viewport
    {
      viewport.TopLeftX = viewport.TopLeftY = .0f;
      viewport.Width = float(width);
      viewport.Height = float(height);
      viewport.MinDepth = .0f;
      viewport.MaxDepth = 1.f;
    }

    camera.setProjection(XMMatrixOrthographicLH(farZ * viewport.Width / viewport.Height, farZ * viewport.Height / viewport.Width, nearZ, farZ));

    return result;
  }

  void Light::setTarget(ID3D11DeviceContext* context)
  {
    ID3D11RenderTargetView* nullTargets = nullptr;

    context->ClearDepthStencilView(mapBufferView.Get(), D3D11_CLEAR_DEPTH, 1.f, 0);
    context->OMSetRenderTargets(1, &nullTargets, mapBufferView.Get());
    context->RSSetViewports(1, &viewport);
  }

  HRESULT Light::rebuildLightBuffers(ID3D11DeviceContext* context)
  {
    HRESULT result = S_OK;

    // Light data
    {
      D3D11_MAPPED_SUBRESOURCE mapped;
      result = context->Map(lightBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
      std::memcpy(mapped.pData, &lightPack, sizeof(DirectionalLightPack));

      // Normalise direction before sending
      XMVECTOR vec = XMLoadFloat4(&lightPack.direction);
      vec = XMVector3Normalize(vec);
      XMStoreFloat4(&reinterpret_cast<DirectionalLightPack*>(mapped.pData)->direction, vec);

      context->Unmap(lightBuffer.Get(), 0);
    }

    // Camera data
    {
      D3D11_MAPPED_SUBRESOURCE mapped;
      result = context->Map(cameraBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
      camera.putPack(mapped.pData);
      context->Unmap(cameraBuffer.Get(), 0);
    }

    return result;
  }

  void Light::updateCameraView()
  {
    XMVECTOR positionLoad = XMLoadFloat3(&renderPosition);
    XMVECTOR upLoad = XMVectorSet(.0f, 1.f, .0f, 1.f);
    upLoad = XMVector3Normalize(upLoad);
    XMVECTOR forwardLoad = XMLoadFloat4(&lightPack.direction);
    forwardLoad = XMVector3Normalize(forwardLoad);

    // TODO: for some reason this doesn't exactly work
    camera.setView(XMMatrixLookToLH(positionLoad, forwardLoad, upLoad));
  }

}