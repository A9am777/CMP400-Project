#pragma once
#include "Rendering/D3DCore.h"
#include "Rendering/DisplayDevice.h"

namespace Haboob
{
  class ShaderManager;

  class Shader
  {
    public:
    enum class Type
    {
      Vertex = 0,
      Pixel,
      Hull,
      Domain,
      Geometry,
      Compute,
      COUNT
    };

    Shader(Type shaderType, const wchar_t* path, bool precompiled = false);
    ~Shader();

    HRESULT initShader(ID3D11Device* device, const ShaderManager* manager);
    HRESULT initShader(ID3D11Device* device, ShaderManager* manager);
    void bindShader(ID3D11DeviceContext* context);
    void unbindShader(ID3D11DeviceContext* context);
    static void dispatch(ID3D11DeviceContext* context, UInt groupX = 1, UInt groupY = 1, UInt groupZ = 1);

    inline Type getType() const { return type; }
    inline bool isCompiled() const { return compiledShader != nullptr; }

    protected:
    HRESULT makeShader(ID3D11DeviceChild** shader, ID3D11Device* device, const ShaderManager* manager);
    void bindVertex(ID3D11DeviceContext* context);
    static void unbindVertex(ID3D11DeviceContext* context);
    void bindPixel(ID3D11DeviceContext* context);
    static void unbindPixel(ID3D11DeviceContext* context);
    void bindHull(ID3D11DeviceContext* context);
    static void unbindHull(ID3D11DeviceContext* context);
    void bindDomain(ID3D11DeviceContext* context);
    static void unbindDomain(ID3D11DeviceContext* context);
    void bindGeometry(ID3D11DeviceContext* context);
    static void unbindGeometry(ID3D11DeviceContext* context);
    void bindCompute(ID3D11DeviceContext* context);
    static void unbindCompute(ID3D11DeviceContext* context);

    private:
    Type type;
    const wchar_t* relativePath;
    bool preferCompiled;
    ComPtr<ID3D11InputLayout> inputLayout; // Vertex descriptor
    ID3D11DeviceChild* compiledShader;
  };
}