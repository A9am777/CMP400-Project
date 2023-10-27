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

  DisplayDevice::DisplayDevice() : featureLevel{D3D_FEATURE_LEVEL_1_0_CORE}, backBufferViewport{}
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
    desc.BufferDesc.Width = desc.BufferDesc.Height = 0xFF;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD; // ImGui forced
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

      ZeroMemory(&backBufferViewport, sizeof(D3D11_VIEWPORT));

      backBufferViewport.Width = backDesc.Width;
      backBufferViewport.Height = backDesc.Height;
      backBufferViewport.MinDepth = .0f;
      backBufferViewport.MaxDepth = 1.f;
    }

    // Create back buffer target
    result = device->CreateRenderTargetView(backBufferTexture.Get(), nullptr, backBufferTarget.GetAddressOf());
    Firebreak(result);

    result = makeDepthBuffer();

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
    deviceContext->ClearDepthStencilView(depthBufferView.Get(), D3D11_CLEAR_DEPTH, 1.f, 0);
  }

  void DisplayDevice::setBackBufferTarget()
  {
    deviceContext->RSSetViewports(1, &backBufferViewport);
    deviceContext->OMSetRenderTargets(1, backBufferTarget.GetAddressOf(), depthBufferView.Get());
  }

  void DisplayDevice::setDepthEnabled(bool useDepth)
  {
    deviceContext->OMSetDepthStencilState(useDepth ? depthEnabledState.Get() : depthDisabledState.Get(), 1);
  }

  HRESULT DisplayDevice::swapBuffer(UINT flags)
  {
    return swapChain->Present(1, flags);
  }

  HRESULT DisplayDevice::makeDepthBuffer()
  {
    HRESULT result = S_OK;

    // Depth stencil texture
    {
      D3D11_TEXTURE2D_DESC depthBufferDesc;
      ZeroMemory(&depthBufferDesc, sizeof(D3D11_TEXTURE2D_DESC));

      depthBufferDesc.Width = backBufferViewport.Width;
      depthBufferDesc.Height = backBufferViewport.Height;
      depthBufferDesc.MipLevels = 1;
      depthBufferDesc.ArraySize = 1;
      depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
      depthBufferDesc.SampleDesc.Count = 1;
      depthBufferDesc.SampleDesc.Quality = 0;
      depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
      depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
      depthBufferDesc.CPUAccessFlags = 0;
      depthBufferDesc.MiscFlags = 0;

      result = device->CreateTexture2D(&depthBufferDesc, NULL, &depthBufferTexture);
    }
    Firebreak(result);

    // Depth stencil view
    {
      D3D11_DEPTH_STENCIL_VIEW_DESC depthViewDesc;
      ZeroMemory(&depthViewDesc, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
      
      depthViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
      depthViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
      depthViewDesc.Texture2D.MipSlice = 0;

      result = device->CreateDepthStencilView(depthBufferTexture.Get(), &depthViewDesc, &depthBufferView);
    }

    return result;
  }

  HRESULT DisplayDevice::makeDepthStates()
  {
    HRESULT result = S_OK;

    D3D11_DEPTH_STENCIL_DESC depthStateDesc;
    ZeroMemory(&depthStateDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));

    // Enabled state
    {
      depthStateDesc.DepthEnable = true;
      depthStateDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
      depthStateDesc.DepthFunc = D3D11_COMPARISON_LESS;
      depthStateDesc.StencilEnable = true;
      depthStateDesc.StencilReadMask = 0xFF;
      depthStateDesc.StencilWriteMask = 0xFF;
      depthStateDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
      depthStateDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
      depthStateDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
      depthStateDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
      depthStateDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
      depthStateDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
      depthStateDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
      depthStateDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

      result = device->CreateDepthStencilState(&depthStateDesc, &depthEnabledState);
    }
    Firebreak(result);

    // Disabled state
    {
      depthStateDesc.DepthEnable = false;
      result = device->CreateDepthStencilState(&depthStateDesc, &depthDisabledState);
    }

    return result;
  }
}