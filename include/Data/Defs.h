#pragma once

#ifndef HABOOB_DEFS_H
#define HABOOB_DEFS_H

// Bitwise operations
#define BIT(n) (1 << n)
template<typename T> T BitMask(const T state, const T mask) { return static_cast<T>(state & mask); }
template<typename T> T BitSet(const T state, const T mask) { return static_cast<T>(state | mask); }
template<typename T> T BitClear(const T state, const T mask) { return static_cast<T>(state & ~mask); }

// Unsigned types
typedef unsigned char Byte;
typedef unsigned int UInt;


#endif