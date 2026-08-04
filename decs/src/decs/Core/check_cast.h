#pragma once
#include "Core.h"

namespace decs
{

	template<typename To, typename From>
		requires std::is_pointer_v<To>&& std::is_pointer_v<From>
	inline To check_cast(From v)
	{
		static_assert(!std::is_same<To, From>::value, "Redundant check_cast");

	#if defined(DECS_DEBUG)
		if constexpr (std::is_polymorphic_v<To> && std::is_polymorphic_v<From>)
		{
			if (!v)
			{
				return nullptr;
			}

			auto result = dynamic_cast<To>(v);

			DECS_ASSERT(result != nullptr, "Failed to cast!");

			return result;
		}
	#endif

		return static_cast<To>(v);

	}
}