#pragma once
#include "LArchetype.h"

#include "decs/Light/LEntity.h"
#include <algorithm>

namespace decs::light
{
	Archetype::Archetype()
	{ }

	Archetype::~Archetype()
	{
		for (auto& data : m_TypeData)
		{
			delete data.m_PackedContainer;
		}
	}

	bool Archetype::ContainComponentOrTagType(TypeID typeID) const
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
		if (GetComponentTagCount() < Limits::MinComponentsInArchetypeToPerformMapLookup)
		{
			for (uint32_t i = 0; i < GetComponentTagCount(); i++)
				if (m_TypeData[i].m_TypeID == typeID) return i;

			return std::numeric_limits<uint32_t>::max();
		}

		auto it = m_TypeIDsIndexes.find(typeID);
		if (it == m_TypeIDsIndexes.end())
			return std::numeric_limits<uint32_t>::max();

		return it->second;
	}

	bool Archetype::HasSameComponentsTagsAs(const Archetype& other) const
	{
		const uint32_t componentAndTagCount = GetComponentTagCount();

		if (other.GetComponentTagCount() != componentAndTagCount)
		{
			return false;
		}

		for (uint32_t i = 0; i < componentAndTagCount; i++)
		{
			if (GetTypeID(i) != other.GetTypeID(i))
			{
				return false;
			}
		}

		return true;
	}

	bool Archetype::HasSameComponentsTagsFiltersAs(const Archetype& other) const
	{
		const size_t anyDataCount = GetComponentTagFilterCount();
		if (anyDataCount != other.GetComponentTagFilterCount())
		{
			return false;
		}

		auto thisTypeData = GetComponentAndTagRecords();
		auto otherTypeData = other.GetComponentAndTagRecords();
		for (size_t i = 0; i < thisTypeData.size(); i++)
		{
			if (thisTypeData[i].m_TypeID != otherTypeData[i].m_TypeID)
			{
				return false;
			}
		}

		auto thisFilters = GetFilters();
		auto otherFilters = other.GetFilters();
		for (size_t i = 0; i < thisFilters.size(); i++)
		{
			auto& filterRecord = thisFilters[i];
			auto& otherFilterRecord = otherFilters[i];

			if (filterRecord.m_FilterTypeID != otherFilterRecord.m_FilterTypeID
				|| filterRecord.m_FilterContainer->StoresSameData(*otherFilterRecord.m_FilterContainer)
				)
			{
				return false;
			}
		}

		return true;
	}

	bool Archetype::HasTypes_Exactly(const ecsVector<TypeID>& types) const
	{
		const uint32_t componentAndTagCount = GetComponentTagCount();

		if (static_cast<uint32_t>(types.size()) != componentAndTagCount)
		{
			return false;
		}

		for (uint32_t i = 0; i < componentAndTagCount; i++)
		{
			if (!ContainComponentOrTagType(types[i]))
			{
				return false;
			}
		}

		return true;
	}

	std::optional<ArchetypeDataKey> Archetype::IsRemoveAnyDataNeighbour(const Archetype& neighbour) const
	{
		const size_t anyDataCount = GetComponentTagFilterCount();
		const size_t neighbourAnyDataCount = neighbour.GetComponentTagFilterCount();

		if (neighbourAnyDataCount >= anyDataCount || (anyDataCount - neighbourAnyDataCount) != 1)
		{
			return {};
		}

		ArchetypeDataKey dataKey{};
		size_t foundedNeighbourTypeCount = 0;

		for (auto& reocrd : m_TypeData)
		{
			if (!neighbour.ContainComponentOrTagType(reocrd.m_TypeID))
			{
				dataKey = reocrd.m_TypeID;
				foundedNeighbourTypeCount++;
			}
			if (foundedNeighbourTypeCount > 1)
			{
				return {};
			}
		}

		for (auto& filterRecord : m_Filters)
		{
			if (!neighbour.ContainFilter(filterRecord.m_FilterContainer))
			{
				dataKey = filterRecord.m_FilterContainer;
				foundedNeighbourTypeCount++;
			}
			if (foundedNeighbourTypeCount > 1)
			{
				return {};
			}
		}

		return std::optional(dataKey);
	}

	std::optional<ArchetypeDataKey> Archetype::IsAddAnyDataNeighbour(const Archetype& neighbour) const
	{
		return neighbour.IsRemoveAnyDataNeighbour(*this);
	}

	bool Archetype::HasSameFiltersAs(const Archetype& other) const
	{
		bool result = false;
		if (m_Filters.size() != other.m_Filters.size())
		{
			return false;
		}

		for (size_t i = 0; i < m_Filters.size(); i++)
		{
			const auto& filter = m_Filters[i];
			const auto& otherFilter = other.m_Filters[i];
			if (!filter.m_FilterContainer->StoresSameData(*otherFilter.m_FilterContainer))
			{
				return false;
			}
		}

		return result;
	}

	void Archetype::AddFilter_WithoutCheckout(IFilterTypeManager* typeManager, IFilterContainerBase* filterContainer)
	{
		if (typeManager == nullptr || filterContainer == nullptr)
		{
			return;
		}

		m_Filters.emplace_back(typeManager, filterContainer);
		filterContainer->IncrementRefCount();
	}

	void Archetype::AddFilterInCorrectPlace(IFilterTypeManager& typeManager, IFilterContainerBase& filterContainer)
	{
		filterContainer.IncrementRefCount();
		for (size_t idx = 0; idx < m_Filters.size(); idx++)
		{
			auto& filterRecord = m_Filters[idx];
			if (filterContainer.GetFilterTypeID() < filterRecord.m_FilterTypeID)
			{
				m_Filters.insert(m_Filters.begin() + idx, ArchetypeFilterData(&typeManager, &filterContainer));
				return;
			}
		}

		m_Filters.push_back(ArchetypeFilterData(&typeManager, &filterContainer));
	}

	bool Archetype::ContainComponentOrTagOrFilterType(TypeID typeID) const noexcept
	{
		return ContainComponentOrTagType(typeID) || HasFilterWithType(typeID);
	}

	void Archetype::ClearEntityDataAndComponents()
	{
		m_Entities.Clear();
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
		IComponentContext* componentContext
	)
	{
		const uint32_t typeIndex = static_cast<uint32_t>(m_TypeData.size());
		m_TypeIDsIndexes[typeID] = typeIndex;
		m_TypeData.emplace_back(typeID, componentContext);
	}

	void Archetype::AddEntityData(EntityData* entityData)
	{
		if (entityData == nullptr)
		{
			return;
		}

		entityData->m_Archetype = this;
		m_Entities.PushBackUpdateIndex(entityData);
	}

	bool Archetype::AddEntityDataAndDefaultComponents(EntityData* entityData)
	{
		if (entityData == nullptr)
		{
			return false;
		}

		entityData->m_Archetype = this;
		m_Entities.PushBackUpdateIndex(entityData);

		for (auto& typeData : m_TypeData)
		{
			if (!typeData.IsTag())
			{
				typeData.m_PackedContainer->PushBackDefault();
			}
		}

		return true;
	}

	void Archetype::InvokeCreateObserversOnEntity(size_t entityIndex)
	{
		if (entityIndex >= m_Entities.Size())
		{
			return;
		}

		auto entityData = m_Entities.Get(entityIndex);
		Entity e(entityData);
		entityData->LockOperations();
		{
			for (auto& typeData : m_TypeData)
			{
				if (typeData.IsTag())
				{
					continue;
				}
				typeData.m_ComponentContext->InvokeOnCreateObserver(e, typeData.m_PackedContainer->GetComponentBasePtr(entityIndex));
			}
		}
		entityData->UnlockOperations();
	}



	void Archetype::RemoveSwapBackEntityData(size_t index)
	{
		m_Entities.RemoveSwapBack_UpdateEntityIndex(index);
	}

	void Archetype::RemoveSwapBackEntity(size_t index)
	{
		if (index >= EntityCount())
		{
			return;
		}

		if (index == EntityCount() - 1)
		{
			for (uint64_t i = 0; i < GetComponentTagCount(); i++)
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
			for (uint64_t i = 0; i < GetComponentTagCount(); i++)
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

	void Archetype::ReserveSpaceInArchetype(size_t desiredCapacity)
	{
		if (m_Entities.Capacity() < desiredCapacity)
		{
			m_Entities.Reserve(desiredCapacity);

			for (uint64_t idx = 0; idx < GetComponentTagCount(); idx++)
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
		m_Entities.Clear();
		for (uint64_t idx = 0; idx < GetComponentTagCount(); idx++)
		{
			auto& typeData = m_TypeData[idx];
			if (!typeData.IsTag())
			{
				typeData.m_PackedContainer->Clear();
			}
		}
	}

	void Archetype::InitEmptyFromOther(
		const Archetype& other,
		ComponentContextManager& componentContextManager,
		FilterManager& filterManager
	)
	{
		uint32_t componentsCount = other.GetComponentTagCount();
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
					componentContextManager.GetOrCreateContext(otherTypeData.m_ComponentContext)
				);
			}
		}

		if (!other.m_Filters.empty())
		{
			m_Filters.reserve(other.m_Filters.size());
			for (auto& otherFilter : other.m_Filters)
			{
				auto filterResult = filterManager.GetMatchingFilter(*otherFilter.m_FilterContainer);
				AddFilter_WithoutCheckout(filterResult.m_FilterTypeManager, filterResult.m_FilterContainer);
			}
		}
	}

	void Archetype::ShrinkToFit()
	{
		m_Entities.ShrinkToFit();
		for (uint64_t idx = 0; idx < GetComponentTagCount(); idx++)
		{
			auto& typeData = m_TypeData[idx];
			if (typeData.IsTag())
			{
				continue;
			}
			typeData.m_PackedContainer->ShrinkToFit();
		}
	}

	void Archetype::ResetOnDestroy(FilterManager& filterManager)
	{
		m_TypeIDsIndexes.clear();
		m_Edges.clear();

		m_Entities.Clear();

		for (auto& data : m_TypeData)
		{
			delete data.m_PackedContainer;
		}

		for (auto& filter : m_Filters)
		{
			filter.m_FilterContainer->DecrementRefCount();
			filterManager.DeleteFilter(filter.m_FilterContainer);
		}

		m_TypeData.clear();
		m_Filters.clear();
	}

	void Archetype::AddEdge(ArchetypeDataKey key, Archetype* archetype, EArchetypeEdgeType edgeType)
	{
		auto& edge = m_Edges[key];
		if (!edge.IsValid())
		{
			edge.m_Archetype = archetype;
			edge.m_EdgeType = edgeType;
		}
	}

	void Archetype::RemoveFromNeighbours()
	{
		for (auto& [edgeKey, edge] : m_Edges)
		{
			edge.m_Archetype->m_Edges.erase(edgeKey);
		}
		m_Edges.clear();
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

		EntityData* entityData = fromArchetype.m_Entities.Get(entityIndex);
		DECS_ASSERT(entityData != nullptr, "Entity data must be valid");

		toArchetype.AddEntityData(entityData);

		uint64_t thisArchetypeIndex = 0;
		uint64_t fromArchetypeIndex = 0;

		for (; thisArchetypeIndex < toArchetype.GetComponentTagCount(); thisArchetypeIndex++)
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

		EntityData* entityData = fromArchetype.m_Entities.Get(entityIndex);
		DECS_ASSERT(entityData != nullptr, "cannot move record where entity data is nullptr!");

		toArchetype.AddEntityData(entityData);

		uint64_t thisArchetypeIndex = 0;
		uint64_t fromArchetypeIndex = 0;

		for (; thisArchetypeIndex < toArchetype.GetComponentTagCount(); thisArchetypeIndex++, fromArchetypeIndex++)
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

		fromArchetype.RemoveSwapBackEntityData(entityIndex);

		return true;
	}

	bool Archetype::MoveEntiyAfterFilterChange(Archetype& fromArchetype, Archetype& toArchetype, uint64_t entityIndex)
	{
		if (entityIndex >= fromArchetype.EntityCount() || fromArchetype.GetComponentTagCount() != toArchetype.GetComponentTagCount())
		{
			return false;
		}

		DECS_ASSERT(fromArchetype.HasSameComponentsTagsAs(toArchetype), "Moving entity can be performed only to archetypes with same types!");

		EntityData* entityData = fromArchetype.m_Entities.Get(entityIndex);
		DECS_ASSERT(entityData != nullptr, "Entity data must be valid");

		toArchetype.AddEntityData(entityData);

		size_t typeCount = toArchetype.GetComponentTagCount();
		for (size_t typeIdx = 0; typeIdx < typeCount; typeIdx++)
		{
			ArchetypeTypeData& fromArchetypeData = fromArchetype.m_TypeData[typeIdx];
			ArchetypeTypeData& toTypeData = toArchetype.m_TypeData[typeIdx];

			if (!fromArchetypeData.IsTag())
			{
				toTypeData.m_PackedContainer->MoveBack(fromArchetypeData.m_PackedContainer->GetComponentBasePtr(entityIndex));
				fromArchetypeData.m_PackedContainer->RemoveSwapBack(entityIndex);
			}
		}

		fromArchetype.RemoveSwapBackEntityData(entityIndex);

		return true;
	}

}