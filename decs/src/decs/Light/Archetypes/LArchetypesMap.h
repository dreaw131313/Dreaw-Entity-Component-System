#pragma once
#include <memory>

#include "decs/Core/TChunkedVector.h"
#include "decs/Core/ChunkAllocator.h"
#include "decs/Light/Component/PackedLightComponentContainer.h"
#include "decs/Light/Filter/LFilter.h"

#include "LArchetype.h"
#include "LArchetypeAllocator.h"
#include "decs/Light/Iteration/LQueryManager.h"

namespace decs::light
{
	class ArchetypesMap;
	template<typename...>
	class TFilterDataTuple;

	class ArchetypesShrinkToFitState
	{
		friend class ArchetypesMap;
	private:
		enum class State
		{
			Started,
			Ended
		};

	public:
		ArchetypesShrinkToFitState()
		{

		}

		ArchetypesShrinkToFitState(uint64_t archetypesToShrinkInOneCall, float maxArchetypeLoadFactor):
			m_ArchetypesToShrinkInOneCall(archetypesToShrinkInOneCall),
			m_MaxArchetypeLoadFactor(maxArchetypeLoadFactor)
		{

		}

		void Reset()
		{
			m_State = State::Ended;
			m_ArchetypesCountToShrink = 0;
			m_CurretnArchetypeIndex = 0;
		}

		inline bool IsEnded()
		{
			return m_State == State::Ended;
		}

	private:
		State m_State = State::Ended;
		uint64_t m_ArchetypesCountToShrink = 0;
		uint64_t m_ArchetypesToShrinkInOneCall = 100;
		uint64_t m_CurretnArchetypeIndex = 0;
		float m_MaxArchetypeLoadFactor = 1.f;

	private:
		void Start(const uint64_t& archetypesToShrink)
		{
			m_State = State::Started;
			m_ArchetypesCountToShrink = archetypesToShrink;
			m_CurretnArchetypeIndex = 0;
		}
	};

	struct ArchetypeDestroyState
	{
		friend class ArchetypesMap;
	public:

	private:
		size_t m_LastCheckdArchetypeIndex = 0;
	};

	struct ArchetypeDestroyConfig
	{
	public:
		size_t m_MaxArchetypesToCheck = 10;
		size_t m_MaxArchetypesDestroy = 2;
		bool m_bDestroyOnlyArchetypesWithFilters = true;
	};

	struct ArchetypeGroup : public ChunkAllocatorResource
	{
	public:
		std::vector<Archetype*> Archetypes;

	public:
		ArchetypeGroup() = default;

		inline uint32_t GetArchetypeCount() const noexcept
		{
			return static_cast<uint32_t>(Archetypes.size());
		}
	};

	enum class EArchetypesGroupType : uint8_t
	{
		ComponentOrTagType,
		FilterType,
		FilterData
	};

	class ArchetypesGroupByOneType : public ChunkAllocatorResource
	{
	public:
		ArchetypesGroupByOneType(
			TChunkAllocator<ArchetypeGroup>& archetypeGroupAllocator,
			TypeID id,
			EArchetypesGroupType groupType
		):
			m_ArchetypeGroupAllocator(archetypeGroupAllocator),
			m_MainTypeID(id),
			m_GroupType(groupType)
		{

		}

		~ArchetypesGroupByOneType()
		{
			for (auto group : m_Groups)
			{
				m_ArchetypeGroupAllocator.Destroy(group);
			}
		}

		EArchetypesGroupType GetGroupType() const noexcept
		{
			return m_GroupType;
		}

		inline bool ShouldHaveMainArchetype() const noexcept
		{
			return m_GroupType == EArchetypesGroupType::FilterData || m_GroupType == EArchetypesGroupType::ComponentOrTagType;
		}

		inline bool IsFilterGroup() const noexcept
		{
			return m_GroupType == EArchetypesGroupType::FilterType || m_GroupType == EArchetypesGroupType::FilterData;
		}

		inline Archetype* GetMainTypeArchetype() const
		{
			// if group is of type FilterType, then there can be more than one archetype with one filter of this type but with different filter data

			if (m_GroupType == EArchetypesGroupType::FilterData || m_GroupType == EArchetypesGroupType::ComponentOrTagType)
			{
				return m_MainTypeArchetype;
			}
			return nullptr;
		}

		inline bool IsEmpty() const noexcept
		{
			return m_ArchetypesCount == 0;
		}

