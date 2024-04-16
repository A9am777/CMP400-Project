#include "Rendering/Scene/Scene.h"
#include "Rendering/Lighting/LightStructs.h"
#include "Rendering/Scene/SceneStructs.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <imgui.h>

namespace Haboob
{
  Scene::Scene()
  {
    
  }

  Scene::~Scene()
  {
    for (auto meshInstance : meshInstances)
    {
      delete meshInstance;
    }
    meshInstances.clear();

  }

  void Scene::addMesh(const std::string& name, Mesh<VertexType>* mesh)
  {
    meshes.push_back({ name, mesh });
  }

  void Scene::addObject(MeshInstance<VertexType>* instance)
  {
    meshInstances.push_back(instance);
  }

  HRESULT Scene::init(ID3D11Device* device, ShaderManager* manager)
  {
    HRESULT result = S_OK;

    meshRenderer.initShaders(device, manager);

    // Create buffers
    {
      D3D11_BUFFER_DESC bufferDesc;

      // Camera buffer
      bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
      bufferDesc.ByteWidth = sizeof(CameraPack);
      bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
      bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
      bufferDesc.MiscFlags = 0;
      bufferDesc.StructureByteStride = 0;
      result = device->CreateBuffer(&bufferDesc, NULL, cameraBuffer.ReleaseAndGetAddressOf());
    }

    return result;
  }

  void Scene::draw(ID3D11DeviceContext* context, bool usePixel)
  {
    meshRenderer.setUploadTransformFunc([=](const XMMATRIX& newTransform) 
    {
      cameraContext->setWorld(newTransform);
      rebuildCameraBuffer(context);
    });

    // Initial camera location
    rebuildCameraBuffer(context);

    context->VSSetConstantBuffers(0, 1, cameraBuffer.GetAddressOf());

    // Render all mesh instances
    meshRenderer.start();
    meshRenderer.bind(context);
    if (!usePixel)
    {
      context->PSSetShader(nullptr, nullptr, 0);
    }
    for (auto instance : meshInstances)
    {
      instance->buildTransform();
      meshRenderer.draw(context, instance);
    }
    meshRenderer.unbind(context);
  }

  void Scene::imguiSceneTree()
  {
    if (ImGui::CollapsingHeader("Scene"))
    {
      for (size_t meshInstanceID = 0; meshInstanceID < meshInstances.size(); ++meshInstanceID)
      {
        auto meshInstance = meshInstances[meshInstanceID];

        std::string instanceName = "Mesh Object " + std::to_string(meshInstanceID);
        ImGui::Text(instanceName.c_str());

        if (ImGui::BeginCombo((instanceName + " Mesh").c_str(), "Mesh type"))
        {
          for (auto& mesh : meshes)
          {
            bool selected = meshInstance->getMesh() == mesh.second;
            if (ImGui::Selectable(mesh.first.c_str(), &selected))
            {
              meshInstance->setMesh(mesh.second);
            }
          }
          ImGui::EndCombo();
        }

        ImGui::DragFloat3((instanceName + " Position").c_str(), &meshInstance->getPosition().x, .05f);
        ImGui::DragFloat3((instanceName + " Scale").c_str(), &meshInstance->getScale().x, .05f);
        
        // Decompose quaternion for better interface
        {
          XMVECTOR quat = XMLoadFloat4(&meshInstance->getRotation());
          XMVECTOR axis;
          float angle;

          XMQuaternionToAxisAngle(&axis, &angle, quat);
          XMFLOAT3 axisFloats;
          XMStoreFloat3(&axisFloats, axis);
          angle = angle * 180.f / M_PI;
          ImGui::DragFloat3((instanceName + " Rot Axis").c_str(), &axisFloats.x);
          ImGui::DragFloat((instanceName + " Rot Angle").c_str(), &angle);
          angle = angle * M_PI / 180.f;
          axis = XMLoadFloat3(&axisFloats);
          axis = XMVector3Normalize(axis);
          if (XMVector3Equal(axis, XMVectorZero()))
          {
            axis = XMVectorSet(1.f, .0f, .0f, 1.f);
          }
          quat = abs(angle) >= FLT_EPSILON ? XMQuaternionRotationAxis(axis, angle) : XMQuaternionIdentity();
          XMStoreFloat4(&meshInstance->getRotation(), quat);
        }

      }

      if (ImGui::Button("Add Mesh Instance"))
      {
        meshInstances.push_back(new MeshInstance<VertexType>(meshes[0].second));
      }
    }
  }

  HRESULT Scene::rebuildCameraBuffer(ID3D11DeviceContext* context)
  {
    HRESULT result = S_OK;

    // Camera data
    {
      D3D11_MAPPED_SUBRESOURCE mapped;
      result = context->Map(cameraBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
      cameraContext->putPack(mapped.pData);
      context->Unmap(cameraBuffer.Get(), 0);
    }

    return result;
  }
}
