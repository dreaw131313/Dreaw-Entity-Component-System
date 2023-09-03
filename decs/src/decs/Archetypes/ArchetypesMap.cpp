#pragma once
#include "ArchetypesMap.h"

namespace decs
{
	ArchetypesMap::ArchetypesMap(uint64_t archetypesVectorChunkSize, uint64_t archetypeGroupsVectorChunkSize) :
		m_Archetypes(archetypesVectorChunkSize),
		m_ArchetrypesGroupsByOneTypeVector(archetypeGroupsVectorChunkSize)
	{

	}

	ArchetypesMap::~ArchetypesMap()
	{
	}

	void ArchetypesMap::ShrinkArchetypesToFit()
	{
		if (ArchetypesCount() == 0)
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

	void ArchetypesMap::ShrinkArchetypesToFit(ArchetypesShrinkToFitState& state)
	{
		if (ArchetypesCount() == 0)
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
		}
	}

	void ArchetypesMap::MakeArchetypeEdges(Archetype& archetype)
	{
		// edges with archetypes with less components:
		{
			const uint64_t componentCountsMinusOne = archetype.ComponentCount() - 1;

			if (componentCountsMinusOne > 0)
			{
				const uint64_t archetypeListIndex = componentCountsMinusOne - 1;
				auto& archetypesListToCreateEdges = m_ArchetypesGroupedByComponentsCount[archetypeListIndex];

				uint64_t archCount = archetypesListToCreateEdges.size();
				for (uint64_t archIdx = 0; archIdx < archCount; archIdx++)
				{
					auto& testArchetype = *archetypesListToCreateEdges[archIdx];

					uint64_t incorrectTests = 0;
					TypeID notFindedType;
					bool isArchetypeValid = true;

					for (uint64_t typeIdx = 0; typeIdx < archetype.ComponentCount(); typeIdx++)
					{
						TypeID typeID = archetype.GetTypeID(typeIdx);

						auto it = testArchetype.m_TypeIDsIndexes.find(typeID);

						if (it == testArchetype.m_TypeIDsIndexes.end())
						{
							incorrectTests += 1;
							if (incorrectTests > 1)
							{
								isArchetypeValid = false;
								break;
							}
							else
							{
								notFindedType = typeID;
							}
						}
					}

					if (isArchetypeValid)
					{
						testArchetype.m_AddEdges[notFindedType] = &archetype;
						archetype.m_RemoveEdges[notFindedType] = &testArchetype;
					}
				}
			}
		}

		// edges with archetype with more components:
		{
			const uint64_t componentCountsPlusOne = (uint64_t)archetype.ComponentCount() + 1;

			if (componentCountsPlusOne <= m_ArchetypesGroupedByComponentsCount.size())
			{
				const uint64_t archetypeListIndex = archetype.ComponentCount();

				auto& archetypesListToCreateEdges = m_ArchetypesGroupedByComponentsCount[archetypeListIndex];
				uint64_t archCount = archetypesListToCreateEdges.size();

				for (uint64_t archIdx = 0; archIdx < archCount; archIdx++)
				{
					auto& testArchetype = *archetypesListToCreateEdges[archIdx];

					uint64_t incorrectTests = 0;
					TypeID lastIncorrectType;
					bool isArchetypeValid = true;

					for (uint64_t typeIdx = 0; typeIdx < testArchetype.ComponentCount(); typeIdx++)
					{
						TypeID typeID = testArchetype.GetTypeID(typeIdx);
						auto it = archetype.m_TypeIDsIndexes.find(typeID);
						if (it == archetype.m_TypeIDsIndexes.end())
						{
							incorrectTests += 1;
							if (incorrectTests > 1)
							{
								isArchetypeValid = false;
								break;
							}
							else
							{
								lastIncorrectType = typeID;
							}
						}
					}

					if (isArchetypeValid)
					{
						testArchetype.m_RemoveEdges[lastIncorrectType] = &archetype;
						archetype.m_AddEdges[lastIncorrectType] = &testArchetype;
					}
				}
			}
		}
	}

