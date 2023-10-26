#include "pch.h"
#include "Rendering/Texture2DResource.h"

using namespace DVF;

Texture2DResource::Texture2DResource()
{
	flags = NULL;
	texture = nullptr;
	textureTarget = nullptr;
	textureView = nullptr;
}

Texture2DResource::Texture2DResource(Texture2DResource&& parent)
{
	texture = parent.texture;
	textureTarget = parent.textureTarget;
	textureView = parent.textureView;
	flags = parent.flags;

	//Remove ownership
	parent.flags &= ~(OWNERSHIP_BASE | OWNERSHIP_TARGET | OWNERSHIP_VIEW);
}

Texture2DResource::~Texture2DResource()
{
	release();
}

ID3D11Texture2D* DVF::Texture2DResource::getTexture()
{
	return texture;
}

ID3D11RenderTargetView* DVF::Texture2DResource::getTarget()
{
	return textureTarget;
}

ID3D11ShaderResourceView* DVF::Texture2DResource::getView()
{
	return textureView;
}

std::wstring DVF::Texture2DResource::getName()
{
	return name;
}

void DVF::Texture2DResource::borrowTexture(ID3D11Texture2D* loanedTexture)
{
	terminateTexture();

	texture = loanedTexture;
	flags &= ~OWNERSHIP_BASE;
}

void DVF::Texture2DResource::borrowTarget(ID3D11RenderTargetView* loanedTarget)
{
	terminateTextureTarget();

	textureTarget = loanedTarget;
	flags &= ~OWNERSHIP_TARGET;
}

void DVF::Texture2DResource::holdTexture(ID3D11Texture2D* providedTexture)
{
	terminateTexture();

	texture = providedTexture;
	flags |= OWNERSHIP_BASE;
}

void DVF::Texture2DResource::holdTarget(ID3D11RenderTargetView* providedTarget)
{
	terminateTextureTarget();

	textureTarget = providedTarget;
	flags |= OWNERSHIP_TARGET;
}

void DVF::Texture2DResource::release()
{
	terminateTexture();
	terminateTextureTarget();
	terminateTextureView();
}

void Texture2DResource::terminateTexture()
{
	if (OWNERSHIP_BASE & flags)
	{
		DVXClear(texture);
	}
}

void Texture2DResource::terminateTextureTarget()
{
	if (OWNERSHIP_TARGET & flags)
	{
		DVXClear(textureTarget);
	}
}

void Texture2DResource::terminateTextureView()
{
	if (OWNERSHIP_VIEW & flags)
	{
		DVXClear(textureView);
	}
}
