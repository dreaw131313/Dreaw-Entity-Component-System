#pragma once
#include <memory>

#include "decs/Core/TChunkedVector.h"
#include "decs/Light/Component/PackedLightComponentContainer.h"
#include "decs/Light/Filter/LFilter.h"

#include "LArchetype.h"

namespace decs::light
{
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

	struct ArchetypeGroup
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

	class ArchetypesGroupByOneType
	{
	public:
		ArchetypesGroupByOneType(
			TChunkedVector<ArchetypeGroup>& archetypeGroupAllocator,
			ArchetypeDataKey id
		):
			m_ArchetypeGroupAllocator(archetypeGroupAllocator),
			m_MainTypeID(id)
		{

		}

		inline bool IsFilterGroup() const noexcept
		{
			return m_MainTypeID.m_FilterContainer != nullptr;
		}

		inline IFilterContainerBase* GetFilterContainer() const noexcept
		{
			return m_MainTypeID.m_FilterContainer;
		}

		inline TypeID GetMainTypeID() const noexcept
		{
			return m_MainTypeID.m_TypeID;
		}

		inline Archetype* GetMainTypeArchetype() const
		{
			return m_MainTypeArchetype;
		}

		inline uint64_t GetArchetypesCount() const
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
			const uint64_t componentAndTagCount = archetype->GetComponentTagFilterCount();

			if (componentAndTagCount == 1)
			{
				if (IsFilterGroup())
				{
					DECS_ASSERT(m_MainTypeID.m_FilterContainer == archetype->GetFilters()[0].m_FilterContainer, "Single component archetype must have filter type id same as m_MainTypeID!");
				}
				else
				{
					DECS_ASSERT(m_MainTypeID.m_TypeID == archetype->GetComponentAndTagRecords()[0].m_TypeID, "Single component archetype must have component type same as m_MainTypeID!");
				}
				m_MainTypeArchetype = archetype;
			}

			if (componentAndTagCount > m_Groups.size())
			{
				m_Groups.resize(componentAndTagCount);
			}

			auto& archetypeGroup = m_Groups[componentAndTagCount - 1];
			if (archetypeGroup == nullptr)
			{
				archetypeGroup = &m_ArchetypeGroupAllocator.EmplaceBack();
			}
			archetypeGroup->Archetypes.push_back(archetype);
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
		TChunkedVector<ArchetypeGroup>& m_ArchetypeGroupAllocator;
		ArchetypeDataKey m_MainTypeID = std::numeric_limits<TypeID>::max();
		Archetype* m_MainTypeArchetype = nullptr;
		std::vector<ArchetypeGroup*> m_Groups;
		uint64_t m_ArchetypesCount = 0;
	};

	class ArchetypesMap
	{
		friend class Container;
		friend class ContainerIterator;
		template<TLightComponentConcept...>
		friend class IterationContainerContext;

	public:
		ArchetypesMap(
			FilterManager& filterManager,
			uint64_t archetypesVectorChunkSize,
			uint64_t archetypeGroupsVectorChunkSize
		);

		~ArchetypesMap();

		inline uint64_t GetArchetypesCount() const noexcept
		{
			return m_Archetypes.Size();
		}

