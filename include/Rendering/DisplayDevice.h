#pragma once

#include "D3DCore.h"
#include <vector>

namespace Haboob
{
  class DisplayDevice
  {
    public:
    DisplayDevice();

    HRESULT create(UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT, const std::vector<D3D_FEATURE_LEVEL>& featureLevelRequest = DisplayDevice::defaultFeatureLevels);
    HRESULT makeSwapChain(HWND context, DXGI_FORMAT bufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM);
    HRESULT makeSwapChain(HWND context, DXGI_SWAP_CHAIN_DESC& desc);

    HRESULT makeBackBuffer();
    HRESULT resizeBackBuffer(size_t width, size_t height);
    void clearBackBuffer();
    void setBackBufferTarget();

    // Swaps the back buffer to screen
    HRESULT swapBuffer(UINT flags = NULL);

    inline ComPtr<ID3D11Device> getDevice() { return device; }
    inline ComPtr<ID3D11DeviceContext> getContext() { return deviceContext; }
    inline D3D_FEATURE_LEVEL getLevel() const { return featureLevel; }

    private:
    static std::vector<D3D_FEATURE_LEVEL> defaultFeatureLevels;

    // Device
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> deviceContext;
    D3D_FEATURE_LEVEL featureLevel;

    // Backbuffer
    ComPtr<IDXGISwapChain> swapChain;
    ComPtr<ID3D11Texture2D> backBufferTexture;
    ComPtr<ID3D11RenderTargetView> backBufferTarget;
    D3D11_VIEWPORT backBufferViewport;
  };
}