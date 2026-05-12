#pragma once
#include "LArchetypesMap.h"

#include <cassert>

namespace decs::light
{
	ArchetypesMap::ArchetypesMap(
		FilterManager& filterManager,
		QueryManager& queryManger,
		uint64_t archetypesVectorChunkSize,
		uint64_t archetypeGroupsVectorChunkSize
	):
		m_FilterManager(filterManager),
		m_ArchetypeAllocator(static_cast<uint32_t>(archetypesVectorChunkSize)),
		m_QueryMangaer(queryManger),
		m_ArchetypesGroupsByOneTypeAllocator(archetypeGroupsVectorChunkSize)
	{

	}

	ArchetypesMap::~ArchetypesMap()
	{
	}

	void ArchetypesMap::ShrinkArchetypesToFit()
	{
		if (GetArchetypesCount() == 0)
		{
			return;
		}

		m_ArchetypeAllocator.IterateOverAllArchetypes([](Archetype& arch)
		{
			arch.ShrinkToFit();
		});
	}

	void ArchetypesMap::ShrinkArchetypesToFit(ArchetypesShrinkToFitState& state)
	{
		/*if (GetArchetypesCount() == 0)
		{
			return;
		}

		if (state.m_State == ArchetypesShrinkToFitState::State::Ended)
		{
			state.Start(m_Archetypes.Size());
		}

		int archetypesToShrink = (int)state.m_ArchetypesToShrinkInOneCall;

		for (uint64_t idx = state.m_CurretnArchetypeIndex; idx < state.m_ArchetypesCountToShrink; idx++)
		{
			state.m_CurretnArchetypeIndex += 1;
			Archetype& archetype = m_Archetypes[idx];

			float loadFactor = archetype.GetLoadFactor();
			if (loadFactor <= state.m_MaxArchetypeLoadFactor)
			{
				archetype.ShrinkToFit();
				archetypesToShrink--;
			}

			if (archetypesToShrink <= 0)
			{
				break;
			}
		}

		if (state.m_CurretnArchetypeIndex >= state.m_ArchetypesCountToShrink)
		{
			state.Reset();
		}*/
	}

	void ArchetypesMap::ClearEntityDataAndComponents()
	{
		IterateOverArchetypes([](Archetype* arch)
		{
			arch->ClearEntityDataAndComponents();
		});
	}

	void ArchetypesMap::MakeArchetypeEdges_4(Archetype& archetype)
	{
		const size_t typeCount = archetype.GetComponentTagFilterCount();
		const size_t addTypeNeighbourTypeCount = typeCount + 1;
		const size_t removeTypeNeighbourTypeCount = typeCount - 1;

		const ArchetypeGroup* bestAddTypeGroup = nullptr;
		size_t bestAddTypeArchetypeCount = std::numeric_limits<size_t>::max();

		auto findBestAddTypeGroup = [&](const ArchetypesGroupByOneType& typeGroup)
		{
			const ArchetypeGroup* currentAddTypeGroup = typeGroup.GetGroupWithComponentTagFilterCount(static_cast<uint64_t>(addTypeNeighbourTypeCount));
			if (currentAddTypeGroup != nullptr && currentAddTypeGroup->GetArchetypeCount() < bestAddTypeArchetypeCount)
			{
				bestAddTypeGroup = currentAddTypeGroup;
				bestAddTypeArchetypeCount = currentAddTypeGroup->GetArchetypeCount();
			}

			if (typeCount > 1)
			{
				/*
				* This potenitaly is faster than checking all existing archetypes with smaller number of components
				*/
				const ArchetypeGroup* removeComponentGroup = typeGroup.GetGroupWithComponentTagFilterCount(static_cast<uint64_t>(removeTypeNeighbourTypeCount));
				if (removeComponentGroup != nullptr)
				{
					for (Archetype* neighbour : removeComponentGroup->Archetypes)
					{
						auto edgeTypeID = archetype.IsRemoveAnyDataNeighbour(*neighbour);
						if (edgeTypeID.has_value() && !archetype.HasAnyEdge(edgeTypeID.value()))
						{
							neighbour->AddEdge(edgeTypeID.value(), &archetype, EArchetypeEdgeType::Add);
							archetype.AddEdge(edgeTypeID.value(), neighbour, EArchetypeEdgeType::Remove);
						}
					}
				}
			}
		};

		// COMPONENTS AND TAGS
		for (auto& typeDataRecord : archetype.GetComponentAndTagRecords())
		{
			const ArchetypesGroupByOneType* typeGroup = GetArchetypesGroupWithoutCreating(typeDataRecord.m_TypeID);

			DECS_ASSERT(typeGroup != nullptr, "This should never fail, because archetype is added to all its types groups:");
			findBestAddTypeGroup(*typeGroup);
		}

		// FILTERS:
		for (auto& filterRecord : archetype.GetFilters())
		{
			const ArchetypesGroupByOneType* typeGroup = GetArchetypesGroupWithoutCreating(filterRecord.m_FilterContainer);

			DECS_ASSERT(typeGroup != nullptr, "This should never fail, because archetype is added to all its types groups:");
			findBestAddTypeGroup(*typeGroup);
		}

		/*
		* Add component neighbours
		* We know that all add component neighbours will be placed in same one type groups as tested archetype, so we need to check neighbours only in one group.
		* Not all archetypes with typeCount + 1 in any single component group are neighbours of tested archetype, but all typeCount + 1 neighbours of tested archetype are in all singlecomponent groups to which this archetype belongs
		* So we first select group with smallest number of archetypes with typeCount + 1 and then test archetypes from best group
		*/

		if (bestAddTypeGroup != nullptr)
		{
			for (Archetype* neighbour : bestAddTypeGroup->Archetypes)
			{
				if (auto edgeTypeID = archetype.IsAddAnyDataNeighbour(*neighbour))
				{
					neighbour->AddEdge(edgeTypeID.value(), &archetype, EArchetypeEdgeType::Remove);
					archetype.AddEdge(edgeTypeID.value(), neighbour, EArchetypeEdgeType::Add);
				}
			}
		}
	}