	void ArchetypesMap::AddArchetypeToCorrectContainers(Archetype& archetype, bool bTryAddToSingleComponentsMap)
	{
		if (bTryAddToSingleComponentsMap && archetype.ComponentCount() == 1)
		{
			m_SingleComponentArchetypes[archetype.GetTypeID(0)] = &archetype;
		}

		if (archetype.ComponentCount() > m_ArchetypesGroupedByComponentsCount.size())
		{
			m_ArchetypesGroupedByComponentsCount.resize(archetype.ComponentCount());
		}

		m_ArchetypesGroupedByComponentsCount[archetype.ComponentCount() - 1].push_back(&archetype);

		AddArchetypeToGroups(&archetype);
	}

	std::pair<Archetype*, bool> ArchetypesMap::FindMatchingArchetype(Archetype* archetypeToMatch)
	{
		const uint64_t typesCount = archetypeToMatch->ComponentCount();
		if (typesCount == 0) return { nullptr,false };

		Archetype* finalArchetype = GetSingleComponentArchetype(archetypeToMatch->GetTypeID(0));
		uint64_t typeIndex = 1;

		bool findedWithEdges = true;
		if (finalArchetype != nullptr)
		{
			while (typeIndex < typesCount)
			{
				auto it = finalArchetype->m_AddEdges.find(archetypeToMatch->GetTypeID(typeIndex));
				if (it != finalArchetype->m_AddEdges.end() && it->second != nullptr)
				{
					finalArchetype = it->second;
					typeIndex += 1;
				}
				else
				{
					finalArchetype = nullptr;
					findedWithEdges = false;
					break;
				}
			}
		}

		if (finalArchetype == nullptr)
		{
			auto it = m_ArchetypesGroupedByOneType.find(archetypeToMatch->GetTypeID(0));
			if (it != m_ArchetypesGroupedByOneType.end())
			{
				ArchetypesGroupByOneType* group = it->second;

				if (group->MaxComponentsCount() >= typesCount)
				{
					std::vector<Archetype*>& archetypes = group->GetArchetypesWithComponentsCount(typesCount);
					uint64_t archetypesSize = archetypes.size();
					auto& matchedArchData = archetypeToMatch->m_TypeData;
					for (uint64_t archIdx = 0; archIdx < archetypesSize; archIdx++)
					{
						finalArchetype = archetypes[archIdx];
						auto& typeData = finalArchetype->m_TypeData;

						for (uint64_t typeIdx = 0; typeIdx < typesCount; typeIdx++)
						{
							if (typeData[typeIdx].m_TypeID != matchedArchData[typeIdx].m_TypeID)
							{
								finalArchetype = nullptr;
								break;
							}
						}

						if (finalArchetype != nullptr)
						{
							break;
						}
					}
				}
			}
		}

		return { finalArchetype,findedWithEdges };
	}

	Archetype* ArchetypesMap::GetOrCreateMatchedArchetype(
		Archetype& fromArchetype,
		ComponentContextsManager* componentContextsManager,
		StableContainersManager* stableContainersManager,
		ObserversManager* observerManager
	)
	{
		auto pair = FindMatchingArchetype(&fromArchetype);
		Archetype* archetype = pair.first;

		if (archetype == nullptr)
		{
			archetype = &m_Archetypes.EmplaceBack();

			// take care that componentContextsManager have the same component contexts that component contextManager which have "fromArchetype" archetype and stableContainersManager have correct stable components containers
			for (uint64_t i = 0; i < fromArchetype.ComponentCount(); i++)
			{
				ArchetypeTypeData& fromArchetypeTypeData = fromArchetype.m_TypeData[i];

				auto& context = componentContextsManager->m_Contexts[fromArchetypeTypeData.m_TypeID];
				if (context == nullptr)
				{
					context = fromArchetypeTypeData.m_ComponentContext->CreateOwnEmptyCopy(observerManager);
				}

				if (fromArchetypeTypeData.m_StableContainer != nullptr)
				{
					StableContainerBase* stableContainer = stableContainersManager->GetStableContainer(fromArchetypeTypeData.m_TypeID);
					if (stableContainer == nullptr)
					{
						stableContainersManager->CreateStableContainerFromOther(fromArchetypeTypeData.m_StableContainer);
					}
				}
			}

			archetype->InitEmptyFromOther(fromArchetype, componentContextsManager, stableContainersManager);
			AddArchetypeToCorrectContainers(*archetype, true);

			/* {
				Archetype* archetypeBuffor = CreateSingleComponentArchetype(*archetype);
				for (uint64_t i = 1; i < archetype->ComponentCount() - 1; i++)
				{
					archetypeBuffor = CreateArchetypeAfterAddComponent(*archetypeBuffor, *archetype, i);
				}
				MakeArchetypeEdges(*archetype);
			}*/
		}
		/*else if (!pair.second)
		{
			Archetype* archetypeBuffor = CreateSingleComponentArchetype(*archetype);
			for (uint64_t i = 1; i < archetype->ComponentCount() - 1; i++)
			{
				archetypeBuffor = CreateArchetypeAfterAddComponent(*archetypeBuffor, *archetype, i);
			}
		}*/

		return archetype;
	}

