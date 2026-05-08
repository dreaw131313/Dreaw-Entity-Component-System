#pragma once

#include "decs/Core/Core.h"
#include "decs/Core/trait.h"
#include "decs/Core/Type.h"


namespace decs::light
{
	class IFilterContainerBase
	{
	public:
		virtual ~IFilterContainerBase() = default;

		inline void IncrementUseCount()
		{
			m_UseCount++;
		}

		inline void DecrementUseCount()
		{
			if (m_UseCount > 0)
			{
				m_UseCount--;
			}
		}

		inline uint32_t GetUseCount() const noexcept
		{
			return m_UseCount;
		}

		virtual TypeID GetDataTypeID() const noexcept = 0;

		virtual size_t GetDataHash() const noexcept = 0;

	private:
		uint32_t m_UseCount = 0;
	};

	template<typename FilterType>
	class FilterContainer
	{
	public:
		const FilterType m_Data{};
		size_t m_DataHash = 0;

	public:
		FilterContainer()
		{
			m_DataHash = std::hash<>{}(m_Data);
		}

		FilterContainer(const FilterType& data):
			m_Data(data)
		{
			m_DataHash = std::hash<>{}(m_Data);
		}

		template<typename...Args>
		FilterContainer(Args&&...args):
			m_Data(std::forward<Args>(args)...)
		{
			m_DataHash = std::hash<>{}(m_Data);
		}

		inline TypeID GetDataTypeID() const noexcept
		{
			return Type<FilterType>::ID();
		}

		inline size_t GetDataHash() const noexcept
		{
			return m_DataHash;
		}

		inline void IncrementUseCount()
		{
			m_UseCount++;
		}

		inline void DecrementUseCount()
		{
			if (m_UseCount > 0)
			{
				m_UseCount--;
			}
		}

		inline uint32_t GetUseCount() const noexcept
		{
			return m_UseCount;
		}

	};
}