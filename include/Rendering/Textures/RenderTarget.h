#pragma once
#include "Rendering/D3DCore.h"
#include "Data/Defs.h"
#include "Rendering/Shaders/RenderCopyShader.h"

namespace Haboob
{
  class RenderTarget
  {
    public:
    RenderTarget();

    void setTarget(ID3D11DeviceContext* context, ID3D11DepthStencilView* depthStencil = nullptr);
    void clear(ID3D11DeviceContext* context, float colour[4] = defaultRed);

    HRESULT create(ID3D11Device* device, UInt width, UInt height, const D3D11_TEXTURE2D_DESC& textureDesc = defaultTextureDesc);
    HRESULT resize(ID3D11Device* device, UInt width, UInt height);

    // Renders from this target to the currently bound target
    void renderFrom(ID3D11DeviceContext* context);

    inline const ID3D11RenderTargetView* getRenderTarget() const { return textureTarget.Get(); }
    inline const ID3D11ShaderResourceView* getShaderView() const { return textureShaderView.Get(); }

    static RenderCopyShader copyShader;
    protected:
    static D3D11_TEXTURE2D_DESC defaultTextureDesc;
    static float defaultRed[4];

    private:
    ComPtr<ID3D11Texture2D> texture;
    ComPtr<ID3D11RenderTargetView> textureTarget;
    ComPtr<ID3D11ShaderResourceView> textureShaderView;
    D3D11_VIEWPORT textureViewport;
  };
}