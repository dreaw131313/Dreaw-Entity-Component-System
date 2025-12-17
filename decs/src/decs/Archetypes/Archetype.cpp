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
		if (GetTypeCount() <= Limits::MinComponentsInArchetypeToPerformMapLookup)
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
		if (GetTypeCount() < Limits::MinComponentsInArchetypeToPerformMapLookup)
		{
			for (uint32_t i = 0; i < GetTypeCount(); i++)
				if (m_TypeData[i].m_TypeID == typeID) return i;

			return std::numeric_limits<uint32_t>::max();
		}

		auto it = m_TypeIDsIndexes.find(typeID);
		if (it == m_TypeIDsIndexes.end())
			return std::numeric_limits<uint32_t>::max();

		return it->second;
	}

	bool Archetype::HasSameTypesAs(const Archetype& archetype) const
	{
		const uint32_t componentAndTagCount = GetTypeCount();

		if (archetype.GetTypeCount() != componentAndTagCount)
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

	bool Archetype::HasTypes_Exactly(const std::vector<TypeID>& types) const
	{
		const uint32_t componentAndTagCount = GetTypeCount();

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

	std::optional<TypeID> Archetype::IsRemoveComponentNeighbour(const Archetype& neighbour) const
	{
		const uint32_t typeCount = GetTypeCount();
		const uint32_t neighbourTypeCount = neighbour.GetTypeCount();

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

	std::optional<TypeID> Archetype::IsAddComponentNeighbour(const Archetype& neighbour) const
	{
		return neighbour.IsRemoveComponentNeighbour(*this);
	}

	void Archetype::ClearEntityDataAndComponents()
	{
		m_EntitiesData.clear();
		for (uint32_t i = 0; i < m_TypeData.size(); i++)
		{
			auto& typeData = m_TypeData[i];
			if (!typeData.IsTag())
			{
				typeData.m_PackedContainer->Clear();
			}
		}
	}

	void Archetype::AddTypeData_WithoutCheck(
		TypeID typeID,
		IPackedComponentContainer* packedContainer
	)
	{
		const uint32_t typeIndex = static_cast<uint32_t>(m_TypeData.size());
		m_TypeIDsIndexes[typeID] = typeIndex;
		if (packedContainer == nullptr)
		{
			// tag data
			m_TypeData.emplace_back(typeID, nullptr);
		}
		else
		{
			m_TypeData.emplace_back(typeID, packedContainer);
		}
	}

	void Archetype::AddEntityData(EntityData* entityData)
	{
		if (entityData == nullptr)
		{
			return;
		}

		entityData->m_Archetype = this;
		entityData->m_IndexInArchetype = static_cast<uint32_t>(EntityCount());

		m_EntitiesData.emplace_back(entityData);
	}

	void Archetype::RemoveSwapBackEntityData(uint64_t index)
	{
		if (index >= EntityCount())
		{
			return;
		}

		if (index < (EntityCount() - 1))
		{
			auto& backEntityData = m_EntitiesData.back();
			m_EntitiesData[index] = backEntityData;
			if (backEntityData.IsValid())
			{
				m_EntitiesData[index].m_EntityData->m_IndexInArchetype = static_cast<uint32_t>(index);
			}

		}
		m_EntitiesData.pop_back();
	}

	void Archetype::RemoveSwapBackEntity(uint64_t index)
	{
		if (index >= EntityCount())
		{
			return;
		}

		if (index == EntityCount() - 1)
		{
			for (uint64_t i = 0; i < GetTypeCount(); i++)
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
			for (uint64_t i = 0; i < GetTypeCount(); i++)
			{
				auto& typeData = m_TypeData[i];
				if (!typeData.IsTag())
				{
					typeData.m_PackedContainer->RemoveSwapBack(index);
				}
			}
		}

		RemoveSwapBackEntityData(index);

	}

	void Archetype::ReserveSpaceInArchetype(uint64_t desiredCapacity)
	{
		if (m_EntitiesData.capacity() < desiredCapacity)
		{
			m_EntitiesData.reserve(desiredCapacity);

			for (uint64_t idx = 0; idx < GetTypeCount(); idx++)
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
		m_EntitiesData.clear();
		for (uint64_t idx = 0; idx < GetTypeCount(); idx++)
		{
			auto& typeData = m_TypeData[idx];
			if (!typeData.IsTag())
			{
				typeData.m_PackedContainer->Clear();
			}
		}
	}

	void Archetype::InitEmptyFromOther(const Archetype& other)
	{
		uint32_t componentsCount = other.GetTypeCount();
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
					otherTypeData.m_PackedContainer->CloneEmpty()
				);
			}
		}
	}

	void Archetype::ShrinkToFit()
	{
		m_EntitiesData.shrink_to_fit();
		for (uint64_t idx = 0; idx < GetTypeCount(); idx++)
		{
			auto& typeData = m_TypeData[idx];
			if (typeData.IsTag())
			{
				continue;
			}
			typeData.m_PackedContainer->ShrinkToFit();
		}
	}

	void Archetype::AddEdge(TypeID componentTypeID, Archetype* archetype, EComponentEdgeType edgeType)
	{
		auto& edge = m_Edges[componentTypeID];
		if (!edge.IsValid())
		{
			edge.m_Archetype = archetype;
			edge.m_EdgeType = edgeType;
		}
	}

	bool Archetype::MoveEntityAfterAddType(
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

		EntityData* entityData = fromArchetype.m_EntitiesData[entityIndex].GetEntityData();

		toArchetype.AddEntityData(entityData);

		uint64_t thisArchetypeIndex = 0;
		uint64_t fromArchetypeIndex = 0;

		for (; thisArchetypeIndex < toArchetype.GetTypeCount(); thisArchetypeIndex++)
		{
			ArchetypeTypeData& toTypeData = toArchetype.m_TypeData[thisArchetypeIndex];
			if (toTypeData.m_TypeID == addedComponentTypeID)
			{
				continue;
			}

			ArchetypeTypeData& fromArchetypeData = fromArchetype.m_TypeData[fromArchetypeIndex];
			if (!fromArchetypeData.IsTag())
			{
				toTypeData.m_PackedContainer->MoveBack(fromArchetypeData.m_PackedContainer->GetComponentBasePtr(entityIndex));
				fromArchetypeData.m_PackedContainer->RemoveSwapBack(entityIndex);
			}

			fromArchetypeIndex++;
		}

		fromArchetype.RemoveSwapBackEntityData(entityIndex);

		return true;
	}

	bool Archetype::MoveEntityAfterRemoveType(
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

		ArchetypeEntityData& archetypeEntityData = fromArchetype.m_EntitiesData[entityIndex];
		toArchetype.AddEntityData(archetypeEntityData.GetEntityData());
		archetypeEntityData.Invalidate();

		uint64_t thisArchetypeIndex = 0;
		uint64_t fromArchetypeIndex = 0;

		for (; thisArchetypeIndex < toArchetype.GetTypeCount(); thisArchetypeIndex++, fromArchetypeIndex++)
		{
			ArchetypeTypeData& toTypeData = toArchetype.m_TypeData[thisArchetypeIndex];
			ArchetypeTypeData& fromArchetypeData = fromArchetype.m_TypeData[fromArchetypeIndex];
			if (fromArchetypeData.m_TypeID == removedComponentTypeID)
			{
				fromArchetypeIndex += 1;
			}

			if (!toTypeData.IsTag())
			{
				ArchetypeTypeData& updatetFromArchetypeData = fromArchetype.m_TypeData[fromArchetypeIndex];
				toTypeData.m_PackedContainer->MoveBack(updatetFromArchetypeData.m_PackedContainer->GetComponentBasePtr(entityIndex));
				updatetFromArchetypeData.m_PackedContainer->RemoveSwapBack(entityIndex);
			}
		}

		return true;
	}
}