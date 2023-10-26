#include "Rendering/DisplayDevice.h"

namespace Haboob
{
  std::vector<D3D_FEATURE_LEVEL> DisplayDevice::defaultFeatureLevels = std::vector<D3D_FEATURE_LEVEL>(
  {
    D3D_FEATURE_LEVEL_11_1,
    D3D_FEATURE_LEVEL_11_0,
    D3D_FEATURE_LEVEL_10_1,
    D3D_FEATURE_LEVEL_10_0,
    D3D_FEATURE_LEVEL_9_3,
    D3D_FEATURE_LEVEL_9_2,
    D3D_FEATURE_LEVEL_9_1,
  });

  DisplayDevice::DisplayDevice()
  {

  }

  HRESULT DisplayDevice::create(UINT flags, const std::vector<D3D_FEATURE_LEVEL>& featureLevelRequest)
  {
    if (device || deviceContext) { return NTE_EXISTS; }
    HRESULT result = S_OK;
    #if Debug
    flags |= D3D11_CREATE_DEVICE_DEBUG;
    #endif

    result = D3D11CreateDevice(
      nullptr,                    // Default adapter
      D3D_DRIVER_TYPE_HARDWARE,   // Hardware accelerated
      0,
      flags,
      featureLevelRequest.data(), // Supported features
      featureLevelRequest.size(),
      D3D11_SDK_VERSION,
      &device,                    // Returns device handle
      &featureLevel,              // Returns supported feature level
      &deviceContext              // Returns device context
    );

    if (FAILED(result))
    {
      device.Reset();
      deviceContext.Reset();
    }

    return result;
  }

  HRESULT DisplayDevice::makeSwapChain(HWND context, DXGI_FORMAT bufferFormat)
  {
    DXGI_SWAP_CHAIN_DESC desc;
    ZeroMemory(&desc, sizeof(DXGI_SWAP_CHAIN_DESC));
    desc.Windowed = TRUE;
    desc.BufferCount = 2;
    desc.BufferDesc.Format = bufferFormat;
    desc.BufferDesc.Width, desc.BufferDesc.Height = 0xFF;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    desc.OutputWindow = context;

    return makeSwapChain(context, desc);
  }

  HRESULT DisplayDevice::makeSwapChain(HWND context, DXGI_SWAP_CHAIN_DESC& desc)
  {
    HRESULT result = S_OK;
    
    ComPtr<IDXGIDevice1> deviceObj;
    ComPtr<IDXGIAdapter> adapter;
    ComPtr<IDXGIFactory> factory;
    
    // Get the DXGI association
    result = device->QueryInterface(deviceObj.GetAddressOf());
    Firebreak(result);

    // Fetch adapter
    result = deviceObj->GetAdapter(&adapter);
    Firebreak(result);

    // Create a swapchain factory
    result = adapter->GetParent(IID_PPV_ARGS(&factory));
    Firebreak(result);

    // Create swapchain
    result = factory->CreateSwapChain(device.Get(), &desc, &swapChain);
    return result;
  }

  HRESULT DisplayDevice::makeBackBuffer()
  {
    HRESULT result = S_OK;

    // Fetch the back buffer from the swap chain
    result = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)backBufferTexture.GetAddressOf());
    Firebreak(result);

    // Set viewport
    {
      D3D11_TEXTURE2D_DESC backDesc;
      backBufferTexture->GetDesc(&backDesc);
      backBufferViewport.Width = backDesc.Width;
      backBufferViewport.Height = backDesc.Height;
      backBufferViewport.MinDepth = .0f;
      backBufferViewport.MaxDepth = 1.f;
    }

    // Create back buffer target
    result = device->CreateRenderTargetView(backBufferTexture.Get(), nullptr, backBufferTarget.GetAddressOf());
    return result;
  }

  HRESULT DisplayDevice::resizeBackBuffer(size_t width, size_t height)
  {
    HRESULT result = S_OK;

    // Finish all pending work
    deviceContext->Flush();

    // Clear targets
    deviceContext->OMSetRenderTargets(0, nullptr, nullptr);

    // Clear back buffer references
    backBufferTexture.Reset();
    backBufferTarget.Reset();

    result = swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    Firebreak(result);

    return makeBackBuffer();
  }

  void DisplayDevice::clearBackBuffer()
  {
    const FLOAT red[4] = { 1.0f, 0.1f, 0.1f, 1.0f };
    deviceContext->ClearRenderTargetView(backBufferTarget.Get(), red);
  }

  void DisplayDevice::setBackBufferTarget()
  {
    deviceContext->RSSetViewports(1, &backBufferViewport);
    deviceContext->OMSetRenderTargets(1, backBufferTarget.GetAddressOf(), nullptr);
  }

  HRESULT DisplayDevice::swapBuffer(UINT flags)
  {
    return swapChain->Present(1, flags);
  }
}