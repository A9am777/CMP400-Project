#include "Rendering/Shaders/Shader.h"
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
    
    shaderLevel = L"_5_0";
    isMacrosBaked = false;
    bakeMacros(nullptr);
  }

  void ShaderManager::bakeMacros(ID3D11Device* device)
  {
    if (isMacrosBaked) { return; } // Skip if the same

    // Produce macro list
    bakedMacros.clear();
    for (auto& it : macroList)
    {
      bakedMacros.push_back({ it.first.c_str(), it.second.c_str() });
    }
    bakedMacros.push_back({ nullptr, nullptr }); // Terminator

    if (device)
    {
      recompileShaders(device);
    }

    isMacrosBaked = true;
  }

  void ShaderManager::addShader(Shader* shader)
  {
    if (shaders.insert(shader).second)
    {
      isMacrosBaked = false; // Hack
    }
  }

  void ShaderManager::removeShader(Shader* shader)
  {
    shaders.erase(shader);
  }

  void ShaderManager::setMacro(const std::string& name, const std::string& value)
  {
    auto macroIt = macroList.find(name);
    if (macroIt == macroList.end() || macroIt->second != value)
    {
      macroList.insert_or_assign(name, value);
      isMacrosBaked = false;
    }
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

  void ShaderManager::setLevel(WLiteral level)
  {
    shaderLevel = level;
  }

  HRESULT ShaderManager::loadFormattedShaderBlob(Shader::Type shaderType, const wchar_t* relativePath, ID3DBlob** blob) const
  {
    // Manages includes
    class SubShaderProcessor : public ID3DInclude
    {
      public:
      std::map<std::wstring, std::string>* fileCache;
      std::wstring parentPath; // Root
      std::vector<std::pair<LPCVOID, std::wstring>> treeDirectories; // Additional recursive directories to check

      SubShaderProcessor(const std::wstring& basePath, std::map<std::wstring, std::string>* cache)
      {
        parentPath = basePath;
        fileCache = (std::map<std::wstring, std::string>*)cache;
      }

      HRESULT Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes) final
      {
        std::string fileName(pFileName);
        std::wstring wfileName(fileName.begin(), fileName.end());
        std::wstring fullFile = parentPath + wfileName;

        #define ReadFileToBlob(filePath, blob)\
        {\
          auto cacheIt = fileCache->find(filePath);\
          if (cacheIt != fileCache->end())\
          {\
            D3DCreateBlob(cacheIt->second.size(), blob.GetAddressOf());\
            std::memcpy((void*)fileBlob->GetBufferPointer(), cacheIt->second.data(), cacheIt->second.size());\
          }\
          else\
          {\
            D3DReadFileToBlob(fullFile.c_str(), fileBlob.GetAddressOf());\
            if (fileBlob.Get())\
            {\
              std::string shaderFile((char*)fileBlob->GetBufferPointer(), fileBlob->GetBufferSize());\
              fileCache->insert({ filePath, shaderFile });\
            }\
          }\
        }

        ComPtr<ID3DBlob> fileBlob;
        HRESULT result = S_OK;
        ReadFileToBlob(fullFile, fileBlob);
        
        for (auto& treeDirsIt = treeDirectories.rbegin(); (result & (HRESULT)ERROR_FILE_NOT_FOUND) && treeDirsIt != treeDirectories.rend(); ++treeDirsIt)
        {
          fullFile = treeDirsIt->second + wfileName;
          ReadFileToBlob(fullFile, fileBlob);
        }

        #undef ReadFileToBlob
        
        if (fileBlob.Get())
        {
          *pBytes = fileBlob->GetBufferSize();
          *ppData = new Byte[*pBytes];
          std::memcpy((void*)*ppData, fileBlob->GetBufferPointer(), *pBytes);
        }
        else
        {
          *ppData = nullptr;
        }

        if (SUCCEEDED(result))
        {
          treeDirectories.push_back({ *ppData, fullFile + UP_LEVEL_DELIM });
        }

        return result;
      }

      HRESULT Close(LPCVOID pData) final
      {
        if (pData)
        {
          treeDirectories.pop_back();
          delete[](void*)pData;
        }

        return S_OK;
      }
    };
    
    // Determine file path and extension
    std::wstring fullFile = fullResolvedPath + relativePath;
    std::wstring basePath = fullFile + UP_LEVEL_DELIM;

    WLiteral typeID = VERTEX_ID;
    switch (shaderType)
    {
      case Shader::Type::Vertex:
        typeID = VERTEX_ID;
      break;
      case Shader::Type::Pixel:
        typeID = PIXEL_ID;
      break;
      case Shader::Type::Hull:
        typeID = HULL_ID;
      break;
      case Shader::Type::Domain:
        typeID = DOMAIN_ID;
      break;
      case Shader::Type::Geometry:
        typeID = GEOMETRY_ID;
      break;
      case Shader::Type::Compute:
        typeID = COMPUTE_ID;
      break;
    }
    fullFile = fullFile + EXT_DELIM + typeID;

    // Shader flags
    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
    #if DEBUGFLAG
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_DEBUG_NAME_FOR_SOURCE;
    #endif

    // Shader profile
    std::wstring wprofile = std::wstring(typeID) + shaderLevel;
    std::string profile = std::string(wprofile.begin(), wprofile.end());

    // Compile shader
    ComPtr<ID3DBlob> errorBlob;
    SubShaderProcessor includer(basePath, const_cast<std::map<std::wstring, std::string>*>(&shaderFileCache));
    HRESULT result = D3DCompileFromFile(fullFile.data(), bakedMacros.data(), &includer, "main", profile.c_str(), flags, NULL, blob, errorBlob.GetAddressOf());

    if (FAILED(result) && errorBlob.Get())
    {
      std::string error;

      error = "Shader (" + profile + ") path [" + std::string(fullFile.begin(), fullFile.end()) + "] says \n";
      error = error + std::string((char*)errorBlob.Get()->GetBufferPointer(), errorBlob.Get()->GetBufferSize());

      // Print error
      MessageBox(NULL, error.c_str(), "Shader Compile Error", MB_ICONERROR | MB_OK);
    }

    return result;
  }

  HRESULT ShaderManager::loadPreCompiledShaderBlob(const wchar_t* relativePath, ID3DBlob** blob) const
  {
    std::wstring fullFile = fullResolvedPath + relativePath + COMPILED_EXT;
    return D3DReadFileToBlob(fullFile.data(), blob);
  }

  HRESULT ShaderManager::recompileShaders(ID3D11Device* device)
  {
    HRESULT result = S_OK;
    
    for (auto shader : shaders)
    {
      result = shader->initShader(device, this);
      Firebreak(result);
    }

    return result;
  }

  void ShaderManager::resolveFullPath()
  {
    fullResolvedPath = rootDirectory + PATH_DELIM + shadersRelativePath + PATH_DELIM;
  }
}
