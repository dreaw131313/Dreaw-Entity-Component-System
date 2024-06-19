#pragma once

#include "traits.h"

namespace decs
{
	class ComponentBase
	{
		friend class Component;
		friend class StableComponent;

	private:
		virtual ~ComponentBase() = default;
	};

	class Component : public ComponentBase
	{
	public:
		inline static constexpr bool IsStable = false;

	};

	class StableComponent : public ComponentBase
	{
	public:
		inline static constexpr bool IsStable = true;

	};

}