	inline Archetype* ArchetypesMap::CreateArchetypeAfterAddComponent(Archetype& toArchetype, Archetype& archetypeToGetContainer, uint64_t addedComponentIndex)
	{
		TypeID addedComponentTypeID = archetypeToGetContainer.m_TypeData[addedComponentIndex].m_TypeID;
		uint64_t componentIndexToAdd = archetypeToGetContainer.FindTypeIndex(addedComponentTypeID);

		auto& edge = toArchetype.m_AddEdges[addedComponentTypeID];
		if (edge != nullptr)
		{
			return edge;
		}

		Archetype& newArchetype = m_Archetypes.EmplaceBack();
		bool isNewComponentTypeAdded = false;

		for (uint32_t i = 0; i < toArchetype.ComponentCount(); i++)
		{
			const ArchetypeTypeData& toTypeData = toArchetype.m_TypeData[i];
			TypeID currentTypeID = toTypeData.m_TypeID;
			if (!isNewComponentTypeAdded && currentTypeID > addedComponentTypeID)
			{
				ArchetypeTypeData& otherTypeData = archetypeToGetContainer.m_TypeData[componentIndexToAdd];
				isNewComponentTypeAdded = true;
				newArchetype.AddTypeID(
					addedComponentTypeID,
					otherTypeData.m_PackedContainer,
					otherTypeData.m_ComponentContext,
					otherTypeData.m_StableContainer
				);
			}

			newArchetype.AddTypeID(
				currentTypeID,
				toTypeData.m_PackedContainer,
				toTypeData.m_ComponentContext,
				toTypeData.m_StableContainer
			);
		}

		if (!isNewComponentTypeAdded)
		{
			ArchetypeTypeData& otherTypeData = archetypeToGetContainer.m_TypeData[componentIndexToAdd];
			newArchetype.AddTypeID(
				addedComponentTypeID,
				otherTypeData.m_PackedContainer,
				otherTypeData.m_ComponentContext,
				otherTypeData.m_StableContainer
			);
		}

		AddArchetypeToCorrectContainers(newArchetype);
		MakeArchetypeEdges(newArchetype);

		return &newArchetype;
	}

	Archetype* ArchetypesMap::GetArchetypeAfterRemoveComponent(Archetype& fromArchetype, TypeID removedComponentTypeID)
	{
		if (fromArchetype.ComponentCount() == 1 && fromArchetype.GetTypeID(0) == removedComponentTypeID)
		{
			return nullptr;
		}

		auto& edge = fromArchetype.m_RemoveEdges[removedComponentTypeID];
		if (edge != nullptr)
		{
			return edge;
		}

		Archetype& newArchetype = m_Archetypes.EmplaceBack();
		for (uint32_t i = 0; i < fromArchetype.ComponentCount(); i++)
		{
			const ArchetypeTypeData& fromArchetypeData = fromArchetype.m_TypeData[i];
			TypeID typeID = fromArchetypeData.m_TypeID;
			if (typeID != removedComponentTypeID)
			{
				newArchetype.AddTypeID(
					typeID,
					fromArchetypeData.m_PackedContainer,
					fromArchetypeData.m_ComponentContext,
					fromArchetypeData.m_StableContainer
				);
			}
		}

		AddArchetypeToCorrectContainers(newArchetype);
		MakeArchetypeEdges(newArchetype);

		return &newArchetype;
	}


}