	void ArchetypesMap::AddArchetypeToCorrectContainers(Archetype& archetype)
	{
		AddArchetypeToGroups(archetype);

		m_HashedArchetypes[ArchetypeHasher(&archetype)] = &archetype;

		if (archetype.GetComponentTagFilterCount() > m_MaxComponentTagFilterCount)
		{
			m_MaxComponentTagFilterCount = archetype.GetComponentTagFilterCount();
		}

		MakeArchetypeEdges_4(archetype);

		m_QueryMangaer.OnCreateArchetype(&archetype);
	}

	Archetype* ArchetypesMap::FindMatchingArchetype(const Archetype& toArchetype)
	{
		const uint64_t typesCount = toArchetype.GetComponentTagCount();
		if (typesCount == 0)
		{
			return nullptr;
		}

		ArchetypeHasher hasherToMatch(&toArchetype);
		auto hashedArchetypeIt = m_HashedArchetypes.find(hasherToMatch);
		if (hashedArchetypeIt != m_HashedArchetypes.end())
		{
			return hashedArchetypeIt->second;
		}

		return nullptr;
	}

	Archetype* ArchetypesMap::GetOrCreateMatchedArchetype(Archetype& fromArchetype)
	{
		Archetype* archetype = FindMatchingArchetype(fromArchetype);

		if (archetype == nullptr)
		{
			archetype = m_ArchetypeAllocator.CreateArchetype();
			archetype->InitEmptyFromOther(fromArchetype, m_FilterManager);
			AddArchetypeToCorrectContainers(*archetype);
		}

		return archetype;
	}

	void ArchetypesMap::AddArchetypeToGroups(Archetype& arch)
	{
		for (auto& record : arch.GetComponentAndTagRecords())
		{
			ArchetypesGroupByOneType* group = GetArchetypesGroup(record.m_TypeID, EArchetypesGroupType::ComponentOrTagType);
			group->AddArchetype(&arch);
		}

		for (auto& filter : arch.GetFilters())
		{
			{
				ArchetypesGroupByOneType* filterDataGroup = GetArchetypesGroup(filter.m_FilterContainer, EArchetypesGroupType::FilterData);
				filterDataGroup->AddArchetype(&arch);
			}

			{
				ArchetypesGroupByOneType* filterTypeGroup = GetArchetypesGroup(filter.m_FilterContainer->GetDataTypeID(), EArchetypesGroupType::FilterType);
				filterTypeGroup->AddArchetype(&arch);
			}
		}
	}

