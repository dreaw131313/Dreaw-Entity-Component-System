#pragma once

#include <stdint.h>

namespace decs::hash
{

	inline uint64_t Combine(uint64_t hash1, uint64_t hash2)
	{
		return hash1 ^ (hash2 + 0x9e3779b97f4a7c15ULL + (hash1 << 6) + (hash1 >> 2));
	}

}