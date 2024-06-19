#pragma once

#include "traits.h"

#define DECS_COMPONENT() public: inline constexpr static bool IsStable = false;
#define DECS_STABLE_COMPONENT() public: inline constexpr static bool IsStable = true;

namespace decs
{
	/// <summary>
	/// Component must have one public static fiel:
	///		- inline static constexpr bool IsStable = true/false;
	/// If it is true component pointer will never change its position in memory, if false component will be moved in memory.
	/// </summary>
	class ComponentBase
	{
		friend class Component;
		friend class StableComponent;

	public:
		virtual ~ComponentBase() = default;
	};

}