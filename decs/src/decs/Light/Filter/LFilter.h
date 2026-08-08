#pragma once

#include "decs/Core/Core.h"
#include "decs/Core/check_cast.h"
#include "decs/Core/trait.h"
#include "decs/Core/Type.h"
#include "decs/Core/ObserverFunction.h"
#include "decs/Core/RefCounterHandle.h"
#include "decs/Core/TChunkedVector.h"

namespace decs::light
{
	class IFilterTypeManager;
	class FilterManager;

	struct Entity;

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

		virtual TypeID GetFilterTypeID() const noexcept = 0;

		virtual size_t GetDataHash() const noexcept = 0;

		virtual bool StoresSameData(const IFilterContainerBase& other) const = 0;

		virtual IFilterTypeManager* CreateFilterTypeManager() const = 0;

		virtual const void* GetFilterDataPtr() const = 0;

	private:
		uint32_t m_UseCount = 0;
	};

	template<filter_concept FilterType>
	class FilterContainer : public IFilterContainerBase
	{
	public:
		using filter_data_type = typename FilterType::DataType;
		using get_span_result = const typename FilterType::DataType&;

	public:
		filter_data_type m_Data{};
		size_t m_DataHash = 0;

	public:
		FilterContainer()
		{
			m_DataHash = std::hash<filter_data_type>{}(m_Data);
		}

		FilterContainer(const filter_data_type& data):
			m_Data(data)
		{
			m_DataHash = std::hash<filter_data_type>{}(m_Data);
		}

		template<typename...Args>
		FilterContainer(Args&&...args):
			m_Data(std::forward<Args>(args)...)
		{
			m_DataHash = std::hash<filter_data_type>{}(m_Data);
		}

		inline const filter_data_type& GetFilterData() const
		{
			return m_Data;
		}

		virtual TypeID GetFilterTypeID() const noexcept override
		{
			return Type<FilterType>::ID();
		}

		inline size_t GetDataHash() const noexcept
		{
			return m_DataHash;
		}

		bool StoresSameData(const IFilterContainerBase& other) const override
		{
			if (GetFilterTypeID() != other.GetFilterTypeID() || GetDataHash() != other.GetDataHash())
			{
				return false;
			}

			const FilterContainer<FilterType>* otherFilterContainer = check_cast<const FilterContainer<FilterType>*>(&other);

			return m_Data == otherFilterContainer->m_Data;
		}

		IFilterTypeManager* CreateFilterTypeManager() const override;

		const void* GetFilterDataPtr() const override
		{
			return &m_Data;
		}

		inline const filter_data_type& GetAsRef(size_t) const
		{
			return m_Data;
		}

		inline const filter_data_type* GetAsPtr(size_t) const
		{
			return &m_Data;
		}

		/*inline std::span<const FilterDataType> GetAsSpan() const
		{
			return { &m_Data , 1 };
		}*/

		inline const filter_data_type& GetAsSpan() const
		{
			return m_Data;
		}

	};

	template<filter_concept FilterType>
	class FilterEntryKey
	{
	public:
		using DataType = FilterType::DataType;
		const DataType* m_DataPtr = nullptr;

	public:
		FilterEntryKey() = default;

		FilterEntryKey(const DataType& filterData):
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

template<::decs::filter_concept FilterType>
struct std::hash<decs::light::FilterEntryKey<FilterType>>
{
	size_t operator ()(const decs::light::FilterEntryKey<FilterType>& v) const
	{
		if (v.m_DataPtr != nullptr)
		{
			return std::hash<typename FilterType::DataType>{}(*v.m_DataPtr);
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

		virtual void InvokeOnAddObserver(const Entity& entity, const void* filterData) = 0;

		virtual void InvokeOnRemoveObserver(const Entity& entity, const void* filterData) = 0;

		virtual void InvokeOnChangeObserver(const Entity& entity, const void* oldFilterData, const void* newFilterData) = 0;
	};

	template<filter_concept FilterType>
	class FilterTypeManager : public IFilterTypeManager
	{
	public:
		using FilterContainerType = FilterContainer<FilterType>;
		using FilterEntryKeyType = FilterEntryKey<FilterType>;

		using filter_data_type = FilterType::DataType;

		using ObserverFunction = TObserverFunction<void(const Entity&, const filter_data_type&)>;
		using ChangeFilterObserverFunction = TObserverFunction<void(const Entity&, const filter_data_type&, const filter_data_type&)>;

	public:
		ObserverFunction m_OnAddObserver{};
		ObserverFunction m_OnRemoveObserver{};
		ChangeFilterObserverFunction m_OnSetObserver{};

	public:
		void Clear() override
		{
			m_Allocator.Clear();
			m_FreeList.clear();
			m_FiltersMap.clear();
		}

		FilterContainerType* GetOrAddContainer(const filter_data_type& filter)
		{
			FilterEntryKeyType tempKey(filter);

			auto it = m_FiltersMap.find(tempKey);
			if (it != m_FiltersMap.end())
			{
				return it->second;
			}

			return CreateContainer(filter);
		}

		inline FilterContainerType* GetContainer(const filter_data_type& filter) const
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

		void InvokeOnAddObserver(const Entity& entity, const void* filterData) override
		{
			m_OnAddObserver.Invoke(entity, *static_cast<const filter_data_type*>(filterData));
		}

		void InvokeOnRemoveObserver(const Entity& entity, const void* filterData) override
		{
			m_OnRemoveObserver.Invoke(entity, *static_cast<const filter_data_type*>(filterData));
		}

		void InvokeOnChangeObserver(const Entity& entity, const void* oldFilterData, const void* newFilterData) override
		{
			m_OnSetObserver.Invoke(
				entity,
				*static_cast<const filter_data_type*>(oldFilterData),
				*static_cast<const filter_data_type*>(newFilterData)
			);
		}

		void InvokeOnAddObserver(const Entity& entity, const filter_data_type& filterData)
		{
			m_OnAddObserver.Invoke(entity, filterData);
		}

		void InvokeOnRemoveObserver(const Entity& entity, const filter_data_type& filterData)
		{
			m_OnRemoveObserver.Invoke(entity, filterData);
		}

		void InvokeOnSetObserver(const Entity& entity, const filter_data_type& oldFilterData, const filter_data_type& newFilterData)
		{
			m_OnSetObserver.Invoke(entity, oldFilterData, newFilterData);
		}

	private:
		TChunkedVector<FilterContainerType> m_Allocator{ 20 };
		ecsVector<FilterContainerType*> m_FreeList{};
		ecsHashMap<FilterEntryKeyType, FilterContainerType*> m_FiltersMap{};

	private:
		FilterContainerType* CreateContainer(const filter_data_type& filter)
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

	struct FilterGetResult
	{
	public:
		IFilterTypeManager* m_FilterTypeManager = nullptr;
		IFilterContainerBase* m_FilterContainer = nullptr;

	public:
		FilterGetResult() = default;

		FilterGetResult(
			IFilterTypeManager* filterTypeManager,
			IFilterContainerBase* filterContainer
		):
			m_FilterTypeManager(filterTypeManager),
			m_FilterContainer(filterContainer)
		{

		}
	};

	template<filter_concept FilterType>
	struct TFilterGetResult
	{
	public:
		FilterTypeManager<FilterType>* m_FilterTypeManager = nullptr;
		FilterContainer<FilterType>* m_FilterContainer = nullptr;

	public:
		TFilterGetResult() = default;

		TFilterGetResult(
			FilterTypeManager<FilterType>* filterTypeManager,
			FilterContainer<FilterType>* filterContainer
		):
			m_FilterTypeManager(filterTypeManager),
			m_FilterContainer(filterContainer)
		{

		}
	};

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
		FilterTypeManager<FilterType>* GetOrCreateFilterTypeManager()
		{
			IFilterTypeManager*& filterTypeMangerBase = m_FilterTypes[Type<FilterType>::ID()];
			if (filterTypeMangerBase == nullptr)
			{
				FilterTypeManager<FilterType>* filterTypeManager = new FilterTypeManager<FilterType>();
				filterTypeMangerBase = filterTypeManager;
			}
			return ::decs::check_cast<FilterTypeManager<FilterType>*>(filterTypeMangerBase);
		}

		template<filter_concept FilterType>
		TFilterGetResult<FilterType> GetOrCreateFilter(const FilterType::DataType& filter)
		{
			FilterTypeManager<FilterType>* filterTypeManager = GetOrCreateFilterTypeManager<FilterType>();

			return TFilterGetResult<FilterType>(filterTypeManager, filterTypeManager->GetOrAddContainer(filter));
		}

		template<filter_concept FilterType>
		TFilterGetResult<FilterType> GetFilterWithoutIncrementRefCount(const FilterType::DataType& filter) const
		{
			auto filterManagerIt = m_FilterTypes.find(Type<FilterType>::ID());
			if (filterManagerIt == m_FilterTypes.end())
			{
				return {};
			}

			auto filterTypeManager = ::decs::check_cast<FilterTypeManager<FilterType>*>(filterManagerIt->second);
			return TFilterGetResult<FilterType>(
				filterTypeManager,
				filterTypeManager->GetContainer(filter)
			);
		}

		FilterGetResult GetMatchingFilter(const IFilterContainerBase& other)
		{
			const TypeID filterTypeID = other.GetFilterTypeID();

			IFilterTypeManager*& filterTypeMangerBase = m_FilterTypes[filterTypeID];
			if (filterTypeMangerBase == nullptr)
			{
				filterTypeMangerBase = other.CreateFilterTypeManager();
			}

			return FilterGetResult(
				filterTypeMangerBase,
				filterTypeMangerBase->CreateMatchingFilterContainer(other)
			);
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

			TypeID typeID = filterTypeContainer->GetFilterTypeID();
			auto it = m_FilterTypes.find(typeID);
			if (it == m_FilterTypes.end())
			{
				return false;
			}
			return it->second->RemoveContainer(filterTypeContainer);
		}

	private:
		ecsHashMap<TypeID, IFilterTypeManager*> m_FilterTypes{};
	};

}