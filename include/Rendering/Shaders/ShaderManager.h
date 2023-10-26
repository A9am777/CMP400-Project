#pragma once
#include "Rendering/D3DCore.h"

#include <string>
#include <vector>

namespace Haboob
{
  class ShaderManager
  {
    public:
    ShaderManager();

    inline const std::vector<D3D11_INPUT_ELEMENT_DESC>& getVertexLayout() const { return globalVertexLayout; }

    void setVertexLayout(const std::vector<D3D11_INPUT_ELEMENT_DESC>& layout);
    void setRootDir(const std::wstring& root);
    void setShaderDir(const std::wstring& shaders);
    HRESULT loadShaderBlob(const wchar_t* relativePath, ID3DBlob** blob) const;

    private:
    static constexpr const wchar_t* COMPILED_EXT = L".cso";
    static constexpr const wchar_t* PATH_DELIM = L"/";

    static const std::vector<D3D11_INPUT_ELEMENT_DESC> defaultVertexLayout;
    std::vector<D3D11_INPUT_ELEMENT_DESC> globalVertexLayout;

    void resolveFullPath();

    std::wstring rootDirectory;
    std::wstring shadersRelativePath;
    std::wstring fullResolvedPath;
  };
}