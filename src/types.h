#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include <cstdint>
#include <cstring>
#include <bitset>
#include <memory>
#include <vector>
#include <functional>
#include <map>

typedef uint8_t byte;
typedef uint16_t word;
typedef std::shared_ptr<byte> pbyte;
typedef std::vector<byte> vbyte;

using std::make_shared;
using std::vector;
using std::function;
using std::map;
using std::bind;

#define kB(x) 0x400*x
#define packWord(high,low) ((word)(high << 8) | low)

// lower n bits and upper n bits of v
template <typename T> T lowern(T v, byte n) {
	return v & ((1 << n) - 1);
}

template <typename T> T uppern(T v, byte n) {
	return lowern((v >> (sizeof(v) * 8 - n)), n);
}

template <typename T> word wbits(T b) {
	return ((word)std::bitset<16>(b).to_ulong());
}