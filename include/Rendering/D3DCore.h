#pragma once

#ifndef D3DCORE_H
#define D3DCORE_H

// Link D3D Libs
#pragma comment(lib,"d3d11")
#pragma comment(lib,"d3dcompiler")

// Lib Includes
#include "d3d11.h"
#include "d3dcompiler.h"

// Smart ptr
#include <wrl.h>
using Microsoft::WRL::ComPtr;

#define Debug (DEBUG || _DEBUG)
#define Firebreak(result) if(FAILED(result)) { return result; }
#define FirebreakClear(result, clear) if(FAILED(result)) { clear->Release(); return result; }

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

#endif