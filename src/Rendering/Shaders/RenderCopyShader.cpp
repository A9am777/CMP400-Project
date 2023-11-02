#include "Rendering/Shaders/RenderCopyShader.h"

namespace Haboob
{
  RenderCopyShader::RenderCopyShader()
  {
    vertexShader = new Shader(Shader::Type::Vertex, L"Utility/ScreenQuad");
    pixelShader = new Shader(Shader::Type::Pixel, L"Utility/OverdrawTexture");

    matrices.projectionMatrix = XMMatrixIdentity();
    matrices.viewMatrix = XMMatrixIdentity();
    matrices.worldMatrix = XMMatrixIdentity();
  }

  RenderCopyShader::~RenderCopyShader()
  {
    delete vertexShader; vertexShader = nullptr;
    delete pixelShader; pixelShader = nullptr;
  }

  HRESULT RenderCopyShader::initShader(ID3D11Device* device, const ShaderManager* manager)
  {
    HRESULT result = S_OK;

    result = quad.build(device);
    Firebreak(result);

    result = vertexShader->initShader(device, manager);
    Firebreak(result);

    result = pixelShader->initShader(device, manager);
    Firebreak(result);

    // Create the matrices buffer
    {
      D3D11_BUFFER_DESC cameraBufferDesc;
      cameraBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
      cameraBufferDesc.ByteWidth = sizeof(CameraPack);
      cameraBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
      cameraBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
      cameraBufferDesc.MiscFlags = 0;
      cameraBufferDesc.StructureByteStride = 0;
      result = device->CreateBuffer(&cameraBufferDesc, NULL, cameraBuffer.ReleaseAndGetAddressOf());
      Firebreak(result);
    }

    // Create the sampler
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

    return result;
  }

  void RenderCopyShader::bindShader(ID3D11DeviceContext* context)
  {
    vertexShader->bindShader(context);
    pixelShader->bindShader(context);

    // Copy over the matrices
    {
      D3D11_MAPPED_SUBRESOURCE mapped;
      HRESULT unused = context->Map(cameraBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
      std::memcpy(mapped.pData, &matrices, sizeof(CameraPack));
      context->Unmap(cameraBuffer.Get(), 0);
    }

    context->VSSetConstantBuffers(0, 1, cameraBuffer.GetAddressOf());
    context->PSSetSamplers(0, 1, sampler.GetAddressOf());

    quad.useBuffers(context);
  }

  void RenderCopyShader::render(ID3D11DeviceContext* context) const
  {
    // Make sure to send in the matrices
    quad.draw(context);
  }

  void RenderCopyShader::setViewport(const D3D11_VIEWPORT& view)
  {
    matrices.worldMatrix = XMMatrixScaling(float(view.Width), float(view.Height), 1.f) * XMMatrixTranslation(view.TopLeftX, view.TopLeftY, .0f);
  }

}
