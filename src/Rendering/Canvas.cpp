#include "pch.h"
#include "Rendering/Canvas.h"

using namespace DVF;

Canvas::Canvas()
{
	ZeroMemory(&view, sizeof(D3D11_VIEWPORT));
}

void Canvas::setAsRenderTarget(ID3D11DeviceContext* devCon)
{
	devCon->RSSetViewports(1, &view);
	devCon->OMSetRenderTargets(1, &textureTarget, depthStencilView);
}

void Canvas::setNoRenderTarget(ID3D11DeviceContext* devCon)
{
	devCon->OMSetRenderTargets(0, nullptr, nullptr);
}

void Canvas::clearColour(ID3D11DeviceContext* devCon, const FLOAT colour[4])
{
	devCon->ClearRenderTargetView(textureTarget, colour);
}

void Canvas::updateMeta(bool refreshPort)
{
	//This should be rarely called
	texture->GetDesc(&textureDesc);

	if (refreshPort)
	{
		setPort(textureDesc.Width, textureDesc.Height);
	}
}

void Canvas::setPort(float width, float height, float x, float y)
{
	view.Width = width;
	view.Height = height;
	view.TopLeftX = x;
	view.TopLeftY = y;
}

void Canvas::setDepth(float min, float max)
{
	view.MinDepth = min;
	view.MaxDepth = max;
}
