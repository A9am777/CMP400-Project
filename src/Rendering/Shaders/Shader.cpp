#include "Rendering/Shaders/Shader.h"

namespace Haboob
{
  Shader::Shader(Type shaderType, const wchar_t* path)
  {
    type = shaderType;
    relativePath = path;
    compiledShader = nullptr;
  }

  Shader::~Shader()
  {
    if (compiledShader)
    {
      compiledShader->Release();
      compiledShader = nullptr;
    }
  }

  HRESULT Shader::initShader(DisplayDevice* device, const ShaderManager* manager)
  {
    if (compiledShader)
    {
      compiledShader->Release();
      compiledShader = nullptr;
    }

    return makeShader(&compiledShader, device, manager);
  }

  void Shader::bindShader(ID3D11DeviceContext* context)
  {
    switch (type)
    {
      case Type::Vertex:
        bindVertex(context);
      break;
      case Type::Pixel:
        bindPixel(context);
      break;
      case Type::Hull:
        bindHull(context);
      break;
      case Type::Domain:
        bindDomain(context);
      break;
      case Type::Geometry:
        bindGeometry(context);
      break;
      case Type::Compute:
        bindCompute(context);
      break;
    }
  }

  HRESULT Shader::makeShader(ID3D11DeviceChild** shader, DisplayDevice* device, const ShaderManager* manager)
  {
    HRESULT result = S_OK;

    // Load the raw shader file
    ComPtr<ID3DBlob> shaderBlob;
    result = manager->loadShaderBlob(relativePath, shaderBlob.GetAddressOf());
    Firebreak(result);

    switch (type)
    {
      case Type::Vertex:
      {
        result = device->getDevice()->CreateVertexShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), NULL, reinterpret_cast<ID3D11VertexShader**>(shader));
        Firebreak(result);

        // Link the specified vertex layout
        auto& layout = manager->getVertexLayout();
        result = device->getDevice()->CreateInputLayout(layout.data(), layout.size(), shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), &inputLayout);
      }
      break;
      case Type::Pixel:
        result = device->getDevice()->CreatePixelShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), NULL, reinterpret_cast<ID3D11PixelShader**>(shader));
      break;
      case Type::Hull:
        result = device->getDevice()->CreateHullShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), NULL, reinterpret_cast<ID3D11HullShader**>(shader));
      break;
      case Type::Domain:
        result = device->getDevice()->CreateDomainShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), NULL, reinterpret_cast<ID3D11DomainShader**>(shader));
      break;
      case Type::Geometry:
        result = device->getDevice()->CreateGeometryShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), NULL, reinterpret_cast<ID3D11GeometryShader**>(shader));
      break;
      case Type::Compute:
        result = device->getDevice()->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), NULL, reinterpret_cast<ID3D11ComputeShader**>(shader));
      break;
    }

    return result;
  }

  void Shader::bindVertex(ID3D11DeviceContext* context)
  {
    context->IASetInputLayout(inputLayout.Get());
    context->VSSetShader(reinterpret_cast<ID3D11VertexShader*>(compiledShader), NULL, 0);
  }

  void Shader::bindPixel(ID3D11DeviceContext* context)
  {
    context->PSSetShader(reinterpret_cast<ID3D11PixelShader*>(compiledShader), NULL, 0);
  }

  void Shader::bindHull(ID3D11DeviceContext* context)
  {
    context->HSSetShader(reinterpret_cast<ID3D11HullShader*>(compiledShader), NULL, 0);
  }

  void Shader::bindDomain(ID3D11DeviceContext* context)
  {
    context->DSSetShader(reinterpret_cast<ID3D11DomainShader*>(compiledShader), NULL, 0);
  }

  void Shader::bindGeometry(ID3D11DeviceContext* context)
  {
    context->GSSetShader(reinterpret_cast<ID3D11GeometryShader*>(compiledShader), NULL, 0);
  }

  void Shader::bindCompute(ID3D11DeviceContext* context)
  {
    context->CSSetShader(reinterpret_cast<ID3D11ComputeShader*>(compiledShader), NULL, 0);
  }

}
