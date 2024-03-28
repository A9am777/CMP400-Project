#pragma once
#include "Rendering/Geometry/MeshRenderer.h"

namespace Haboob
{
  template<typename VertexT>
  inline MeshInstance<VertexT>::MeshInstance(Mesh<VertexT>* mesh) : baseMesh{ mesh }, visible{ true }
  {
    transform = XMMatrixIdentity();
    position = XMFLOAT3{ .0f, .0f, .0f };
    scale = XMFLOAT3{ 1.f, 1.f, 1.f };
    quat = XMFLOAT4{ .0f, .0f, .0f, 1.f };
  }

  template<typename VertexT>
  inline void MeshInstance<VertexT>::buildTransform()
  {
    auto positionLoad = XMLoadFloat3(&position);
    auto scaleLoad = XMLoadFloat3(&scale);
    auto quatLoad = XMLoadFloat4(&quat);
    transform = XMMatrixTransformation(XMVectorZero(), XMQuaternionIdentity(), scaleLoad, XMVectorZero(), quatLoad, positionLoad);
  }

  template<typename VertexT>
  inline MeshRenderer<VertexT>::MeshRenderer()
  {
    deferredVertexShader = new Shader(Shader::Type::Vertex, L"Raster/DeferredMeshShaderV", true);
    deferredPixelShader = new Shader(Shader::Type::Pixel, L"Raster/DeferredMeshShaderP", true);
    useDeferredContext = true;
  }

  template<typename VertexT>
  inline MeshRenderer<VertexT>::~MeshRenderer()
  {
    delete deferredVertexShader;
    delete deferredPixelShader;
  }

  template<typename VertexT>
  inline void MeshRenderer<VertexT>::initShaders(ID3D11Device* device, ShaderManager* manager)
  {
    deferredVertexShader->initShader(device, manager);
    deferredPixelShader->initShader(device, manager);
  }

  template<typename VertexT>
  inline void MeshRenderer<VertexT>::start()
  {
    cachedMesh = nullptr;
  }

  template<typename VertexT>
  inline void MeshRenderer<VertexT>::bind(ID3D11DeviceContext* context)
  {
    deferredVertexShader->bindShader(context);
    deferredPixelShader->bindShader(context);
  }

  template<typename VertexT>
  inline void MeshRenderer<VertexT>::draw(ID3D11DeviceContext* context, MeshInstance<VertexT>* instance)
  {
    if (!instance->visible) { return; }

    uploadTransformFunc(instance->transform);
    if (instance->baseMesh != cachedMesh)
    {
      cachedMesh = instance->baseMesh;
      cachedMesh->useBuffers(context);
    }
    
    cachedMesh->draw(context);
  }
  template<typename VertexT>
  inline void MeshRenderer<VertexT>::unbind(ID3D11DeviceContext* context)
  {
    deferredVertexShader->unbindShader(context);
    deferredPixelShader->unbindShader(context);
  }
}