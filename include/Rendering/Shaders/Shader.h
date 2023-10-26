#pragma once
#include "Rendering/D3DCore.h"
#include "Rendering/DisplayDevice.h"
#include "ShaderManager.h"

namespace Haboob
{
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

    Shader(Type shaderType, const wchar_t* path);
    ~Shader();

    HRESULT initShader(DisplayDevice* device, const ShaderManager* manager);
    void bindShader(ID3D11DeviceContext* context);

    protected:
    HRESULT makeShader(ID3D11DeviceChild** shader, DisplayDevice* device, const ShaderManager* manager);
    void bindVertex(ID3D11DeviceContext* context);
    void bindPixel(ID3D11DeviceContext* context);
    void bindHull(ID3D11DeviceContext* context);
    void bindDomain(ID3D11DeviceContext* context);
    void bindGeometry(ID3D11DeviceContext* context);
    void bindCompute(ID3D11DeviceContext* context);

    private:
    Type type;
    const wchar_t* relativePath;
    ComPtr<ID3D11InputLayout> inputLayout; // Vertex descriptor
    ID3D11DeviceChild* compiledShader;
  };
}