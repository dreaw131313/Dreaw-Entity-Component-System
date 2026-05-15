#pragma once

#include "decs/Core/Core.h"
#include "decs/Core/check_cast.h"
#include "decs/Core/trait.h"
#include "decs/Core/Type.h"
#include "decs/Core/RefCounterHandle.h"
#include "decs/Core/TChunkedVector.h"

#include <iostream>

namespace decs::light
{
	class IFilterTypeManager;
	class FilterManager;

	class IFilterContainerBase
	{
	public:
		virtual ~IFilterContainerBase() = default;

		inline void IncrementRefCount()
		{
			m_UseCount++;
		}

		inline void DecrementRefCount()
		{
			if (m_UseCount > 0)
			{
				m_UseCount--;
			}
		}

		inline uint32_t GetRefCount() const noexcept
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
		FilterType m_Data{};
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

		inline const FilterType& GetFilterData() const
		{
			return m_Data;
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

		inline const FilterType* GetAsPtr(size_t) const
		{
			return &m_Data;
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

		virtual void Clear() = 0;

		virtual bool RemoveContainer(IFilterContainerBase* container) = 0;
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
			for (size_t i = 0; i < m_Allocator.Size(); i++)
			{
				std::cout << m_Allocator[i].GetRefCount() << "\n";
			}
		}

		void Clear() override
		{
			m_Allocator.Clear();
			m_FreeList.clear();
			m_FiltersMap.clear();
		}

		FilterContainerType* GetOrAddContainer(const FilterType& filter)
		{
			FilterEntryKeyType tempKey(filter);

			auto it = m_FiltersMap.find(tempKey);
			if (it != m_FiltersMap.end())
			{
				return it->second;
			}

			return CreateContainer(filter);
		}

		inline FilterContainerType* GetContainer(const FilterType& filter) const
		{
			auto it = m_FiltersMap.find(FilterEntryKeyType(filter));
			return it != m_FiltersMap.end() ? it->second : nullptr;
		}

		bool RemoveContainer(FilterContainerType* container)
		{
			if (container == nullptr || container->GetRefCount() > 0)
			{
				return false;
			}

			if (m_FiltersMap.erase(FilterEntryKeyType(container->m_Data)) == 0)
			{
				return false;
			}

			if (container == (&m_Allocator.Back()))
			{
				m_Allocator.PopBack();
			}
			else
			{
				m_FreeList.push_back(container);
			}

			return true;
		}

		IFilterContainerBase* CreateMatchingFilterContainer(const IFilterContainerBase& other) override
		{
			const FilterContainerType* otherCasted = ::decs::check_cast<const FilterContainerType*>(&other);

			auto it = m_FiltersMap.find(FilterEntryKeyType(otherCasted->m_Data));
			if (it != m_FiltersMap.end())
			{
				return it->second;
			}

			return CreateContainer(otherCasted->m_Data);
		}

		bool RemoveContainer(IFilterContainerBase* container) override
		{
			return RemoveContainer(::decs::check_cast<FilterContainerType*>(container));
		}

	private:
		TChunkedVector<FilterContainerType> m_Allocator{ 20 };
		std::vector<FilterContainerType*> m_FreeList{};
		ecsMap<FilterEntryKeyType, FilterContainerType*> m_FiltersMap{};

	private:
		FilterContainerType* CreateContainer(const FilterType& filter)
		{
			FilterContainerType* newContainer = nullptr;
			if (m_FreeList.empty())
			{
				newContainer = &m_Allocator.EmplaceBack(filter);
			}
			else
			{
				newContainer = m_FreeList.back();
				m_FreeList.pop_back();
				newContainer->m_Data = filter;
			}

			m_FiltersMap[FilterEntryKeyType(newContainer->m_Data)] = newContainer;
			return newContainer;
		}
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
			Clear();
		}

		void Clear()
		{
			for (auto& [key, filterTypeManager] : m_FilterTypes)
			{
				delete filterTypeManager;
			}
			m_FilterTypes.clear();
		}

		template<filter_concept FilterType>
		FilterContainer<FilterType>* GetOrCreateFilter(const FilterType& filter)
		{
			IFilterTypeManager*& filterTypeMangerBase = m_FilterTypes[Type<FilterType>::ID()];
			if (filterTypeMangerBase == nullptr)
			{
				filterTypeMangerBase = new FilterTypeManager<FilterType>();
			}

			FilterTypeManager<FilterType>* filterTypeManager = check_cast<FilterTypeManager<FilterType>*>(filterTypeMangerBase);
			return filterTypeManager->GetOrAddContainer(filter);
		}

		template<filter_concept FilterType>
		IFilterContainerBase* GetFilterWithoutIncrementRefCount(const FilterType& filter) const
		{
			auto filterManagerIt = m_FilterTypes.find(Type<FilterType>::ID());
			if (filterManagerIt == m_FilterTypes.end())
			{
				return nullptr;
			}

			return check_cast<const FilterTypeManager<FilterType>*>(filterManagerIt->second)->GetContainer(filter);
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
		bool DeleteFilter(FilterContainer<FilterType>* filterTypeContainer)
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

		bool DeleteFilter(IFilterContainerBase* filterTypeContainer)
		{
			if (filterTypeContainer == nullptr)
			{
				return false;
			}

			TypeID typeID = filterTypeContainer->GetDataTypeID();
			auto it = m_FilterTypes.find(typeID);
			if (it == m_FilterTypes.end())
			{
				return false;
			}
			return it->second->RemoveContainer(filterTypeContainer);
		}

	private:
		std::unordered_map<TypeID, IFilterTypeManager*> m_FilterTypes{};
	};

}