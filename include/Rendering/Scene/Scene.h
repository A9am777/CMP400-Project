#pragma once

#include "Rendering/Geometry/GeometryStructs.h"
#include "Rendering/Geometry/MeshRendererImpl.h"
#include "Camera.h"

#include <string>

namespace Haboob
{
  class Scene
  {
    public:
    Scene();
    ~Scene();

    inline void setCameraDefault(Camera* camera) { cameraContext = &defaultCam; }
    inline void setCamera(Camera* camera) { cameraContext = camera; }

    void addMesh(const std::string& name, Mesh<VertexType>* mesh);
    void addObject(MeshInstance<VertexType>* instance);

    HRESULT init(ID3D11Device* device, ShaderManager* manager);
    HRESULT rebuildCameraBuffer(ID3D11DeviceContext* context);
    void draw(ID3D11DeviceContext* context, bool usePixel = true);

    ComPtr<ID3D11Buffer>& getCameraBuffer() { return cameraBuffer; }
    MeshRenderer<VertexType>& getMeshRenderer() { return meshRenderer; }

    void imguiSceneTree();


    private:
    // Reusable buffers
    ComPtr<ID3D11Buffer> cameraBuffer;

    Camera defaultCam;
    Camera* cameraContext = &defaultCam;

    MeshRenderer<VertexType> meshRenderer;
    std::vector<MeshInstance<VertexType>*> meshInstances;
    std::vector<std::pair<std::string, Mesh<VertexType>*>> meshes;
  };

}