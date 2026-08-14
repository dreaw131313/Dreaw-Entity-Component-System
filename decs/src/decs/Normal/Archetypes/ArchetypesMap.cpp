#pragma once
#include "ArchetypesMap.h"

#include <cassert>

namespace decs
{
	void ArchetypesGroupByOneType::AddArchetype(Archetype* archetype)
	{
		m_ArchetypesCount += 1;
		const uint64_t componentAndTagCount = archetype->GetComponentAndTagCount();

		if (componentAndTagCount == 1)
		{
			DECS_ASSERT(m_MainTypeID == archetype->GetTypeID(0), "Single component archetype must have component type same as m_MainTypeID!");
			m_MainTypeArchetype = archetype;
		}

		if (componentAndTagCount > m_Groups.size())
		{
			m_Groups.resize(componentAndTagCount);
		}

		auto& archetypeGroup = m_Groups[componentAndTagCount - 1];
		archetypeGroup.Archetypes.push_back(archetype);
	}

	ArchetypesMap::ArchetypesMap(uint64_t archetypesVectorChunkSize, uint64_t archetypeGroupsVectorChunkSize):
		m_Archetypes(archetypesVectorChunkSize),
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

		uint64_t chunksCount = m_Archetypes.ChunkCount();
		for (uint64_t chunkIdx = 0; chunkIdx < chunksCount; chunkIdx++)
		{
			uint64_t chunkSize = m_Archetypes.GetChunkSize(chunkIdx);
			Archetype* chunk = m_Archetypes.GetChunk(chunkIdx);

			for (uint64_t idx = 0; idx < chunkSize; idx++)
			{
				Archetype& archetype = chunk[idx];
				archetype.ShrinkToFit();
			}
		}
	}

	void ArchetypesMap::ShrinkArchetypesToFit(ArchetypesShrinkToFitState& state, const ArchetypesShrinkToFitConfig& config)
	{
		size_t currentArchetypeIndex = state.m_LastArchetypeIndex;

		if (currentArchetypeIndex >= m_Archetypes.Size())
		{
			currentArchetypeIndex = 0;
		}

		size_t checkedArchetypeCount = 0;
		size_t shrinkedArchetypeCount = 0;

		auto keepShrinking = [&] () -> bool
		{
			return checkedArchetypeCount < config.m_MaxArchetypeCountToCheck && shrinkedArchetypeCount < config.m_MaxArchetypesToShrink;
		};

		while (keepShrinking())
		{
			Archetype& archetype = m_Archetypes[currentArchetypeIndex];
			currentArchetypeIndex = (currentArchetypeIndex + 1) % m_Archetypes.Size();
			checkedArchetypeCount++;

			if (archetype.GetLoadFactor() <= config.m_MinArchetypeLoadFactor)
			{
				archetype.ShrinkToFit();
				shrinkedArchetypeCount++;
			}
		}

		state.m_LastArchetypeIndex = currentArchetypeIndex;
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
		const uint32_t typeCount = archetype.GetComponentTagCount();
		const uint32_t addTypeNeighbourTypeCount = typeCount + 1;
		const uint32_t removeTypeNeighbourTypeCount = typeCount - 1;

		const ArchetypeGroup* bestAddTypeGroup = nullptr;
		uint32_t bestAddTypeArchetypeCount = std::numeric_limits<uint32_t>::max();

		for (uint32_t typeIdx = 0; typeIdx < typeCount; typeIdx++)
		{
			const ArchetypesGroupByOneType* typeGroup = GetArchetypesGroupWithoutCreating(archetype.GetTypeID(typeIdx));
			DECS_ASSERT(typeGroup != nullptr, "This should never fail, because archetype is added to all its types groups:");

			const ArchetypeGroup* currentAddTypeGroup = typeGroup->GetGroupWithTypeCount(static_cast<uint64_t>(addTypeNeighbourTypeCount));
			if (currentAddTypeGroup != nullptr && currentAddTypeGroup->GetArchetypeCount() < bestAddTypeArchetypeCount)
			{
				bestAddTypeGroup = currentAddTypeGroup;
				bestAddTypeArchetypeCount = static_cast<uint32_t>(currentAddTypeGroup->GetArchetypeCount());
			}

			if (typeCount > 1)
			{
				/*
				* This potenitaly is faster than checking all existing archetypes with smaller number of components
				*/
				const ArchetypeGroup* removeComponentGroup = typeGroup->GetGroupWithTypeCount(static_cast<uint64_t>(removeTypeNeighbourTypeCount));
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
		AddArchetypeToGroups(&archetype);

		m_HashedArchetypes[ArchetypeHasher(&archetype)] = &archetype;

		if (archetype.GetComponentTagCount() > m_MaxComponentTagCount)
		{
			m_MaxComponentTagCount = archetype.GetComponentTagCount();
		}

		MakeArchetypeEdges_4(archetype);
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

	Archetype* ArchetypesMap::GetOrCreateMatchedArchetype(
		Archetype& fromArchetype,
		ComponentContextsManager* componentContextsManager
	)
	{
		Archetype* archetype = FindMatchingArchetype(fromArchetype);

		if (archetype == nullptr)
		{
			archetype = &m_Archetypes.EmplaceBack();

			// take care that componentContextsManager have the same component contexts that component contextManager which have "fromArchetype" archetype and stableContainersManager have correct stable components containers
			for (uint64_t i = 0; i < fromArchetype.GetComponentAndTagCount(); i++)
			{
				ArchetypeTypeData& fromArchetypeTypeData = fromArchetype.m_TypeData[i];
				if (!fromArchetypeTypeData.IsTag())
				{
					componentContextsManager->GetOrCreateComponentContextFromOtherContext(fromArchetypeTypeData.m_ComponentContext);
				}
			}

			archetype->InitEmptyFromOther(fromArchetype, componentContextsManager);
			AddArchetypeToCorrectContainers(*archetype);
		}

		return archetype;
	}

	Archetype* ArchetypesMap::CreateSingleComponentArchetype(TypeID componentTypeID, IComponentContext* componentContext)
	{
		auto archetype = GetSingleComponentArchetype(componentTypeID);
		if (archetype != nullptr)
		{
			return archetype;
		}
		archetype = &m_Archetypes.EmplaceBack();
		archetype->AddTypeData_WithoutCheck(componentTypeID, componentContext);
		AddArchetypeToCorrectContainers(*archetype);
		return archetype;
	}

	Archetype* ArchetypesMap::GetOrCreateArchetypeAfterAddComponent(const Archetype& toArchetype, TypeID componentTypeID, IComponentContext* componentContext)
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

		if (toArchetype.ContainType(componentTypeID))
		{
			assert(false);
			return nullptr;
		}

		Archetype& newArchetype = m_Archetypes.EmplaceBack();
		AddTypeDataAfterAddComponent(toArchetype, newArchetype, componentTypeID, componentContext);

		AddArchetypeToCorrectContainers(newArchetype);

		return &newArchetype;
	}

	Archetype* ArchetypesMap::CreateArchetypeAfterAddComponent(const Archetype& toArchetype, TypeID componentTypeID, IComponentContext* componentContext)
	{
		Archetype& newArchetype = m_Archetypes.EmplaceBack();
		AddTypeDataAfterAddComponent(toArchetype, newArchetype, componentTypeID, componentContext);

		AddArchetypeToCorrectContainers(newArchetype);

		return &newArchetype;
	}

	Archetype* ArchetypesMap::GetOrCreateArchetypeAfterRemoveComponent(const Archetype& fromArchetype, TypeID removedComponentTypeID)
	{
		if (fromArchetype.GetComponentAndTagCount() == 1 && fromArchetype.GetTypeID(0) == removedComponentTypeID)
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

		// next check if we have this component in archetype if no we must not create new archetype
		if (!fromArchetype.ContainType(removedComponentTypeID))
		{
			// this is error:
			assert(false);
			return nullptr;
		}

		Archetype& newArchetype = m_Archetypes.EmplaceBack();
		AddTypeDataAfterRemoveComponent(fromArchetype, newArchetype, removedComponentTypeID);
		AddArchetypeToCorrectContainers(newArchetype);

		return &newArchetype;
	}

	Archetype* ArchetypesMap::GetArchetypeAfterAddTag(const Archetype& toArchetype, TypeID tagType)
	{
		return GetOrCreateArchetypeAfterAddComponent(toArchetype, tagType, nullptr);
	}

	Archetype* ArchetypesMap::GetArchetypeAfterRemoveTag(const Archetype& fromArchetype, TypeID tagType)
	{
		return GetOrCreateArchetypeAfterRemoveComponent(fromArchetype, tagType);
	}

	Archetype* ArchetypesMap::CreateSingleTagArchetype(TypeID componentTypeID)
	{
		auto archetype = GetSingleComponentArchetype(componentTypeID);
		if (archetype != nullptr)
		{
			return archetype;
		}
		archetype = &m_Archetypes.EmplaceBack();
		archetype->AddTypeData_WithoutCheck(componentTypeID, nullptr);
		AddArchetypeToCorrectContainers(*archetype);

		return archetype;
	}

	void ArchetypesMap::AddTypeDataAfterRemoveComponent(const Archetype& fromArchetype, Archetype& toArchetype, TypeID compType)
	{
		for (uint32_t i = 0; i < fromArchetype.GetComponentAndTagCount(); i++)
		{
			const ArchetypeTypeData& fromArchetypeData = fromArchetype.m_TypeData[i];
			if (fromArchetypeData.m_TypeID != compType)
			{
				if (fromArchetypeData.IsTag())
				{
					toArchetype.AddTypeData_WithoutCheck(fromArchetypeData.m_TypeID, nullptr);
				}
				else
				{
					toArchetype.AddTypeData_WithoutCheck(
						fromArchetypeData.m_TypeID,
						fromArchetypeData.m_ComponentContext
					);
				}
			}
		}
	}

	void ArchetypesMap::AddTypeDataAfterAddComponent(const Archetype& baseArchetype, Archetype& toArchetype, TypeID componentTypeID, IComponentContext* addedComponentContext)
	{
		bool isNewComponentTypeAdded = false;

		for (uint32_t i = 0; i < baseArchetype.GetComponentAndTagCount(); i++)
		{
			const ArchetypeTypeData& baseTypeData = baseArchetype.m_TypeData[i];

			if (!isNewComponentTypeAdded && baseTypeData.m_TypeID > componentTypeID)
			{
				isNewComponentTypeAdded = true;
				toArchetype.AddTypeData_WithoutCheck(
					componentTypeID,
					addedComponentContext
				);
			}

			if (baseTypeData.IsTag())
			{
				toArchetype.AddTypeData_WithoutCheck(
					baseTypeData.m_TypeID,
					nullptr
				);
			}
			else
			{
				toArchetype.AddTypeData_WithoutCheck(
					baseTypeData.m_TypeID,
					baseTypeData.m_ComponentContext
				);
			}
		}

		if (!isNewComponentTypeAdded)
		{
			toArchetype.AddTypeData_WithoutCheck(
				componentTypeID,
				addedComponentContext
			);
		}
	}


}