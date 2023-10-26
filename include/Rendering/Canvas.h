#pragma once

//Structures
#include "Texture2DResource.h"

//Framework
#include "D3DCore.h"

namespace DVF
{
	//Representation of a texture with rendering capabilities
	class Canvas : public Texture2DResource
	{
		public:
		Canvas();

		void setAsRenderTarget(ID3D11DeviceContext* devCon);
		static void setNoRenderTarget(ID3D11DeviceContext* devCon);

		void clearColour(ID3D11DeviceContext* devCon, const FLOAT colour[4] = defaultRed);

		//Use provided texture to fetch metadata
		void updateMeta(bool refreshPort = true);

		void setPort(float width, float height, float x = 0.0f, float y = 0.0f);
		void setDepth(float min, float max);

		protected:
		//Depth stencil works for 2d and 3d
		ID3D11Texture2D* depthStencil = nullptr;
		ID3D11DepthStencilView* depthStencilView = nullptr;

		//Metadata
		D3D11_TEXTURE2D_DESC textureDesc; //For automatic resizing and other dynamic adjustments
		D3D11_VIEWPORT view; //Porthole onto the texture

		private:
		static constexpr FLOAT defaultRed[4] = { 1.0f, 0.1f, 0.1f, 1.0f };
	};
}