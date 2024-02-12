#pragma once

#include "D3DCore.h"
#include "Data/Defs.h"
#include <vector>

namespace Haboob
{
  class DisplayDevice
  {
    public:
    enum RasterFlags : Byte
    {
      RASTER_FLAG_SOLID = BIT(0), // Solid otherwise wireframe
      RASTER_FLAG_CULL = BIT(1), // Culling enabled otherwise disabled
      RASTER_FLAG_BACK = BIT(2), // Back face cull otherwise front
      RASTER_STATE_COUNT = BIT(3),  // State permutation count

      RASTER_STATE_DEFAULT = RASTER_FLAG_SOLID | RASTER_FLAG_CULL | RASTER_FLAG_BACK
    };

    DisplayDevice();

    HRESULT create(UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT, const std::vector<D3D_FEATURE_LEVEL>& featureLevelRequest = DisplayDevice::defaultFeatureLevels);
    HRESULT makeSwapChain(HWND context, DXGI_FORMAT bufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM);
    HRESULT makeSwapChain(HWND context, DXGI_SWAP_CHAIN_DESC& desc);

    HRESULT makeStates();
    HRESULT makeBackBuffer();
    HRESULT resizeBackBuffer(size_t width, size_t height);
    void clearBackBuffer();
    void setBackBufferTarget();

    // States
    void setDepthEnabled(bool useDepth);
    void setWireframe(bool isWireframe);
    void setCull(bool isCull);
    void setCullBackface(bool isCullBack);
    void setRasterState(RasterFlags newState = RASTER_STATE_DEFAULT, bool force = false);
    inline RasterFlags getRasterState() const { return rasterState; }

    // Swaps the back buffer to screen
    HRESULT swapBuffer(UINT flags = NULL);

    // Device
    inline ComPtr<ID3D11Device> getDevice() { return device; }
    inline ComPtr<ID3D11DeviceContext> getContext() { return deviceContext; }
    inline D3D_FEATURE_LEVEL getLevel() const { return featureLevel; }

    // Buffers
    inline ID3D11DepthStencilView* getDepthBuffer() { return depthBufferView.Get(); }

    inline const XMMATRIX& getOrthoMatrix() const { return orthoMatrix; }

    private:
    static std::vector<D3D_FEATURE_LEVEL> defaultFeatureLevels;

    HRESULT makeDepthBuffer();

    // Device
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> deviceContext;
    D3D_FEATURE_LEVEL featureLevel;

    // Backbuffer
    ComPtr<IDXGISwapChain> swapChain;
    ComPtr<ID3D11Texture2D> backBufferTexture;
    ComPtr<ID3D11RenderTargetView> backBufferTarget;

    D3D11_VIEWPORT backBufferViewport;
    XMMATRIX orthoMatrix;
    
    ComPtr<ID3D11Texture2D> depthBufferTexture;
    ComPtr<ID3D11DepthStencilView> depthBufferView;

    // States
    ComPtr<ID3D11DepthStencilState> depthEnabledState;
    ComPtr<ID3D11DepthStencilState> depthDisabledState;

    ComPtr<ID3D11RasterizerState> rasterStates[RASTER_STATE_COUNT];
    RasterFlags rasterState;
  };
}