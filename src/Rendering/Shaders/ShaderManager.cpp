#include "Rendering/Shaders/ShaderManager.h"

namespace Haboob
{
  const std::vector<D3D11_INPUT_ELEMENT_DESC> ShaderManager::defaultVertexLayout = {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
  };

  ShaderManager::ShaderManager()
  {
    globalVertexLayout = defaultVertexLayout;
  }

  void ShaderManager::setVertexLayout(const std::vector<D3D11_INPUT_ELEMENT_DESC>& layout)
  {
    globalVertexLayout = layout;
  }

  void ShaderManager::setRootDir(const std::wstring& root)
  {
    rootDirectory = root;
    resolveFullPath();
  }

  void ShaderManager::setShaderDir(const std::wstring& shaders)
  {
    shadersRelativePath = shaders;
    resolveFullPath();
  }

  HRESULT ShaderManager::loadShaderBlob(const wchar_t* relativePath, ID3DBlob** blob) const
  {
    std::wstring fullFile = fullResolvedPath + relativePath + COMPILED_EXT;
    return D3DReadFileToBlob(fullFile.data(), blob);
  }

  void ShaderManager::resolveFullPath()
  {
    fullResolvedPath = rootDirectory + PATH_DELIM + shadersRelativePath + PATH_DELIM;
  }
}
