#pragma once
#include "LArchetypesMap.h"

#include <cassert>

namespace decs::light
{
	ArchetypesMap::ArchetypesMap(uint64_t archetypesVectorChunkSize, uint64_t archetypeGroupsVectorChunkSize):
		m_Archetypes(archetypesVectorChunkSize),
		m_ArchetrypesGroupsByOneTypeAllocator(archetypeGroupsVectorChunkSize)
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

	void ArchetypesMap::ShrinkArchetypesToFit(ArchetypesShrinkToFitState& state)
	{
		if (GetArchetypesCount() == 0)
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

	void ArchetypesMap::ClearEntityDataAndComponents()
	{
		IterateOverArchetypes([](Archetype* arch)
		{
			arch->ClearEntityDataAndComponents();
		});
	}

	void ArchetypesMap::MakeArchetypeEdges_4(Archetype& archetype)
	{
		const uint32_t typeCount = archetype.GetTypeCount();
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
				bestAddTypeArchetypeCount = currentAddTypeGroup->GetArchetypeCount();
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
						auto edgeTypeID = archetype.IsRemoveComponentNeighbour(*neighbour);
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
				if (auto edgeTypeID = archetype.IsAddComponentNeighbour(*neighbour))
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

		if (archetype.GetTypeCount() > m_MaxTypeCountInArchetypes)
		{
			m_MaxTypeCountInArchetypes = archetype.GetTypeCount();
		}

		MakeArchetypeEdges_4(archetype);
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

	Archetype* ArchetypesMap::GetOrCreateMatchedArchetype(Archetype& fromArchetype)
	{
		Archetype* archetype = FindMatchingArchetype(fromArchetype);

		if (archetype == nullptr)
		{
			archetype = &m_Archetypes.EmplaceBack();
			archetype->InitEmptyFromOther(fromArchetype);
			AddArchetypeToCorrectContainers(*archetype);
		}

		return archetype;
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

		if (toArchetype.ContainType(componentTypeID))
		{
			assert(false);
			return nullptr;
		}

		Archetype& newArchetype = m_Archetypes.EmplaceBack();
		AddTypeDataAfterAddComponent(toArchetype, newArchetype, componentTypeID, packedContainer);

		AddArchetypeToCorrectContainers(newArchetype);

		return &newArchetype;
	}

	Archetype* ArchetypesMap::GetArchetypeAfterRemoveComponent(const Archetype& fromArchetype, TypeID removedComponentTypeID)
	{
		if (fromArchetype.GetTypeCount() == 1 && fromArchetype.GetTypeID(0) == removedComponentTypeID)
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

		return archetype;
	}

	void ArchetypesMap::AddTypeDataAfterRemoveComponent(const Archetype& fromArchetype, Archetype& toArchetype, TypeID removedComponentType)
	{
		for (uint32_t i = 0; i < fromArchetype.GetTypeCount(); i++)
		{
			const ArchetypeTypeData& fromArchetypeData = fromArchetype.m_TypeData[i];
			if (fromArchetypeData.m_TypeID != removedComponentType)
			{
				if (fromArchetypeData.IsTag())
				{
					toArchetype.AddTypeData_WithoutCheck(fromArchetypeData.m_TypeID, nullptr);
				}
				else
				{
					toArchetype.AddTypeData_WithoutCheck(
						fromArchetypeData.m_TypeID,
						fromArchetypeData.m_PackedContainer->CloneEmpty()
					);
				}
			}
		}
	}

	void ArchetypesMap::AddTypeDataAfterAddComponent(const Archetype& baseArchetype, Archetype& toArchetype, TypeID componentTypeID, IPackedLightComponentContainer* packedContainer)
	{
		bool isNewComponentTypeAdded = false;

		for (uint32_t i = 0; i < baseArchetype.GetTypeCount(); i++)
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
					baseTypeData.m_PackedContainer->CloneEmpty()
				);
			}
		}

		if (!isNewComponentTypeAdded)
		{
			toArchetype.AddTypeData_WithoutCheck(
				componentTypeID,
				packedContainer
			);
		}
	}


}