		inline size_t GetArchetypesCount() const
		{
			return m_ArchetypesCount;
		}

		inline size_t GetMaxComponentTagFilterCount() const
		{
			return m_Groups.size();
		}

		void AddArchetype(Archetype* archetype)
		{
			m_ArchetypesCount += 1;
			const uint64_t componentTagFilterCount = archetype->GetComponentTagFilterCount();

			if (componentTagFilterCount == 1 && ShouldHaveMainArchetype())
			{
				if (IsFilterGroup())
				{
					DECS_ASSERT(
						m_MainTypeID == archetype->GetFilters()[0].m_FilterTypeID,
						"Single component archetype must have filter type id same as m_MainTypeID!"
					);
				}
				else
				{
					DECS_ASSERT(
						m_MainTypeID == archetype->GetComponentAndTagRecords()[0].m_TypeID,
						"Single component archetype must have component type same as m_MainTypeID!"
					);
				}
				m_MainTypeArchetype = archetype;
			}

			if (componentTagFilterCount > m_Groups.size())
			{
				m_Groups.resize(componentTagFilterCount);
			}

			auto& archetypeGroup = m_Groups[componentTagFilterCount - 1];
			if (archetypeGroup == nullptr)
			{
				archetypeGroup = m_ArchetypeGroupAllocator.Create();
			}
			archetypeGroup->Archetypes.push_back(archetype);
		}

		void RemoveArchetype(Archetype* archetype)
		{
			if (IsEmpty())
			{
				return;
			}

			const uint64_t componentTagFilterCount = archetype->GetComponentTagFilterCount();

			if (componentTagFilterCount > m_Groups.size())
			{
				return;
			}

			auto& archetypes = m_Groups[componentTagFilterCount - 1]->Archetypes;
			for (size_t idx = 0; idx < archetypes.size(); idx++)
			{
				if (archetypes[idx] == archetype)
				{
					if (archetype == m_MainTypeArchetype)
					{
						m_MainTypeArchetype = nullptr;
					}

					if (idx < (archetypes.size() - 1))
					{
						archetypes[idx] = archetypes.back();
					}
					archetypes.pop_back();
					m_ArchetypesCount--;
					break;
				}
			}
		}

		std::span<Archetype*> GetArchetypesWithComponentTagFilterCount(uint64_t componentsCount) const
		{
			uint64_t groupIndex = componentsCount - 1;
			if (groupIndex >= m_Groups.size())
			{
				return {};
			}
			auto group = m_Groups[groupIndex];
			if (group == nullptr)
			{
				return {};
			}
			return group->Archetypes;
		}

		const ArchetypeGroup* GetGroupWithComponentTagFilterCount(uint64_t componentsCount) const
		{
			uint64_t groupIndex = componentsCount - 1;
			if (groupIndex >= m_Groups.size())
			{
				return nullptr;
			}
			return m_Groups[groupIndex];
		}

		inline Archetype* GetSingleComponentArchetype() const
		{
			if (m_Groups.empty())
			{
				return nullptr;
			}

			auto group = m_Groups.front();
			if (group == nullptr || group->Archetypes.empty())
			{
				return nullptr;
			}
			return group->Archetypes.front();
		}

		template<typename Callable>
		void IterateOverAllArchetypes(Callable&& func)
		{
			uint64_t archetypesGroupCount = m_Groups.size();
			for (uint64_t groupIdx = 0; groupIdx < archetypesGroupCount; groupIdx++)
			{
				auto group = m_Groups[groupIdx];
				if (group != nullptr)
				{
					auto& groupArchetypes = group->Archetypes;
					uint64_t archetypeCount = groupArchetypes.size();
					for (uint64_t archIdx = 0; archIdx < archetypeCount; archIdx++)
					{
						auto archetype = groupArchetypes[archIdx];
						func(archetype);
					}
				}
			}
		}

	private:
		TChunkAllocator<ArchetypeGroup>& m_ArchetypeGroupAllocator;
		std::vector<ArchetypeGroup*> m_Groups{};
		size_t m_ArchetypesCount = 0;

		TypeID m_MainTypeID = std::numeric_limits<TypeID>::max();
		Archetype* m_MainTypeArchetype = nullptr;

		EArchetypesGroupType m_GroupType = EArchetypesGroupType::ComponentOrTagType;
	};