	Archetype* ArchetypesMap::CreateArchetypeAfterAddComponent(const Archetype& toArchetype, TypeID componentTypeID, IPackedLightComponentContainer* packedContainer)
	{
		//auto& edge = toArchetype.m_AddEdges[addedComponentTypeID];
		auto edge = toArchetype.GetEdge(componentTypeID);
		if (edge.IsValid())
		{
			if (edge.m_EdgeType == EArchetypeEdgeType::Add)
			{
				return edge.m_Archetype;
			}
			else
			{
				return nullptr;
			}
		}

		if (toArchetype.ContainComponentOrTagType(componentTypeID))
		{
			assert(false);
			return nullptr;
		}

		Archetype* newArchetype = m_ArchetypeAllocator.CreateArchetype();
		AddTypeDataAfterAddComponent(toArchetype, *newArchetype, componentTypeID, packedContainer);

		AddArchetypeToCorrectContainers(*newArchetype);

		return newArchetype;
	}

	Archetype* ArchetypesMap::GetArchetypeAfterRemoveComponent(const Archetype& fromArchetype, TypeID removedComponentTypeID)
	{
		if (fromArchetype.GetComponentTagCount() == 1 && fromArchetype.GetTypeID(0) == removedComponentTypeID)
		{
			return nullptr;
		}

		// first we check if we have edge it will fail only once
		auto edge = fromArchetype.GetEdge(removedComponentTypeID);
		if (edge.IsValid())
		{
			if (edge.m_EdgeType == EArchetypeEdgeType::Remove)
			{
				return edge.m_Archetype;
			}
			else
			{
				return nullptr;
			}
		}

		DECS_ASSERT(fromArchetype.ContainComponentOrTagType(removedComponentTypeID), "from archetype mus have component with typeID removedComponentTypeID!");

		Archetype* newArchetype = m_ArchetypeAllocator.CreateArchetype();
		AddTypeDataAfterRemoveComponent(fromArchetype, *newArchetype, removedComponentTypeID);
		AddArchetypeToCorrectContainers(*newArchetype);

		return newArchetype;
	}

	Archetype* ArchetypesMap::GetArchetypeAfterAddTag(const Archetype& toArchetype, TypeID tagType)
	{
		return CreateArchetypeAfterAddComponent(toArchetype, tagType, nullptr);
	}

	Archetype* ArchetypesMap::GetArchetypeAfterRemoveTag(const Archetype& fromArchetype, TypeID tagType)
	{
		return GetArchetypeAfterRemoveComponent(fromArchetype, tagType);
	}

	Archetype* ArchetypesMap::CreateSingleTagArchetype(TypeID componentTypeID)
	{
		auto archetype = GetSingleComponentArchetype(componentTypeID);
		if (archetype != nullptr)
		{
			return archetype;
		}
		archetype = m_ArchetypeAllocator.CreateArchetype();
		archetype->AddTypeData_WithoutCheck(componentTypeID, nullptr);
		AddArchetypeToCorrectContainers(*archetype);

		return archetype;
	}

	void ArchetypesMap::AddTypeDataAfterRemoveComponent(const Archetype& fromArchetype, Archetype& toArchetype, TypeID removedComponentType)
	{
		for (uint32_t i = 0; i < fromArchetype.GetComponentTagCount(); i++)
		{
			const ArchetypeTypeData& fromArchetypeData = fromArchetype.m_TypeData[i];
			if (fromArchetypeData.m_TypeID != removedComponentType)
			{
				toArchetype.AddTypeData_WithoutCheck(
					fromArchetypeData.m_TypeID,
					fromArchetypeData.m_PackedContainer != nullptr ? fromArchetypeData.m_PackedContainer->CloneEmpty() : nullptr
				);
			}
		}

		for (auto& baseFilterRecord : fromArchetype.GetFilters())
		{
			toArchetype.AddFilter_WithoutCheckout(baseFilterRecord.m_FilterContainer);
		}
	}

