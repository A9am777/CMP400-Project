#pragma once

#ifndef HABOOB_DEFS_H
#define HABOOB_DEFS_H

// Bitwise operations
#define BIT(n) (1 << n)
template<typename T> T BitMask(const T state, const T mask) { return static_cast<T>(state & mask); }
template<typename T> T BitSet(const T state, const T mask) { return static_cast<T>(state | mask); }
template<typename T> T BitClear(const T state, const T mask) { return static_cast<T>(state & ~mask); }
template<typename T> T BitCopy(const T state, const T mask, const T value) { return BitSet(BitClear(state, mask), BitMask(value, mask)); }

// Unsigned types
typedef unsigned char Byte;
typedef unsigned int UInt;
typedef unsigned long ULong;

// C-style strings
typedef const char* CString;
typedef const wchar_t* WCString;
typedef const char* CStringC;
typedef const wchar_t* WCStringC;
typedef const char* Literal; // Program space
typedef const wchar_t* WLiteral;

#endif