	class ArchetypesMap
	{
		friend class Container;
		friend class ContainerIterator;
		template<light_component_or_filter_concept...>
		friend class IterationContainerContext;
		template<typename...>
		friend class TFilterDataTuple;

	public:
		ArchetypesMap(
			FilterManager& filterManager,
			QueryManager& queryManger,
			uint64_t archetypesVectorChunkSize,
			uint64_t archetypeGroupsVectorChunkSize
		);

		~ArchetypesMap();

		inline uint64_t GetArchetypesCount() const noexcept
		{
			return m_ArchetypeAllocator.GetCreatedArchetypes().size();
		}

		inline uint64_t EmptyArchetypesCount() const
		{
			uint64_t emptyArchetypesCount = 0;

			for (auto archetype : m_ArchetypeAllocator.GetCreatedArchetypes())
			{
				if (archetype->EntityCount() == 0)
				{
					emptyArchetypesCount += 1;
				}
			}

			return emptyArchetypesCount;
		}

		/// <summary>
		/// 
		/// </summary>
		/// <returns>Max number of componets + tags + filters in archetype</returns>
		inline size_t GetMaxComponentTagFilterCount() const
		{
			return m_MaxComponentTagFilterCount;
		}

		void ShrinkArchetypesToFit();

		void ShrinkArchetypesToFit(ArchetypesShrinkToFitState& state);

		template<typename Callable>
		void IterateOverArchetypesWithType(TypeID componentType, Callable&& func)
		{
			auto groupedArchetypesIt = m_ArchetypesGroupedByOneType.find(componentType);
			if (groupedArchetypesIt == m_ArchetypesGroupedByOneType.end())
			{
				return;
			}
			groupedArchetypesIt->second->IterateOverAllArchetypes(func);
		}

		template<typename Callable>
		void IterateOverArchetypes(Callable&& func)
		{
			auto archetypes = m_ArchetypeAllocator.GetCreatedArchetypes();
			for (auto archetype : archetypes)
			{
				func(archetype);
			}
		}

		void ClearEntityDataAndComponents();

	private:
		FilterManager& m_FilterManager;
		QueryManager& m_QueryManager;

		ArchetypeAllocator m_ArchetypeAllocator;
		TChunkAllocator<ArchetypeGroup> m_ArchetypesGroupsAllocator{ 100 };
		TChunkAllocator<ArchetypesGroupByOneType> m_ArchetypesGroupsByOneTypeAllocator{ 100 };

		ecsMap<ArchetypeDataKey, ArchetypesGroupByOneType*> m_ArchetypesGroupedByOneType{};
		ecsMap<ArchetypeHasher, Archetype*> m_HashedArchetypes{};

		size_t m_MaxComponentTagFilterCount = 0;
		// UTILITY
	private:
		void MakeArchetypeEdges_4(Archetype& archetype);

		void AddArchetypeToCorrectContainers(Archetype& archetype);

		/// <summary>
		/// Can be used to get single tags components
		/// </summary>
		/// <param name="typeID"></param>
		/// <returns></returns>
		inline Archetype* GetSingleComponentArchetype(TypeID typeID) const
		{
			auto it = m_ArchetypesGroupedByOneType.find(typeID);
			return it != m_ArchetypesGroupedByOneType.end() ? it->second->GetMainTypeArchetype() : nullptr;
		}

		template<light_component_or_filter_concept TComponent>
		Archetype* GetSingleComponentArchetype() const
		{
			return GetSingleComponentArchetype(Type<TComponent>::ID());
		}

		ArchetypesGroupByOneType* GetArchetypesGroup(ArchetypeDataKey id, EArchetypesGroupType groupType);

		ArchetypesGroupByOneType* GetArchetypesGroupWithoutCreating(ArchetypeDataKey id) const;

		Archetype* FindMatchingArchetype(const Archetype& toArchetype);

		Archetype* GetOrCreateMatchedArchetype(Archetype& fromArchetype);

		void AddArchetypeToGroups(Archetype& arch);

		// CREATING ARCHETYPES
	private:
		template<light_component_or_filter_concept TComponent>
		Archetype* CreateSingleComponentArchetype()
		{
			TYPE_ID_CONSTEXPR TypeID componentTypeID = Type<TComponent>::ID();

			auto archetype = GetSingleComponentArchetype(componentTypeID);
			if (archetype != nullptr)
			{
				return archetype;
			}
			archetype = m_ArchetypeAllocator.CreateArchetype();
			archetype->AddTypeData_WithoutCheck(componentTypeID, new PackedLightComponentContainer<TComponent>());
			AddArchetypeToCorrectContainers(*archetype);
			return archetype;
		}

