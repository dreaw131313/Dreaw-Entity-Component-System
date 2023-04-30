#pragma once
#include "Archetype.h"

namespace decs
{
	Archetype::Archetype()
	{
	}

	Archetype::~Archetype()
	{
		for (auto& data : m_TypeData)
		{
			delete data.m_PackedContainer;
		}
	}

	uint32_t Archetype::FindTypeIndex(TypeID typeID) const
	{
		if (m_ComponentsCount < Limits::MinComponentsInArchetypeToPerformMapLookup)
		{
			for (uint32_t i = 0; i < m_ComponentsCount; i++)
				if (m_TypeData[i].m_TypeID == typeID) return i;

			return Limits::MaxComponentCount;
		}

		auto it = m_TypeIDsIndexes.find(typeID);
		if (it == m_TypeIDsIndexes.end())
			return Limits::MaxComponentCount;

		return it->second;
	}

	void Archetype::AddTypeID(
		const TypeID& id,
		PackedContainerBase* frompackedContainer,
		ComponentContextBase* componentContext,
		StableContainerBase* stableContainer
	)
	{
		auto it = m_TypeIDsIndexes.find(id);
		if (it == m_TypeIDsIndexes.end())
		{
			m_ComponentsCount += 1;
			m_TypeIDsIndexes[id] = (uint32_t)m_TypeData.size();
			m_TypeData.emplace_back(
				id,
				frompackedContainer->CreateOwnEmptyCopy(),
				componentContext,
				stableContainer
			);
		}
	}

	void Archetype::AddEntityData(EntityData* entityData)
	{
		m_EntitiesData.emplace_back(entityData);

		entityData->m_Archetype = this;
		entityData->m_IndexInArchetype = m_EntitiesCount;

		m_EntitiesCount += 1;
	}

	void Archetype::RemoveSwapBackEntityData(uint64_t index)
	{
		if (index < (m_EntitiesCount - 1))
		{
			m_EntitiesData[index] = m_EntitiesData.back();
			m_EntitiesData[index].m_EntityData->m_IndexInArchetype = static_cast<uint32_t>(index);
		}
		m_EntitiesData.pop_back();
		m_EntitiesCount -= 1;
	}

	void Archetype::RemoveSwapBackEntity(uint64_t index)
	{
		if (m_EntitiesCount == 0) return;

		if (index == m_EntitiesCount - 1)
		{
			m_EntitiesData.pop_back();

			for (uint64_t i = 0; i < m_ComponentsCount; i++)
			{
				auto& typeData = m_TypeData[i];
				if (typeData.m_StableContainer != nullptr)
				{
					StableComponentRef* compRef = static_cast<StableComponentRef*>(typeData.m_PackedContainer->GetComponentDataAsVoid(index));
					typeData.m_StableContainer->Remove(compRef->m_ChunkIndex, compRef->m_Index);
				}

				typeData.m_PackedContainer->PopBack();
			}

			m_EntitiesCount -= 1;
		}
		else
		{
			m_EntitiesData[index] = m_EntitiesData.back();
			m_EntitiesData.pop_back();

			for (uint64_t i = 0; i < m_ComponentsCount; i++)
			{
				auto& typeData = m_TypeData[i];
				if (typeData.m_StableContainer != nullptr)
				{
					StableComponentRef* compRef = static_cast<StableComponentRef*>(typeData.m_PackedContainer->GetComponentDataAsVoid(index));
					typeData.m_StableContainer->Remove(compRef->m_ChunkIndex, compRef->m_Index);
				}

				typeData.m_PackedContainer->RemoveSwapBack(index);
			}

			m_EntitiesCount -= 1;
			m_EntitiesData[index].m_EntityData->m_IndexInArchetype = static_cast<uint32_t>(index);
		}
	}

	void Archetype::ReserveSpaceInArchetype(uint64_t desiredCapacity)
	{
		if (m_EntitiesData.capacity() < desiredCapacity)
		{
			m_EntitiesData.reserve(desiredCapacity);

			for (uint64_t idx = 0; idx < m_ComponentsCount; idx++)
			{
				m_TypeData[idx].m_PackedContainer->Reserve(desiredCapacity);
			}
		}
	}

	void Archetype::Reset()
	{
		m_EntitiesCount = 0;
		m_EntitiesData.clear();
		for (uint64_t idx = 0; idx < m_ComponentsCount; idx++)
		{
			m_TypeData[idx].m_PackedContainer->Clear();
		}
	}

	void Archetype::InitEmptyFromOther(Archetype& other, ComponentContextsManager* componentContexts, StableContainersManager* stableComponentsManager)
	{
		m_ComponentsCount = other.m_ComponentsCount;
		m_TypeData.reserve(m_ComponentsCount);
		m_AddingOrderTypeIDs.reserve(m_ComponentsCount);

		for (uint32_t i = 0; i < m_ComponentsCount; i++)
		{
			ArchetypeTypeData& otherTypeData = other.m_TypeData[i];
			otherTypeData.m_TypeID;
			m_TypeIDsIndexes[otherTypeData.m_TypeID] = i;

			m_TypeData.emplace_back(
				otherTypeData.m_TypeID,
				otherTypeData.m_PackedContainer->CreateOwnEmptyCopy(),
				componentContexts->GetComponentContext(otherTypeData.m_TypeID),
				stableComponentsManager->GetStableContainer(otherTypeData.m_TypeID)
			);

			m_AddingOrderTypeIDs.push_back(other.m_AddingOrderTypeIDs[i]);
		}
	}

	void Archetype::MoveEntityComponentsAfterRemoveComponent(
		TypeID removedComponentTypeID,
		Archetype* fromArchetype,
		uint64_t fromIndex,
		EntityData* entityData
	)
	{
		uint64_t thisArchetypeIndex = 0;
		uint64_t fromArchetypeIndex = 0;

		this->AddEntityData(entityData);

		for (; thisArchetypeIndex < m_ComponentsCount; thisArchetypeIndex++, fromArchetypeIndex++)
		{
			ArchetypeTypeData& thisTypeData = m_TypeData[thisArchetypeIndex];
			ArchetypeTypeData& fromArchetypeData = fromArchetype->m_TypeData[fromArchetypeIndex];
			if (fromArchetypeData.m_TypeID == removedComponentTypeID)
			{
				if (fromArchetypeData.m_StableContainer != nullptr)
				{
					StableComponentRef* componentRef = static_cast<StableComponentRef*>(fromArchetypeData.m_PackedContainer->GetComponentDataAsVoid(fromIndex));
					fromArchetypeData.m_StableContainer->Remove(componentRef->m_ChunkIndex, componentRef->m_Index);
				}

				fromArchetypeData.m_PackedContainer->RemoveSwapBack(fromIndex);

				fromArchetypeIndex += 1;
			}

			ArchetypeTypeData& updatetFromArchetypeData = fromArchetype->m_TypeData[fromArchetypeIndex];
			thisTypeData.m_PackedContainer->MoveEmplaceBackFromVoid(
				updatetFromArchetypeData.m_PackedContainer->GetComponentDataAsVoid(fromIndex)
			);
			updatetFromArchetypeData.m_PackedContainer->RemoveSwapBack(fromIndex);
		}

		fromArchetype->RemoveSwapBackEntityData(fromIndex);
	}

	void Archetype::ShrinkToFit()
	{
		m_EntitiesData.shrink_to_fit();
		for (uint64_t idx = 0; idx < m_ComponentsCount; idx++)
		{
			m_TypeData[idx].m_PackedContainer->ShrinkToFit();
		}
	}

}