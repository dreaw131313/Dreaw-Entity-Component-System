#pragma once
#include "decspch.h"
#include "Core.h"

namespace decs
{
	template<typename T>
	class Type final
	{
	public:
		static constexpr TypeID ID()
		{
			constexpr auto id = c_string_hash(FULL_FUNCTION_NAME);
			return id;
		}
	};

}