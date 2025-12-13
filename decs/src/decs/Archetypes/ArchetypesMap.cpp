#pragma once
#include "ArchetypesMap.h"

#include <cassert>

namespace decs
{
	ArchetypesMap::ArchetypesMap(uint64_t archetypesVectorChunkSize, uint64_t archetypeGroupsVectorChunkSize):
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

	void ArchetypesMap::FullClear()
	{
		m_Archetypes.Clear();
		m_ArchetypesGroupedByComponentsCount.clear();
		m_ArchetrypesGroupsByOneTypeVector.Clear();
		m_ArchetypesGroupedByOneType.clear();
	}

	void ArchetypesMap::ClearEntityDataAndComponents()
	{
		IterateOverArchetypes([](Archetype* arch)
		{
			arch->ClearEntityDataAndComponents();
		});
	}

	void ArchetypesMap::MakeArchetypeEdges(Archetype& archetype)
	{
		// edges with archetypes with less components:
		{
			const uint64_t componentCountsMinusOne = archetype.GetComponentAndTagCount() - 1;

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

					for (uint64_t typeIdx = 0; typeIdx < archetype.GetComponentAndTagCount(); typeIdx++)
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
						testArchetype.AddEdge(notFindedType, &archetype, EComponentEdgeType::Add);
						archetype.AddEdge(notFindedType, &testArchetype, EComponentEdgeType::Remove);
					}
				}
			}
		}

		// edges with archetype with more components:
		{
			const uint64_t componentCountsPlusOne = (uint64_t)archetype.GetComponentAndTagCount() + 1;

			if (componentCountsPlusOne <= m_ArchetypesGroupedByComponentsCount.size())
			{
				const uint64_t archetypeListIndex = archetype.GetComponentAndTagCount();

				auto& archetypesListToCreateEdges = m_ArchetypesGroupedByComponentsCount[archetypeListIndex];
				uint64_t archCount = archetypesListToCreateEdges.size();

				for (uint64_t archIdx = 0; archIdx < archCount; archIdx++)
				{
					auto& testArchetype = *archetypesListToCreateEdges[archIdx];

					uint64_t incorrectTests = 0;
					TypeID lastIncorrectType = std::numeric_limits<TypeID>::max();
					bool isArchetypeValid = true;

					for (uint64_t typeIdx = 0; typeIdx < testArchetype.GetComponentAndTagCount(); typeIdx++)
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
						testArchetype.AddEdge(lastIncorrectType, &archetype, EComponentEdgeType::Remove);
						archetype.AddEdge(lastIncorrectType, &testArchetype, EComponentEdgeType::Add);
					}
				}
			}
		}
	}

	void ArchetypesMap::AddArchetypeToCorrectContainers(Archetype& archetype)
	{
		if (archetype.GetComponentAndTagCount() > m_ArchetypesGroupedByComponentsCount.size())
		{
			m_ArchetypesGroupedByComponentsCount.resize(archetype.GetComponentAndTagCount());
		}

		m_ArchetypesGroupedByComponentsCount[archetype.GetComponentAndTagCount() - 1].push_back(&archetype);

		AddArchetypeToGroups(&archetype);

		m_HashedArchetypes[ArchetypeHasher(&archetype)] = &archetype;

		if (archetype.GetTypeCount() > m_MaxTypeCountInArchetypes)
		{
			m_MaxTypeCountInArchetypes = archetype.GetTypeCount();
		}
	}

	Archetype* ArchetypesMap::FindMatchingArchetype(const Archetype& toArchetype)
	{
		const uint64_t typesCount = toArchetype.GetTypeCount();
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

	Archetype* ArchetypesMap::CreateSingleComponentArchetype(TypeID componentTypeID, ComponentContextBase* componentContext)
	{
		auto archetype = GetSingleComponentArchetype(componentTypeID);
		if (archetype != nullptr)
		{
			return archetype;
		}
		archetype = &m_Archetypes.EmplaceBack();
		archetype->AddTypeData_WithoutCheck(componentTypeID, componentContext);
		AddArchetypeToCorrectContainers(*archetype);
		MakeArchetypeEdges(*archetype);
		return archetype;
	}

	Archetype* ArchetypesMap::CreateArchetypeAfterAddComponent(const Archetype& toArchetype, TypeID componentTypeID, ComponentContextBase* componentContext)
	{
		//auto& edge = toArchetype.m_AddEdges[addedComponentTypeID];
		auto edge = toArchetype.GetEdge(componentTypeID);
		if (edge.IsValid())
		{
			if (edge.m_EdgeType == EComponentEdgeType::Add)
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
		MakeArchetypeEdges(newArchetype);

		return &newArchetype;
	}

	Archetype* ArchetypesMap::GetArchetypeAfterRemoveComponent(const Archetype& fromArchetype, TypeID removedComponentTypeID)
	{
		if (fromArchetype.GetComponentAndTagCount() == 1 && fromArchetype.GetTypeID(0) == removedComponentTypeID)
		{
			return nullptr;
		}

		// first we check if we have edge it will fail only once
		auto edge = fromArchetype.GetEdge(removedComponentTypeID);
		if (edge.IsValid())
		{
			if (edge.m_EdgeType == EComponentEdgeType::Remove)
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
		MakeArchetypeEdges(newArchetype);

		return &newArchetype;
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
		archetype = &m_Archetypes.EmplaceBack();
		archetype->AddTypeData_WithoutCheck(componentTypeID, nullptr);
		AddArchetypeToCorrectContainers(*archetype);
		MakeArchetypeEdges(*archetype);
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

	void ArchetypesMap::AddTypeDataAfterAddComponent(const Archetype& baseArchetype, Archetype& toArchetype, TypeID componentTypeID, ComponentContextBase* addedComponentContext)
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