		inline uint64_t EmptyArchetypesCount() const
		{
			uint64_t emptyArchetypesCount = 0;
			uint64_t archetypesCount = m_Archetypes.Size();

			for (uint64_t i = 0; i < archetypesCount; i++)
			{
				if (m_Archetypes[i].EntityCount() == 0)
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
			int64_t chunkCount = static_cast<int64_t>(m_Archetypes.ChunkCount());
			for (int64_t chunkIdx = chunkCount - 1; chunkIdx >= 0; chunkIdx--)
			{
				auto chunk = m_Archetypes.GetChunk(chunkIdx);
				int64_t elementCount = m_Archetypes.GetChunkSize(chunkIdx);

				for (int64_t elementIdx = elementCount - 1; elementIdx >= 0; elementIdx--)
				{
					func(&chunk[elementIdx]);
				}
			}
		}

		void ClearEntityDataAndComponents();

	private:
		FilterManager& m_FilterManager;

		TChunkedVector<Archetype> m_Archetypes{ 100 };
		TChunkedVector<ArchetypeGroup> m_ArchetrypesGroupsAllocator{ 100 };
		TChunkedVector<ArchetypesGroupByOneType> m_ArchetrypesGroupsByOneTypeAllocator{ 100 };

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

		template<TLightComponentConcept TComponent>
		Archetype* GetSingleComponentArchetype() const
		{
			return GetSingleComponentArchetype(Type<TComponent>::ID());
		}

		inline ArchetypesGroupByOneType* GetArchetypesGroup(ArchetypeDataKey id)
		{
			ArchetypesGroupByOneType*& group = m_ArchetypesGroupedByOneType[id];
			if (group == nullptr)
			{
				group = &m_ArchetrypesGroupsByOneTypeAllocator.EmplaceBack(m_ArchetrypesGroupsAllocator, id);
			}
			return group;
		}

		inline ArchetypesGroupByOneType* GetArchetypesGroupWithoutCreating(ArchetypeDataKey id) const
		{
			auto it = m_ArchetypesGroupedByOneType.find(id);
			if (it == m_ArchetypesGroupedByOneType.end())
			{
				return nullptr;
			}
			return it->second;
		}

		Archetype* FindMatchingArchetype(const Archetype& toArchetype);

		Archetype* GetOrCreateMatchedArchetype(Archetype& fromArchetype);

		void AddArchetypeToGroups(Archetype& arch);

		// CREATING ARCHETYPES
	private:
		template<TLightComponentConcept TComponent>
		Archetype* CreateSingleComponentArchetype()
		{
			TYPE_ID_CONSTEXPR TypeID componentTypeID = Type<TComponent>::ID();

			auto archetype = GetSingleComponentArchetype(componentTypeID);
			if (archetype != nullptr)
			{
				return archetype;
			}
			archetype = &m_Archetypes.EmplaceBack();
			archetype->AddTypeData_WithoutCheck(componentTypeID, new PackedLightComponentContainer<TComponent>());
			AddArchetypeToCorrectContainers(*archetype);
			return archetype;
		}

		template<TLightComponentConcept T>
		inline Archetype* GetArchetypeAfterAddComponent(Archetype& toArchetype)
		{
			TYPE_ID_CONSTEXPR TypeID addedComponentTypeID = Type<T>::ID();
			auto edge = toArchetype.GetEdge(addedComponentTypeID);

			if (edge.IsValid() && edge.m_EdgeType == EArchetypeEdgeType::Add)
			{
				return edge.m_Archetype;
			}

			if (toArchetype.ContainType(addedComponentTypeID))
			{
				DECS_ASSERT(false, "This path has no sense!");
				return nullptr;
			}

			Archetype& newArchetype = m_Archetypes.EmplaceBack();
			AddTypeDataAfterAddComponent(toArchetype, newArchetype, addedComponentTypeID, new PackedLightComponentContainer<T>());
			AddArchetypeToCorrectContainers(newArchetype);

			return &newArchetype;
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
			IFilterContainerBase* filterContainer = m_FilterManager.GetFilter<FilterType>(filter);
			Archetype* archetype = GetSingleFilterArchetype(filterContainer);
			if (archetype != nullptr)
			{
				return archetype;
			}

			archetype = &m_Archetypes.EmplaceBack();
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

			auto newFilterContainer = m_FilterManager.GetFilter<FilterType>(filter);

			auto edge = toArchetype.GetEdge(ArchetypeDataKey(newFilterContainer));
			if (edge.IsValid())
			{
				DECS_ASSERT(edge.m_EdgeType == EArchetypeEdgeType::Add, "It must be add edge!");

				return edge.m_Archetype;
			}

			Archetype& newArchetype = m_Archetypes.EmplaceBack();
			AddTypeDataAfterAddFilter(toArchetype, newArchetype, newFilterContainer);
			AddArchetypeToCorrectContainers(newArchetype);

			return &newArchetype;
		}

		Archetype* GetOrCreateArchetypeAfterRemoveFilter(const Archetype& fromArchetype, TypeID filterTypeID);

		template<filter_concept FilterType>
		Archetype* GetOrCreateArchetypeAfterRemoveFilter(const Archetype& fromArchetype)
		{
			return GetOrCreateArchetypeAfterRemoveFilter(fromArchetype, Type<FilterType>::ID());
		}


	};
}