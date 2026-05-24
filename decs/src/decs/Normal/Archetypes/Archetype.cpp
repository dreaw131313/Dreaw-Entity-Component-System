#pragma once
#include "Archetype.h"

#include <algorithm>

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

	bool Archetype::ContainType(TypeID typeID) const
	{
		if (GetComponentTagCount() <= Limits::MinComponentsInArchetypeToPerformMapLookup)
		{
			for (auto& typeData : m_TypeData)
			{
				if (typeData.m_TypeID == typeID)
				{
					return true;
				}
			}
			return false;
		}

		return m_TypeIDsIndexes.contains(typeID);
	}

	uint32_t Archetype::FindTypeIndex(TypeID typeID) const
	{
		if (GetComponentAndTagCount() < Limits::MinComponentsInArchetypeToPerformMapLookup)
		{
			for (uint32_t i = 0; i < GetComponentAndTagCount(); i++)
				if (m_TypeData[i].m_TypeID == typeID) return i;

			return std::numeric_limits<uint32_t>::max();
		}

		auto it = m_TypeIDsIndexes.find(typeID);
		if (it == m_TypeIDsIndexes.end())
			return std::numeric_limits<uint32_t>::max();

		return it->second;
	}

	bool Archetype::HasSameComponentsTagsAs(const Archetype& archetype) const
	{
		const uint32_t componentAndTagCount = GetComponentAndTagCount();

		if (archetype.GetComponentAndTagCount() != componentAndTagCount)
		{
			return false;
		}

		for (uint32_t i = 0; i < componentAndTagCount; i++)
		{
			if (GetTypeID(i) != archetype.GetTypeID(i))
			{
				return false;
			}
		}

		return true;
	}

	bool Archetype::HasTypes_Exactly(const ecsVector<TypeID>& types) const
	{
		const uint32_t componentAndTagCount = GetComponentAndTagCount();

		if (static_cast<uint32_t>(types.size()) != componentAndTagCount)
		{
			return false;
		}

		for (uint32_t i = 0; i < componentAndTagCount; i++)
		{
			if (!ContainType(types[i]))
			{
				return false;
			}
		}

		return true;
	}

	std::optional<TypeID> Archetype::IsRemoveAnyDataNeighbour(const Archetype& neighbour) const
	{
		const uint32_t typeCount = GetComponentTagCount();
		const uint32_t neighbourTypeCount = neighbour.GetComponentTagCount();

		if (neighbourTypeCount >= typeCount || (typeCount - neighbourTypeCount) != 1)
		{
			return {};
		}

		TypeID neighbourTypeID = decs::InvalidTypeID;
		uint32_t foundedNeighbourTypeCount = 0;

		for (uint32_t typeIdx = 0; typeIdx < typeCount; typeIdx++)
		{
			const TypeID currentTypeID = GetTypeID(typeIdx);

			if (!neighbour.ContainType(currentTypeID))
			{
				neighbourTypeID = currentTypeID;
				foundedNeighbourTypeCount++;
			}
			if (foundedNeighbourTypeCount > 1)
			{
				return {};
			}
		}

		return std::optional<TypeID>(neighbourTypeID);
	}

	std::optional<TypeID> Archetype::IsAddAnyDataNeighbour(const Archetype& neighbour) const
	{
		return neighbour.IsRemoveAnyDataNeighbour(*this);
	}

	void Archetype::ClearEntityDataAndComponents()
	{
		m_EntityStorage.Clear();
		for (uint32_t i = 0; i < m_TypeData.size(); i++)
		{
			auto& typeData = m_TypeData[i];
			if (!typeData.IsTag())
			{
				typeData.m_PackedContainer->Clear();
			}
		}
	}

	void Archetype::InsertComponentContextInCorrectPlace(IComponentContext* componentContext, uint32_t typeDataIndex)
	{
		int32_t contextCount = static_cast<int32_t>(m_ComponentContextsInOrder.size());
		int32_t contextCountMinusOne = contextCount - 1;
		for (int32_t i = contextCountMinusOne; i >= 0; i--)
		{
			auto& orderData = m_ComponentContextsInOrder[i];
			if (orderData.m_ComponentContext->GetObserverOrder() <= componentContext->GetObserverOrder())
			{
				if (i < contextCountMinusOne)
				{
					// insert on i+1 place
					auto insertPos = m_ComponentContextsInOrder.begin();
					std::advance(insertPos, i + 1);
					m_ComponentContextsInOrder.insert(insertPos, { componentContext, typeDataIndex });
				}
				else
				{
					// pushback
					m_ComponentContextsInOrder.push_back({ componentContext, typeDataIndex });
				}
				return;
			}
		}
		m_ComponentContextsInOrder.insert(m_ComponentContextsInOrder.begin(), { componentContext, typeDataIndex });
	}

	void Archetype::AddTypeData_WithoutCheck(TypeID typeID, IComponentContext* componentContext)
	{
		const uint32_t typeIndex = static_cast<uint32_t>(m_TypeData.size());
		m_TypeIDsIndexes[typeID] = typeIndex;
		if (componentContext == nullptr)
		{
			// tag data
			m_TypeData.emplace_back(typeID, nullptr, nullptr, nullptr);
		}
		else
		{
			m_TypeData.emplace_back(typeID, componentContext->CreatePackedContainer(), componentContext, componentContext->GetStableContainer());
			InsertComponentContextInCorrectPlace(componentContext, typeIndex);
		}
	}

	void Archetype::UpdateOrderOfComponentContexts()
	{
		static auto sortLambda = [](OrderData& lhs, OrderData& rhs)
		{
			if (lhs.m_ComponentContext->GetObserverOrder() < rhs.m_ComponentContext->GetObserverOrder())
			{
				return true;
			}
			return false;
		};

		std::sort(m_ComponentContextsInOrder.begin(), m_ComponentContextsInOrder.end(), sortLambda);
	}

	void Archetype::AddEntityData(EntityData* entityData)
	{
		if (entityData == nullptr)
		{
			return;
		}

		m_EntityStorage.PushBack_UpdateEntityIndex(entityData, entityData->IsActive());
		entityData->m_Archetype = this;
	}

	void Archetype::RemoveSwapBackEntityData(uint64_t index)
	{
		m_EntityStorage.RemoveSwapBack(index, true);
	}

	void Archetype::RemoveSwapBackEntity(uint64_t index)
	{
		if (index >= EntityCount())
		{
			return;
		}

		if (index == EntityCount() - 1)
		{
			for (uint64_t i = 0; i < GetComponentAndTagCount(); i++)
			{
				auto& typeData = m_TypeData[i];
				if (!typeData.IsTag())
				{
					EntityComponent* componentPtr = typeData.m_PackedContainer->GetComponentBasePtr(index);
					typeData.m_PackedContainer->PopBack();
					typeData.m_StableContainer->Destroy(componentPtr);
				}
			}

		}
		else
		{
			for (uint64_t i = 0; i < GetComponentAndTagCount(); i++)
			{
				auto& typeData = m_TypeData[i];
				if (!typeData.IsTag())
				{
					EntityComponent* componentPtr = typeData.m_PackedContainer->GetComponentBasePtr(index);
					typeData.m_PackedContainer->RemoveSwapBack(index);
					typeData.m_StableContainer->Destroy(componentPtr);
				}
			}
		}

		RemoveSwapBackEntityData(index);

	}

	void Archetype::RemoveSwapBackEntityAfterRemoveComponent(uint64_t index)
	{
		if (index >= EntityCount())
		{
			return;
		}

		if (index == EntityCount() - 1)
		{
			for (uint64_t i = 0; i < GetComponentAndTagCount(); i++)
			{
				auto& typeData = m_TypeData[i];
				if (!typeData.IsTag())
				{
					EntityComponent* componentPtr = typeData.m_PackedContainer->GetComponentBasePtr(index);
					typeData.m_PackedContainer->PopBack();
				}
			}

		}
		else
		{
			for (uint64_t i = 0; i < GetComponentAndTagCount(); i++)
			{
				auto& typeData = m_TypeData[i];
				if (!typeData.IsTag())
				{
					EntityComponent* componentPtr = typeData.m_PackedContainer->GetComponentBasePtr(index);
					typeData.m_PackedContainer->RemoveSwapBack(index);
				}
			}
		}

		RemoveSwapBackEntityData(index);
	}

	void Archetype::RemoveSwapBackRecordRaw(uint64_t index)
	{
		if (index >= EntityCount())
		{
			return;
		}

		if (index == EntityCount() - 1)
		{
			m_EntityStorage.PopBack();
			for (uint64_t i = 0; i < GetComponentAndTagCount(); i++)
			{
				auto& typeData = m_TypeData[i];
				if (!typeData.IsTag())
				{
					typeData.m_PackedContainer->PopBack();
				}
			}
		}
		else
		{
			m_EntityStorage.RemoveSwapBack(static_cast<size_t>(index), true);
			auto backEntityRecord = m_EntityStorage.GetBackRecord();
			if (backEntityRecord.first != nullptr)
			{
				backEntityRecord.first->m_IndexInArchetype = static_cast<uint32_t>(index);
			}
			m_EntityStorage.SetEntityRecord(index, backEntityRecord.first, backEntityRecord.second);
			m_EntityStorage.PopBack();


			for (uint64_t i = 0; i < GetComponentAndTagCount(); i++)
			{
				auto& typeData = m_TypeData[i];
				if (!typeData.IsTag())
				{
					typeData.m_PackedContainer->RemoveSwapBack(index);
				}
			}
		}

	}

	void Archetype::SetRecordAsIntendedToDelayedDestroy(uint64_t index)
	{
		if (index >= EntityCount())
		{
			return;
		}
		m_EntityStorage.InvalidateRecord(static_cast<size_t>(index));
	}

	void Archetype::ReserveSpaceInArchetype(uint64_t desiredCapacity)
	{
		if (m_EntityStorage.GetCapacity() < desiredCapacity)
		{
			m_EntityStorage.Reserve(desiredCapacity);

			for (uint64_t idx = 0; idx < GetComponentAndTagCount(); idx++)
			{
				auto& typeData = m_TypeData[idx];
				if (!typeData.IsTag())
				{
					typeData.m_PackedContainer->Reserve(desiredCapacity);
				}
			}
		}
	}

	void Archetype::Reset()
	{
		m_EntityStorage.Clear();
		for (uint64_t idx = 0; idx < GetComponentAndTagCount(); idx++)
		{
			auto& typeData = m_TypeData[idx];
			if (!typeData.IsTag())
			{
				typeData.m_PackedContainer->Clear();
			}
		}
	}

	void Archetype::InitEmptyFromOther(const Archetype& other, ComponentContextsManager* componentContexts)
	{
		uint32_t componentsCount = other.GetComponentAndTagCount();
		m_TypeData.reserve(componentsCount);

		for (uint32_t i = 0; i < componentsCount; i++)
		{
			const ArchetypeTypeData& otherTypeData = other.m_TypeData[i];
			otherTypeData.m_TypeID;
			m_TypeIDsIndexes[otherTypeData.m_TypeID] = i;

			if (otherTypeData.IsTag())
			{
				AddTypeData_WithoutCheck(
					otherTypeData.m_TypeID,
					nullptr
				);
			}
			else
			{
				AddTypeData_WithoutCheck(
					otherTypeData.m_TypeID,
					componentContexts->GetComponentContext(otherTypeData.m_TypeID)
				);
			}
		}
	}

	void Archetype::RemoveSwapBackEntityAfterMoveEntityWithoutDestroyingSource(uint64_t entityIndex, TypeID removedComponentTypeID)
	{
		if (entityIndex >= EntityCount())
		{
			return;
		}

		const uint32_t componentCount = GetComponentAndTagCount();
		if (entityIndex == EntityCount() - 1)
		{
			for (uint64_t i = 0; i < componentCount; i++)
			{
				auto& typeData = m_TypeData[i];
				if (!typeData.IsTag())
				{
					if (removedComponentTypeID == typeData.m_TypeID)
					{
						EntityComponent* componentPtr = typeData.m_PackedContainer->GetComponentBasePtr(entityIndex);
						typeData.m_StableContainer->Destroy(componentPtr);
					}

					typeData.m_PackedContainer->PopBack();
				}
			}
		}
		else
		{
			for (uint64_t i = 0; i < componentCount; i++)
			{
				auto& typeData = m_TypeData[i];
				if (!typeData.IsTag())
				{
					if (removedComponentTypeID == typeData.m_TypeID)
					{
						EntityComponent* componentPtr = typeData.m_PackedContainer->GetComponentBasePtr(entityIndex);
						typeData.m_StableContainer->Destroy(componentPtr);
					}

					typeData.m_PackedContainer->RemoveSwapBack(entityIndex);
				}
			}
		}

		RemoveSwapBackEntityData(entityIndex);

	}

	void Archetype::ShrinkToFit()
	{
		m_EntityStorage.ShrinkToFit();
		for (uint64_t idx = 0; idx < GetComponentAndTagCount(); idx++)
		{
			auto& typeData = m_TypeData[idx];
			if (typeData.IsTag())
			{
				continue;
			}
			typeData.m_PackedContainer->ShrinkToFit();
		}
	}

	void Archetype::AddEdge(TypeID componentTypeID, Archetype* archetype, EArchetypeEdgeType edgeType)
	{
		auto& edge = m_Edges[componentTypeID];
		if (!edge.IsValid())
		{
			edge.m_Archetype = archetype;
			edge.m_EdgeType = edgeType;
		}
	}

	bool Archetype::MoveEntityComponentsAfterAddComponent(
		Archetype& fromArchetype,
		Archetype& toArchetype,
		uint64_t entityIndex,
		TypeID addedComponentTypeID
	)
	{
		if (entityIndex >= fromArchetype.EntityCount())
		{
			return false;
		}

		EntityData* entityData = fromArchetype.m_EntityStorage.GetEntity(entityIndex);
		DECS_ASSERT(entityData != nullptr, "EntityData must be valid pointer!");

		toArchetype.AddEntityData(entityData);

		uint64_t thisArchetypeIndex = 0;
		uint64_t fromArchetypeIndex = 0;

		for (; thisArchetypeIndex < toArchetype.GetComponentAndTagCount(); thisArchetypeIndex++)
		{
			ArchetypeTypeData& thisTypeData = toArchetype.m_TypeData[thisArchetypeIndex];
			if (thisTypeData.m_TypeID == addedComponentTypeID)
			{
				continue;
			}

			ArchetypeTypeData& fromArchetypeData = fromArchetype.m_TypeData[fromArchetypeIndex];
			if (!fromArchetypeData.IsTag())
			{
				thisTypeData.m_PackedContainer->PushBackFromBase(fromArchetypeData.m_PackedContainer->GetComponentBasePtr(entityIndex));
				fromArchetypeData.m_PackedContainer->RemoveSwapBack(entityIndex);
			}

			fromArchetypeIndex++;
		}

		fromArchetype.RemoveSwapBackEntityData(entityIndex);

		return true;
	}

	bool Archetype::MoveEntityAfterAddComponentWithoutDestroyingFromSource(
		Archetype& fromArchetype,
		Archetype& toArchetype,
		uint64_t entityIndex,
		TypeID addedComponentTypeID
	)
	{
		if (entityIndex >= fromArchetype.EntityCount())
		{
			return false;
		}


		auto archetypeEntityData = fromArchetype.m_EntityStorage.GetEntityRecord(entityIndex);

		DECS_ASSERT(archetypeEntityData.m_EntityData != nullptr, "EntityData must be valid pointer!");

		toArchetype.AddEntityData(archetypeEntityData.m_EntityData);
		fromArchetype.m_EntityStorage.SetEntityRecord(entityIndex, nullptr, false);

		uint64_t thisArchetypeIndex = 0;
		uint64_t fromArchetypeIndex = 0;

		for (; thisArchetypeIndex < toArchetype.GetComponentAndTagCount(); thisArchetypeIndex++)
		{
			ArchetypeTypeData& thisTypeData = toArchetype.m_TypeData[thisArchetypeIndex];
			if (thisTypeData.m_TypeID == addedComponentTypeID)
			{
				continue;
			}

			ArchetypeTypeData& fromArchetypeData = fromArchetype.m_TypeData[fromArchetypeIndex];
			if (!fromArchetypeData.IsTag())
			{
				thisTypeData.m_PackedContainer->PushBackFromBase(fromArchetypeData.m_PackedContainer->GetComponentBasePtr(entityIndex));
			}

			fromArchetypeIndex++;
		}

		return true;
	}

	bool Archetype::MoveEntityAfterRemoveComponentWithoutDestroyingFromSource(
		Archetype& fromArchetype,
		Archetype& toArchetype,
		uint64_t entityIndex,
		TypeID removedComponentTypeID
	)
	{
		if (entityIndex >= fromArchetype.EntityCount())
		{
			return false;
		}

		auto archetypeEntityData = fromArchetype.m_EntityStorage.GetEntityRecord(entityIndex);

		DECS_ASSERT(archetypeEntityData.m_EntityData != nullptr, "EntityData must be valid pointer!");


		toArchetype.AddEntityData(archetypeEntityData.m_EntityData);
		fromArchetype.m_EntityStorage.SetEntityRecord(entityIndex, nullptr, false);

		uint64_t thisArchetypeIndex = 0;
		uint64_t fromArchetypeIndex = 0;

		for (; thisArchetypeIndex < toArchetype.GetComponentAndTagCount(); thisArchetypeIndex++, fromArchetypeIndex++)
		{
			ArchetypeTypeData& thisTypeData = toArchetype.m_TypeData[thisArchetypeIndex];
			ArchetypeTypeData& fromArchetypeData = fromArchetype.m_TypeData[fromArchetypeIndex];
			if (fromArchetypeData.m_TypeID == removedComponentTypeID)
			{
				fromArchetypeIndex += 1;
			}

			if (!thisTypeData.IsTag())
			{
				ArchetypeTypeData& updatetFromArchetypeData = fromArchetype.m_TypeData[fromArchetypeIndex];
				thisTypeData.m_PackedContainer->PushBackFromBase(updatetFromArchetypeData.m_PackedContainer->GetComponentBasePtr(entityIndex));
			}
		}

		return true;
	}
}