		template<light_component_or_filter_concept T>
		inline Archetype* GetArchetypeAfterAddComponent(Archetype& toArchetype)
		{
			TYPE_ID_CONSTEXPR TypeID addedComponentTypeID = Type<T>::ID();
			auto edge = toArchetype.GetEdge(addedComponentTypeID);

			if (edge.IsValid() && edge.m_EdgeType == EArchetypeEdgeType::Add)
			{
				return edge.m_Archetype;
			}

			if (toArchetype.ContainComponentOrTagType(addedComponentTypeID))
			{
				DECS_ASSERT(false, "This path has no sense!");
				return nullptr;
			}

			Archetype* newArchetype = m_ArchetypeAllocator.CreateArchetype();
			AddTypeDataAfterAddComponent(toArchetype, *newArchetype, addedComponentTypeID, new PackedLightComponentContainer<T>());
			AddArchetypeToCorrectContainers(*newArchetype);

			return newArchetype;
		}

		Archetype* CreateArchetypeAfterAddComponent(const Archetype& toArchetype, TypeID componentTypeID, IPackedLightComponentContainer* packedContainer);

		Archetype* GetArchetypeAfterRemoveComponent(const Archetype& fromArchetype, TypeID removedComponentTypeID);

		Archetype* GetArchetypeAfterAddTag(const Archetype& toArchetype, TypeID tagType);

		Archetype* GetArchetypeAfterRemoveTag(const Archetype& fromArchetype, TypeID tagType);

		Archetype* CreateSingleTagArchetype(TypeID componentTypeID);

		void AddTypeDataAfterRemoveComponent(const Archetype& fromArchetype, Archetype& toArchetype, TypeID compType);

		void AddTypeDataAfterAddComponent(const Archetype& baseArchetype, Archetype& toArchetype, TypeID componentTypeID, IPackedLightComponentContainer* packedContainer);

		void AddTypeDataAfterRemoveFilter(const Archetype& fromArchetype, Archetype& toArchetype, IFilterContainerBase* filterContainer);

		void AddTypeDataAfterAddFilter(const Archetype& baseArchetype, Archetype& toArchetype, IFilterContainerBase* filterContainer);

		// FITLER ARCHETYPES:
		Archetype* GetSingleFilterArchetype(IFilterContainerBase* filterContainer)
		{
			auto it = m_ArchetypesGroupedByOneType.find(filterContainer);
			return it != m_ArchetypesGroupedByOneType.end() ? it->second->GetMainTypeArchetype() : nullptr;
		}

		template<filter_concept FilterType>
		Archetype* GetOrCreateSingleFilterArchetype(const FilterType& filter)
		{
			IFilterContainerBase* filterContainer = m_FilterManager.GetOrCreateFilter<FilterType>(filter);
			Archetype* archetype = GetSingleFilterArchetype(filterContainer);
			if (archetype != nullptr)
			{
				return archetype;
			}

			archetype = m_ArchetypeAllocator.CreateArchetype();
			archetype->AddFilter_WithoutCheckout(filterContainer);
			AddArchetypeToCorrectContainers(*archetype);
			return archetype;
		}

		template<filter_concept FilterType>
		Archetype* GetOrCreateArchetypeAfterSetFilter(const Archetype& toArchetype, const FilterType& filter)
		{
			FilterContainer<FilterType>* archetypeFilter = toArchetype.GetFilterContainer<FilterType>();
			if (archetypeFilter != nullptr)
			{
				if (archetypeFilter->m_Data == filter)
				{
					// return same container so we can drop const
					return const_cast<Archetype*>(&toArchetype);
				}

				Archetype* archetypeWithoutFilter = GetOrCreateArchetypeAfterRemoveFilter<FilterType>(toArchetype);
				if (archetypeWithoutFilter == nullptr)
				{
					return GetOrCreateSingleFilterArchetype<FilterType>(filter);
				}
				else
				{
					return GetOrCreateArchetypeAfterSetFilter<FilterType>(*archetypeWithoutFilter, filter);
				}
			}

			auto newFilterContainer = m_FilterManager.GetOrCreateFilter<FilterType>(filter);

			auto edge = toArchetype.GetEdge(ArchetypeDataKey(newFilterContainer));
			if (edge.IsValid())
			{
				DECS_ASSERT(edge.m_EdgeType == EArchetypeEdgeType::Add, "It must be add edge!");

				return edge.m_Archetype;
			}

			Archetype* newArchetype = m_ArchetypeAllocator.CreateArchetype();
			AddTypeDataAfterAddFilter(toArchetype, *newArchetype, newFilterContainer);
			AddArchetypeToCorrectContainers(*newArchetype);

			return newArchetype;
		}

