#pragma once

#include "traits.h"

namespace decs
{
	/// <summary>
	/// Component must have one public static fiel:
	///		- inline static constexpr bool IsStable = true/false;
	/// If it is true component pointer will never change its position in memory, if false component will be moved in memory.
	/// </summary>
	class ComponentBase
	{
		friend class Container;
		template<typename>
		friend class ComponentContext;

	public:
		virtual ~ComponentBase() = default;

	private:
		bool m_bIsCreatedByContainer = false;
		bool m_bIsEnabledByECS = false;
	};

}