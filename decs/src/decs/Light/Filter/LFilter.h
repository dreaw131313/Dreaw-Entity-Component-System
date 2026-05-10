#pragma once

#include "decs/Core/Core.h"
#include "decs/Core/check_cast.h"
#include "decs/Core/trait.h"
#include "decs/Core/Type.h"
#include "decs/Core/RefCounterHandle.h"

namespace decs::light
{
	class IFilterTypeManager;
	class FilterManager;

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

		virtual bool StoresSameData(const IFilterContainerBase& other) const = 0;

		virtual IFilterTypeManager* CreateFilterTypeManager() const = 0;

	private:
		uint32_t m_UseCount = 0;
	};

	template<filter_concept FilterType>
	class FilterContainer : public IFilterContainerBase
	{
	public:
		const FilterType m_Data{};
		size_t m_DataHash = 0;

	public:
		FilterContainer()
		{
			m_DataHash = std::hash<FilterType>{}(m_Data);
		}

		FilterContainer(const FilterType& data):
			m_Data(data)
		{
			m_DataHash = std::hash<FilterType>{}(m_Data);
		}

		template<typename...Args>
		FilterContainer(Args&&...args):
			m_Data(std::forward<Args>(args)...)
		{
			m_DataHash = std::hash<FilterType>{}(m_Data);
		}

		inline TypeID GetDataTypeID() const noexcept
		{
			return Type<FilterType>::ID();
		}

		inline size_t GetDataHash() const noexcept
		{
			return m_DataHash;
		}

		bool StoresSameData(const IFilterContainerBase& other) const override
		{
			if (GetDataTypeID() != other.GetDataTypeID() || GetDataHash() != other.GetDataHash())
			{
				return false;
			}

			const FilterContainer<FilterType>* otherFilterContainer = check_cast<const FilterContainer<FilterType>*>(&other);

			return m_Data == otherFilterContainer->m_Data;
		}

		IFilterTypeManager* CreateFilterTypeManager() const override;

		inline const FilterType& GetAsRef(size_t) const
		{
			return m_Data;
		}

		inline std::span<const FilterType> GetAsSpan() const
		{
			return { &m_Data , 1 };
		}
	};

	template<filter_concept FilterType>
	class FilterEntryKey
	{
	public:
		const FilterType* m_DataPtr = nullptr;

	public:
		FilterEntryKey() = default;

		FilterEntryKey(const FilterType& filterData):
			m_DataPtr(&filterData)
		{

		}

		bool operator ==(const FilterEntryKey& other) const noexcept
		{
			if (m_DataPtr == nullptr || other.m_DataPtr == nullptr)
			{
				return m_DataPtr == other.m_DataPtr;
			}

			return m_DataPtr == other.m_DataPtr || ((*m_DataPtr) == (*other.m_DataPtr));
		}
	};
}

template<typename FilterType>
struct std::hash<decs::light::FilterEntryKey<FilterType>>
{
	size_t operator ()(const decs::light::FilterEntryKey<FilterType>& v) const
	{
		if (v.m_DataPtr != nullptr)
		{
			return std::hash<FilterType>{}(*v.m_DataPtr);
		}
		return 0;
	}
};

namespace decs::light
{
	class IFilterTypeManager
	{
	public:
		virtual ~IFilterTypeManager() = default;

		virtual IFilterContainerBase* CreateMatchingFilterContainer(const IFilterContainerBase& other) = 0;
	};

	template<filter_concept FilterType>
	class FilterTypeManager : public IFilterTypeManager
	{
	public:
		using FilterContainerType = FilterContainer<FilterType>;
		using FilterEntryKeyType = FilterEntryKey<FilterType>;

	public:
		~FilterTypeManager()
		{
			for (auto& [key, value] : m_Filters)
			{
				delete value;
			}
		}

		FilterContainerType* GetOrAddContainer(const FilterType& filter)
		{
			// geting existing filter container
			{
				FilterEntryKeyType tempKey(filter);

				auto it = m_Filters.find(tempKey);
				if (it != m_Filters.end())
				{
					return it->second;
				}
			}

			// create new filter container:
			{
				FilterContainerType* container = new FilterContainerType(filter);
				m_Filters[FilterEntryKeyType(container->m_Data)] = container;

				return container;
			}
		}

		bool RemoveContainer(FilterContainerType* container)
		{
			if (container == nullptr)
			{
				return false;
			}

			FilterEntryKeyType key{ &container->m_Data };

			if (m_Filters.erase(key) == 0)
			{
				return false;
			}

			delete container;

			return true;
		}

		IFilterContainerBase* CreateMatchingFilterContainer(const IFilterContainerBase& other) override
		{
			const FilterContainerType* otherCasted = check_cast<const FilterContainerType*>(&other);

			FilterEntryKeyType tempKey(otherCasted->m_Data);

			auto it = m_Filters.find(tempKey);
			if (it != m_Filters.end())
			{
				return it->second;
			}

			FilterContainerType* newFilterContainer = new FilterContainerType(otherCasted->m_Data);
			m_Filters[FilterEntryKeyType(newFilterContainer->m_Data)] = newFilterContainer;
			return newFilterContainer;
		}

	private:
		std::unordered_map<FilterEntryKeyType, FilterContainerType*> m_Filters{};
	};

	template<filter_concept FilterType>
	IFilterTypeManager* FilterContainer<FilterType>::CreateFilterTypeManager() const
	{
		return new FilterTypeManager<FilterType>();
	}

	class FilterManager
	{
	public:
		~FilterManager()
		{
			for (auto& [key, filterTypeManager] : m_FilterTypes)
			{
				delete filterTypeManager;
			}
		}

	public:
		template<filter_concept FilterType>
		FilterContainer<FilterType>* GetFilter(const FilterType& filter)
		{
			IFilterTypeManager*& filterTypeMangerBase = m_FilterTypes[Type<FilterType>::ID()];
			if (filterTypeMangerBase == nullptr)
			{
				filterTypeMangerBase = new FilterTypeManager<FilterType>();
			}

			FilterTypeManager<FilterType>* filterTypeManager = check_cast<FilterTypeManager<FilterType>*>(filterTypeMangerBase);
			return filterTypeManager->GetOrAddContainer(filter);
		}

		IFilterContainerBase* GetMatchingFilter(const IFilterContainerBase& other)
		{
			const TypeID filterTypeID = other.GetDataTypeID();

			IFilterTypeManager*& filterTypeMangerBase = m_FilterTypes[filterTypeID];
			if (filterTypeMangerBase == nullptr)
			{
				filterTypeMangerBase = other.CreateFilterTypeManager();
			}

			return filterTypeMangerBase->CreateMatchingFilterContainer(other);
		}

		template<filter_concept FilterType>
		bool DeleteFilter(const FilterContainer<FilterType>* filterTypeContainer)
		{
			if (filterTypeContainer == nullptr)
			{
				return false;
			}

			TYPE_ID_CONSTEXPR TypeID typeID = Type<FilterType>::ID();
			auto it = m_FilterTypes.find(typeID);
			if (it == m_FilterTypes.end())
			{
				return false;
			}

			FilterTypeManager<FilterType>* manager = check_cast<FilterTypeManager<FilterType>*>(it->second);
			return manager->RemoveContainer(filterTypeContainer);
		}

	private:
		std::unordered_map<TypeID, IFilterTypeManager*> m_FilterTypes{};
	};

}