		Archetype* GetOrCreateArchetypeAfterRemoveFilter(const Archetype& fromArchetype, TypeID filterTypeID);

		template<filter_concept FilterType>
		Archetype* GetOrCreateArchetypeAfterRemoveFilter(const Archetype& fromArchetype)
		{
			return GetOrCreateArchetypeAfterRemoveFilter(fromArchetype, Type<FilterType>::ID());
		}

		// DESTROYING ARCHETYPES OVER TIME
		void TryDestroyArchetypes(ArchetypeDestroyState& state, const ArchetypeDestroyConfig& config);

		void RemoveArchetypeFromMap(Archetype* archetpye);
	};


	class IFilterDataTuple : public RefCountedObject
	{
	public:
		virtual bool TestArchetype(const Archetype& archetpye)  const = 0;

		/// <summary>
		/// 
		/// </summary>
		/// <returns>group if found group which has less componentTagFilterInGroup than currentMaxComponentTagFilterInGroup, if not find nullptr</returns>
		virtual const ArchetypesGroupByOneType* GetBestArchetypeGroup(
			const ArchetypesMap& archetypesMap,
			size_t smallestArchetypeCount
		) const = 0;
	};

	using IFilterDataTupleHandle = TRefCountHandle<IFilterDataTuple>;

	template<typename... FilterTypes>
	class TFilterDataTuple final : public IFilterDataTuple
	{
	public:
		std::tuple<pure_type_t<FilterTypes>...> m_FiltersData{};

	public:
		TFilterDataTuple(FilterTypes&&... filterData):
			m_FiltersData(std::forward_as_tuple(std::forward<FilterTypes>(filterData)...))
		{

		}

		bool TestArchetype(const Archetype& archetype) const override
		{
			return (CompareFilterTypeData<FilterTypes>(archetype) && ...);
		}

	private:
		template<typename T>
		bool CompareFilterTypeData(const Archetype& archetype) const
		{
			FilterContainer<T>* filterContainer = archetype.GetFilterContainer<T>();
			if (filterContainer == nullptr)
			{
				return false;
			}

			const T& data = std::get<T>(m_FiltersData);

			return filterContainer->m_Data == data;
		}

		const ArchetypesGroupByOneType* GetBestArchetypeGroup(
			const ArchetypesMap& archetypesMap,
			size_t smallestArchetypeCount
		) const override
		{
			std::pair<const ArchetypesGroupByOneType*, size_t> bestResult = { nullptr, smallestArchetypeCount };

			((bestResult = GetGroup<pure_type_t<FilterTypes>>(archetypesMap, bestResult.first, bestResult.second)), ...);

			return bestResult.first;
		}

		template<typename T>
		std::pair<const ArchetypesGroupByOneType*, size_t> GetGroup(const ArchetypesMap& archetypesMap, const ArchetypesGroupByOneType* currentGroup, size_t smallestArchetypeCount) const
		{
			const FilterManager& filterManager = archetypesMap.m_FilterManager;
			IFilterContainerBase* filterContainer = filterManager.GetFilterWithoutIncrementRefCount(std::get<pure_type_t<T>>(m_FiltersData));
			if (filterContainer == nullptr)
			{
				return { currentGroup, smallestArchetypeCount };
			}

			auto& groupsMap = archetypesMap.m_ArchetypesGroupedByOneType;
			auto groupIt = groupsMap.find(ArchetypeDataKey(filterContainer));
			if (groupIt == groupsMap.end())
			{
				return { currentGroup, smallestArchetypeCount };
			}

			const ArchetypesGroupByOneType* newBestGroup = groupIt->second;

			if (newBestGroup->GetArchetypesCount() >= smallestArchetypeCount)
			{
				return { currentGroup, smallestArchetypeCount };
			}

			return { newBestGroup, newBestGroup->GetArchetypesCount() };
		}
	};

	template<typename... FilterTypes>
	using TFilterDataTupleHandle = TRefCountHandle<TFilterDataTuple<FilterTypes...>>;
}