	void ArchetypesMap::AddTypeDataAfterAddComponent(const Archetype& baseArchetype, Archetype& toArchetype, TypeID componentTypeID, IPackedLightComponentContainer* packedContainer)
	{
		bool isNewComponentTypeAdded = false;

		for (uint32_t i = 0; i < baseArchetype.GetComponentTagCount(); i++)
		{
			const ArchetypeTypeData& baseTypeData = baseArchetype.m_TypeData[i];

			if (!isNewComponentTypeAdded && baseTypeData.m_TypeID > componentTypeID)
			{
				isNewComponentTypeAdded = true;
				toArchetype.AddTypeData_WithoutCheck(
					componentTypeID,
					packedContainer
				);
			}

			toArchetype.AddTypeData_WithoutCheck(
				baseTypeData.m_TypeID,
				baseTypeData.m_PackedContainer != nullptr ? baseTypeData.m_PackedContainer->CloneEmpty() : nullptr
			);
		}

		if (!isNewComponentTypeAdded)
		{
			toArchetype.AddTypeData_WithoutCheck(
				componentTypeID,
				packedContainer
			);
		}

		for (auto& baseFilterRecord : baseArchetype.GetFilters())
		{
			toArchetype.AddFilter_WithoutCheckout(baseFilterRecord.m_FilterContainer);
		}

	}

	void ArchetypesMap::AddTypeDataAfterRemoveFilter(const Archetype& fromArchetype, Archetype& toArchetype, IFilterContainerBase* filterContainer)
	{
		DECS_ASSERT(filterContainer != nullptr, "Filter must not be nullptr!");

		for (auto& baseTypeRecord : fromArchetype.GetComponentAndTagRecords())
		{
			toArchetype.AddTypeData_WithoutCheck(
				baseTypeRecord.m_TypeID,
				baseTypeRecord.m_PackedContainer != nullptr ? baseTypeRecord.m_PackedContainer->CloneEmpty() : nullptr
			);
		}

		for (auto& baseFilterRecord : fromArchetype.GetFilters())
		{
			if (filterContainer != baseFilterRecord.m_FilterContainer)
			{
				toArchetype.AddFilter_WithoutCheckout(baseFilterRecord.m_FilterContainer);
			}
		}
	}

	void ArchetypesMap::AddTypeDataAfterAddFilter(const Archetype& baseArchetype, Archetype& toArchetype, IFilterContainerBase* filterContainer)
	{
		DECS_ASSERT(filterContainer != nullptr, "Filter must not be nullptr!");

		for (auto& baseTypeRecord : baseArchetype.GetComponentAndTagRecords())
		{
			toArchetype.AddTypeData_WithoutCheck(
				baseTypeRecord.m_TypeID,
				baseTypeRecord.m_PackedContainer != nullptr ? baseTypeRecord.m_PackedContainer->CloneEmpty() : nullptr
			);
		}


		bool bisNewFilterAdded = false;

		for (auto& baseFilterRecord : baseArchetype.GetFilters())
		{
			if (!bisNewFilterAdded && filterContainer->GetDataTypeID() < baseFilterRecord.m_FilterTypeID)
			{
				toArchetype.AddFilter_WithoutCheckout(filterContainer);
				bisNewFilterAdded = true;
			}
			toArchetype.AddFilter_WithoutCheckout(baseFilterRecord.m_FilterContainer);
		}

		if (!bisNewFilterAdded)
		{
			toArchetype.AddFilter_WithoutCheckout(filterContainer);
		}
	}

	Archetype* ArchetypesMap::GetOrCreateArchetypeAfterRemoveFilter(const Archetype& fromArchetype, TypeID filterTypeID)
	{
		IFilterContainerBase* archetypeFilter = fromArchetype.GetFilterContainer(filterTypeID);
		DECS_ASSERT(archetypeFilter != nullptr, "fromArchetype must have this filter type to remove it from!");

		if (fromArchetype.GetComponentTagFilterCount() == 1)
		{
			// there is only one filter in archetype without tags and components, so after remove this tag there will be empty archetype whichis handled by nullptr
			return nullptr;
		}

		auto edge = fromArchetype.GetEdge(ArchetypeDataKey(archetypeFilter));
		if (edge.IsValid())
		{
			DECS_ASSERT(edge.m_EdgeType == EArchetypeEdgeType::Remove, "It must be remove edge!");

			return edge.m_Archetype;
		}

		Archetype* newArchetype = m_ArchetypeAllocator.CreateArchetype();
		AddTypeDataAfterRemoveFilter(fromArchetype, *newArchetype, archetypeFilter);
		AddArchetypeToCorrectContainers(*newArchetype);

		return newArchetype;
	}

}