#pragma once

namespace decs
{
	class Component
	{
	public:
		inline static constexpr bool IsStable = false;

	};

	class StableComponent
	{
	public:
		inline static constexpr bool IsStable = true;

	};

}