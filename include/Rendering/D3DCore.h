#pragma once

#ifndef D3DCORE_H
#define D3DCORE_H

// Link D3D Libs
#pragma comment(lib,"d3d11")
#pragma comment(lib,"d3dcompiler")

// Lib Includes
#include <d3d11.h>
#include <d3dcompiler.h>

// Smart ptr
#include <wrl.h>
using Microsoft::WRL::ComPtr;

// Maths/data
#include <directxmath.h>
using namespace DirectX;

// Useful utils
#define DEBUGFLAG (DEBUG || _DEBUG)
#define Firebreak(result) if(FAILED(result)) { return result; }
#define FirebreakClear(result, clear) if(FAILED(result)) { clear->Release(); return result; }

#endif