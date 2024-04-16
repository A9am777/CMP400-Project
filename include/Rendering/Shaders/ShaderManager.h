#pragma once
#include "Rendering/D3DCore.h"

#include "Data/Defs.h"

#include <string>
#include <vector>
#include <map>
#include <set>

#include "Rendering/Shaders/Shader.h"

namespace Haboob
{
  class ShaderManager
  {
    public:
    ShaderManager();

    inline const std::vector<D3D11_INPUT_ELEMENT_DESC>& getVertexLayout() const { return globalVertexLayout; }

    void bakeMacros(ID3D11Device* device);

    void addShader(Shader* shader);
    void removeShader(Shader* shader);

    void setMacro(const std::string& name, const std::string& value);

    void setVertexLayout(const std::vector<D3D11_INPUT_ELEMENT_DESC>& layout);
    void setRootDir(const std::wstring& root);
    void setShaderDir(const std::wstring& shaders);
    void setLevel(WLiteral level);

    HRESULT loadPreCompiledShaderBlob(const wchar_t* relativePath, ID3DBlob** blob) const;
    HRESULT loadFormattedShaderBlob(Shader::Type shaderType, const wchar_t* relativePath, ID3DBlob** blob) const;

    protected:
    HRESULT recompileShaders(ID3D11Device* device); // Recompiles all listening shaders

    private:
    static constexpr WLiteral COMPILED_EXT = L".cso";
    static constexpr WLiteral VERTEX_ID = L"vs";
    static constexpr WLiteral PIXEL_ID = L"ps";
    static constexpr WLiteral HULL_ID = L"hs";
    static constexpr WLiteral DOMAIN_ID = L"ds";
    static constexpr WLiteral GEOMETRY_ID = L"gs";
    static constexpr WLiteral COMPUTE_ID = L"cs";
    static constexpr WLiteral LIBRARY_ID = L"lib";
    static constexpr WLiteral EXT_DELIM = L".";
    static constexpr WLiteral PATH_DELIM = L"/";
    static constexpr WLiteral UP_LEVEL_DELIM = L"/../";

    static const std::vector<D3D11_INPUT_ELEMENT_DESC> defaultVertexLayout;
    std::vector<D3D11_INPUT_ELEMENT_DESC> globalVertexLayout;

    void resolveFullPath();

    std::wstring rootDirectory;
    std::wstring shadersRelativePath;
    std::wstring fullResolvedPath;
    WLiteral shaderLevel = nullptr;

    std::set<Shader*> shaders;

    std::map<std::wstring, std::string> shaderFileCache; // Cached raw shader files

    std::map<std::string, std::string> macroList;
    std::vector<D3D_SHADER_MACRO> bakedMacros;
    bool isMacrosBaked; // Macro list has been propagated through shaders
  };
}