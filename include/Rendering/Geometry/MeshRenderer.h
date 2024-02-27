#pragma once

#include "Rendering/Shaders/ShaderManager.h"
#include "Mesh.h"

#include <functional>

namespace Haboob
{
  template<typename VertexT> class MeshRenderer;

  template<typename VertexT> class MeshInstance
  {
    friend class MeshRenderer<VertexT>;

    public:
    MeshInstance(Mesh<VertexT>* mesh = nullptr);

    inline void setMesh(Mesh<VertexT>* newMesh) { baseMesh = newMesh; }
    void buildTransform();

    const inline XMMATRIX& getTransform() { return transform; }

    inline XMFLOAT3& getPosition() { return position; }
    inline XMFLOAT3& getScale() { return scale; }
    inline XMFLOAT4& getRotation() { return quat; }

    inline const Mesh<VertexT>* getMesh() const { return baseMesh; }

    protected:
    Mesh<VertexT>* baseMesh;
    XMMATRIX transform;
    XMFLOAT3 position;
    XMFLOAT3 scale;
    XMFLOAT4 quat;
  };

  template<typename VertexT> class MeshRenderer
  {
    public:
    MeshRenderer();
    ~MeshRenderer();

    inline void setUploadTransformFunc(const std::function<void(const XMMATRIX&)>& func) { uploadTransformFunc = func; }

    void initShaders(ID3D11Device* device, ShaderManager* manager);

    void start();
    void bind(ID3D11DeviceContext* context);
    void draw(ID3D11DeviceContext* context, MeshInstance<VertexT>* instance);
    void unbind(ID3D11DeviceContext* context);

    private:
    std::function<void(const XMMATRIX&)> uploadTransformFunc; // Used to upload mesh transform

    bool useDeferredContext;
    Shader* deferredVertexShader;
    Shader* deferredPixelShader;

    Mesh<VertexT>* cachedMesh; // Last drawn mesh context
  };
}