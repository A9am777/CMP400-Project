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
    void draw(ID3D11DeviceContext* context);

    ComPtr<ID3D11Buffer>& getCameraBuffer() { return cameraBuffer; }

    void imguiSceneTree();

    protected:
    HRESULT rebuildCameraBuffer(ID3D11DeviceContext* context);

    private:
    // Reusable buffers
    ComPtr<ID3D11Buffer> cameraBuffer;
    ComPtr<ID3D11Buffer> lightBuffer;

    Camera defaultCam;
    Camera* cameraContext = &defaultCam;

    MeshRenderer<VertexType> meshRenderer;
    std::vector<MeshInstance<VertexType>*> meshInstances;
    std::vector<std::pair<std::string, Mesh<VertexType>*>> meshes;
  };

}