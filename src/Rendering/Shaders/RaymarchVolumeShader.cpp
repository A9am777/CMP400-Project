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

    return result;
  }

  void RaymarchVolumeShader::bindShader(ID3D11DeviceContext* context)
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
  }

  void RaymarchVolumeShader::unbindShader(ID3D11DeviceContext* context)
  {
    computeShader->unbindShader(context);

    void* nullpo = nullptr;
    context->CSSetUnorderedAccessViews(0, 1, (ID3D11UnorderedAccessView**)&nullpo, 0);
    context->CSSetConstantBuffers(0, 1, (ID3D11Buffer**)&nullpo);
    context->CSSetConstantBuffers(1, 1, (ID3D11Buffer**)&nullpo);
    context->CSSetConstantBuffers(2, 1, (ID3D11Buffer**)&nullpo);
  }

  void RaymarchVolumeShader::render(ID3D11DeviceContext* context) const
  {
    computeShader->dispatch(context, renderTarget->getWidth(), renderTarget->getHeight());
  }
}
