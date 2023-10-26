#pragma once

// Link D3D Libs
#pragma comment(lib,"d3d11")
#pragma comment(lib,"d3dcompiler")

// Includes
#include "d3d11.h"
#include "d3dcompiler.h"

template<typename R> void DVXClear(R& res)
{
	if (res)
	{
		res->Release();
		res = nullptr;
	}
}

template<typename R> void DVXClear(const R& res)
{
	if (res) { res->Release(); }
}

template<typename F, typename... T> void DVXClear(F& first, T&... rest)
{
	DVXClear(first);
	DVXClear(rest...);
}
