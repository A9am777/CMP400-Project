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

  HRESULT Shader::initShader(ID3D11Device* device, const ShaderManager* manager)
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

  void Shader::unbindShader(ID3D11DeviceContext* context)
  {
    switch (type)
    {
      case Type::Vertex:
        unbindVertex(context);
        break;
      case Type::Pixel:
        unbindPixel(context);
        break;
      case Type::Hull:
        unbindHull(context);
        break;
      case Type::Domain:
        unbindDomain(context);
        break;
      case Type::Geometry:
        unbindGeometry(context);
        break;
      case Type::Compute:
        unbindCompute(context);
        break;
    }
  }

  void Shader::dispatch(ID3D11DeviceContext* context, UInt groupX, UInt groupY, UInt groupZ)
  {
    context->Dispatch(groupX, groupY, groupZ);
  }

  HRESULT Shader::makeShader(ID3D11DeviceChild** shader, ID3D11Device* device, const ShaderManager* manager)
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
        result = device->CreateVertexShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), NULL, reinterpret_cast<ID3D11VertexShader**>(shader));
        Firebreak(result);

        // Link the specified vertex layout
        auto& layout = manager->getVertexLayout();
        result = device->CreateInputLayout(layout.data(), layout.size(), shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), &inputLayout);
      }
      break;
      case Type::Pixel:
        result = device->CreatePixelShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), NULL, reinterpret_cast<ID3D11PixelShader**>(shader));
      break;
      case Type::Hull:
        result = device->CreateHullShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), NULL, reinterpret_cast<ID3D11HullShader**>(shader));
      break;
      case Type::Domain:
        result = device->CreateDomainShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), NULL, reinterpret_cast<ID3D11DomainShader**>(shader));
      break;
      case Type::Geometry:
        result = device->CreateGeometryShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), NULL, reinterpret_cast<ID3D11GeometryShader**>(shader));
      break;
      case Type::Compute:
        result = device->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), NULL, reinterpret_cast<ID3D11ComputeShader**>(shader));
      break;
    }

    return result;
  }

  void Shader::bindVertex(ID3D11DeviceContext* context)
  {
    context->IASetInputLayout(inputLayout.Get());
    context->VSSetShader(reinterpret_cast<ID3D11VertexShader*>(compiledShader), NULL, 0);
  }

  void Shader::unbindVertex(ID3D11DeviceContext* context)
  {
    context->IASetInputLayout(nullptr);
    context->VSSetShader(nullptr, NULL, 0);
  }

  void Shader::bindPixel(ID3D11DeviceContext* context)
  {
    context->PSSetShader(reinterpret_cast<ID3D11PixelShader*>(compiledShader), NULL, 0);
  }

  void Shader::unbindPixel(ID3D11DeviceContext* context)
  {
    context->PSSetShader(nullptr, NULL, 0);
  }

  void Shader::bindHull(ID3D11DeviceContext* context)
  {
    context->HSSetShader(reinterpret_cast<ID3D11HullShader*>(compiledShader), NULL, 0);
  }

  void Shader::unbindHull(ID3D11DeviceContext* context)
  {
    context->HSSetShader(nullptr, NULL, 0);
  }

  void Shader::bindDomain(ID3D11DeviceContext* context)
  {
    context->DSSetShader(reinterpret_cast<ID3D11DomainShader*>(compiledShader), NULL, 0);
  }

  void Shader::unbindDomain(ID3D11DeviceContext* context)
  {
    context->DSSetShader(nullptr, NULL, 0);
  }

  void Shader::bindGeometry(ID3D11DeviceContext* context)
  {
    context->GSSetShader(reinterpret_cast<ID3D11GeometryShader*>(compiledShader), NULL, 0);
  }

  void Shader::unbindGeometry(ID3D11DeviceContext* context)
  {
    context->GSSetShader(nullptr, NULL, 0);
  }

  void Shader::bindCompute(ID3D11DeviceContext* context)
  {
    context->CSSetShader(reinterpret_cast<ID3D11ComputeShader*>(compiledShader), NULL, 0);
  }
  void Shader::unbindCompute(ID3D11DeviceContext* context)
  {
    context->CSSetShader(nullptr, NULL, 0);
  }
}
