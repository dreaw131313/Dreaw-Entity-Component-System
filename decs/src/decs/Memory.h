#pragma once
#include "decs/Core.h"

namespace decs::Memory
{
	inline constexpr uint64_t Align(uint64_t memoryOffset, uint64_t alignment)
	{
		return (memoryOffset + alignment - 1) & ~(alignment - 1);
	}
}