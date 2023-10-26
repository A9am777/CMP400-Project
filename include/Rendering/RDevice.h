#pragma once

//Structures
#include "Canvas.h"

//Framework
#include "D3DCore.h"

namespace DVF
{
	//Rendering device
	class RDevice
	{
		public:
		RDevice();
		~RDevice();

		//Create this device
		HRESULT create(UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT);

		//Make a swap chain for this device
		HRESULT makeSwapChain(HWND context, DXGI_FORMAT bufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM);
		HRESULT makeSwapChain(HWND context, DXGI_SWAP_CHAIN_DESC options);

		HRESULT makeBackBuffer();

		void clearBackBuffer();
		HRESULT resize(size_t width, size_t height);
		void setBackBufferTarget();

		//Swap the back buffer to present the render
		HRESULT swapBuffer(UINT flags = NULL);

		ID3D11Device* getDevice();
		ID3D11DeviceContext* getContext();
		D3D_FEATURE_LEVEL getLevel();

		private:
		static constexpr D3D_FEATURE_LEVEL featureLevels[] = 
		{
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0,
			D3D_FEATURE_LEVEL_9_3,
			D3D_FEATURE_LEVEL_9_2,
			D3D_FEATURE_LEVEL_9_1,
		};

		//Device
		ID3D11Device* dev = nullptr;
		ID3D11DeviceContext* devCon = nullptr;
		D3D_FEATURE_LEVEL supportedL;

		//Strategy
		IDXGISwapChain* swapChain = nullptr;
		Canvas backBuffer;
	};
}