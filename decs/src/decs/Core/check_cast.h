#pragma once
#include "Core.h"

namespace decs
{

	template<typename To, typename From>
	inline To check_cast(From v)
	{
		static_assert(!std::is_same<To, From>::value, "Redundant check_cast");

	#if defined(DECS_DEBUG)

		if (!v)
		{
			return nullptr;
		}

		auto result = dynamic_cast<To>(v);

		DECS_ASSERT(result != nullptr, "Failed to cast!");

		return result;
	#else 

		return static_cast<To>(v);
	#endif

	}
}