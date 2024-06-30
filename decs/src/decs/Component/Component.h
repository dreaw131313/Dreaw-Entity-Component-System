#pragma once

#include "traits.h"

namespace decs
{
	class ComponentBase
	{
		friend class Container;
		template<typename>
		friend class ComponentContext;
		template<typename>
		friend class PackedContainer;
		template<typename>
		friend class StablePackedContainer;

	public:
		ComponentBase() = default;

		ComponentBase(const ComponentBase& other)
		{

		}

		ComponentBase(ComponentBase&& other) noexcept
		{

		}

		ComponentBase& operator =(const ComponentBase& other)
		{
			return *this;
		}

		ComponentBase& operator =(ComponentBase&& other) noexcept
		{
			return *this;
		}

		virtual ~ComponentBase() = default;

		inline bool IsCreatedByECS() const
		{
			return m_bIsCreatedByContainer;
		}

		inline bool IsEnabledByECS() const
		{
			return m_bIsEnabledByECS;
		}

	private:
		bool m_bIsCreatedByContainer = false;
		bool m_bIsEnabledByECS = false;

	private:
		inline void SetFlags(bool bIsCreated, bool bIsEnabled)
		{
			m_bIsCreatedByContainer = bIsCreated;
			m_bIsEnabledByECS = bIsEnabled;
		}
	};

}