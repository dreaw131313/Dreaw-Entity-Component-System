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

	uint32_t Archetype::FindTypeIndex(TypeID typeID) const
	{
		if (ComponentCount() < Limits::MinComponentsInArchetypeToPerformMapLookup)
		{
			for (uint32_t i = 0; i < ComponentCount(); i++)
				if (m_TypeData[i].m_TypeID == typeID) return i;

			return std::numeric_limits<uint32_t>::max();
		}

		auto it = m_TypeIDsIndexes.find(typeID);
		if (it == m_TypeIDsIndexes.end())
			return std::numeric_limits<uint32_t>::max();

		return it->second;
	}

	void Archetype::ClearEntityDataAndComponents()
	{
		m_EntitiesData.clear();
		for (uint32_t i = 0; i < m_TypeData.size(); i++)
		{
			auto& typeData = m_TypeData[i];
			typeData.m_PackedContainer->Clear();
		}
		m_EntitiesCount = 0;
	}

	void Archetype::InsertComponentContextInCorrectPlace(ComponentContextBase* componentContext, uint32_t typeDataIndex)
	{
		// TODO: find better way to insert new elements,
		/*uint64_t size = m_ComponentContextsInOrder.size();
		for (uint32_t i = 0; i < size; i++)
		{
		if (m_ComponentContextsInOrder[i].m_ComponentContext->GetObserverOrder() >= componentContext->GetObserverOrder())
		{
		auto insertPos = m_ComponentContextsInOrder.begin();
		std::advance(insertPos, i);
		m_ComponentContextsInOrder.insert(insertPos, { componentContext, typeDataIndex });
		return;
		}
		}
		m_ComponentContextsInOrder.push_back({ componentContext, typeDataIndex });*/

		//
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
	}

	void Archetype::AddTypeData_WithoutCheck(TypeID typeID, PackedContainerBase* packedContainer, ComponentContextBase* componentContext)
	{
		const uint32_t typeIndex = static_cast<uint32_t>(m_TypeData.size());
		m_TypeIDsIndexes[typeID] = typeIndex;
		if (packedContainer == nullptr || componentContext == nullptr)
		{
			// tag data
			m_TypeData.emplace_back(typeID, nullptr, nullptr, nullptr);
		}
		else
		{
			m_TypeData.emplace_back(typeID, packedContainer, componentContext, componentContext->GetStableContainer());
			InsertComponentContextInCorrectPlace(componentContext, typeIndex);
		}
	}

	void Archetype::AddTypeData_WithCheck(
		const TypeID& id,
		PackedContainerBase* packedContainer,
		ComponentContextBase* componentContext
	)
	{
		auto it = m_TypeIDsIndexes.find(id);
		if (it == m_TypeIDsIndexes.end())
		{
			AddTypeData_WithoutCheck(
				id,
				packedContainer,
				componentContext
			);
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
		m_EntitiesData.emplace_back(entityData);

		entityData->m_Archetype = this;
		entityData->m_IndexInArchetype = m_EntitiesCount;

		m_EntitiesCount += 1;
	}

	void Archetype::RemoveSwapBackEntityData(uint64_t index)
	{
		if (index >= m_EntitiesCount)
		{
			return;
		}

		if (index < (m_EntitiesCount - 1))
		{
			auto& backEntityData = m_EntitiesData.back();
			m_EntitiesData[index] = backEntityData;
			if (backEntityData.IsValid())
			{
				m_EntitiesData[index].m_EntityData->m_IndexInArchetype = static_cast<uint32_t>(index);
			}

		}
		m_EntitiesData.pop_back();
		m_EntitiesCount -= 1;
	}

	void Archetype::RemoveSwapBackEntity(uint64_t index)
	{
		if (index >= m_EntitiesCount)
		{
			return;
		}

		if (index == m_EntitiesCount - 1)
		{
			for (uint64_t i = 0; i < ComponentCount(); i++)
			{
				auto& typeData = m_TypeData[i];
				if (!typeData.IsTag())
				{
					ComponentBase* componentPtr = typeData.m_PackedContainer->GetComponentBasePtr(index);
					typeData.m_PackedContainer->PopBack();
					typeData.m_StableContainer->Remove(componentPtr);
				}
			}

		}
		else
		{
			for (uint64_t i = 0; i < ComponentCount(); i++)
			{
				auto& typeData = m_TypeData[i];
				if (!typeData.IsTag())
				{
					ComponentBase* componentPtr = typeData.m_PackedContainer->GetComponentBasePtr(index);
					typeData.m_PackedContainer->RemoveSwapBack(index);
					typeData.m_StableContainer->Remove(componentPtr);
				}
			}
		}

		RemoveSwapBackEntityData(index);

	}

	void Archetype::RemoveSwapBackRecordRaw(uint64_t index)
	{
		if (index >= m_EntitiesCount)
		{
			return;
		}

		if (index == m_EntitiesCount - 1)
		{
			m_EntitiesData.pop_back();
			for (uint64_t i = 0; i < ComponentCount(); i++)
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
			auto& backEntityData = m_EntitiesData.back();
			if (backEntityData.IsValid())
			{
				backEntityData.m_EntityData->m_IndexInArchetype = static_cast<uint32_t>(index);
			}

			m_EntitiesData[index] = m_EntitiesData.back();
			m_EntitiesData.pop_back();

			for (uint64_t i = 0; i < ComponentCount(); i++)
			{
				auto& typeData = m_TypeData[i];
				if (!typeData.IsTag())
				{
					typeData.m_PackedContainer->RemoveSwapBack(index);
				}
			}
		}

		m_EntitiesCount -= 1;
	}

	void Archetype::SetRecordAsIntendedToDelayedDestroy(uint64_t index)
	{
		if (index >= m_EntitiesCount)
		{
			return;
		}
		m_EntitiesData[index].Invalidate();
	}

	void Archetype::ReserveSpaceInArchetype(uint64_t desiredCapacity)
	{
		if (m_EntitiesData.capacity() < desiredCapacity)
		{
			m_EntitiesData.reserve(desiredCapacity);

			for (uint64_t idx = 0; idx < ComponentCount(); idx++)
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
		m_EntitiesCount = 0;
		m_EntitiesData.clear();
		for (uint64_t idx = 0; idx < ComponentCount(); idx++)
		{
			auto& typeData = m_TypeData[idx];
			if (!typeData.IsTag())
			{
				typeData.m_PackedContainer->Clear();
			}
		}
	}

	void Archetype::InitEmptyFromOther(Archetype& other, ComponentContextsManager* componentContexts)
	{
		uint32_t componentsCount = other.ComponentCount();
		m_TypeData.reserve(componentsCount);

		for (uint32_t i = 0; i < componentsCount; i++)
		{
			ArchetypeTypeData& otherTypeData = other.m_TypeData[i];
			otherTypeData.m_TypeID;
			m_TypeIDsIndexes[otherTypeData.m_TypeID] = i;

			if (otherTypeData.IsTag())
			{
				AddTypeData_WithoutCheck(
					otherTypeData.m_TypeID,
					nullptr,
					nullptr
				);
			}
			else
			{
				AddTypeData_WithoutCheck(
					otherTypeData.m_TypeID,
					otherTypeData.m_PackedContainer->Clone(),
					componentContexts->GetComponentContext(otherTypeData.m_TypeID)
				);
			}
		}
	}

	/*

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

		for (; thisArchetypeIndex < ComponentCount(); thisArchetypeIndex++, fromArchetypeIndex++)
		{
			ArchetypeTypeData& thisTypeData = m_TypeData[thisArchetypeIndex];
			ArchetypeTypeData& fromArchetypeData = fromArchetype->m_TypeData[fromArchetypeIndex];
			if (fromArchetypeData.m_TypeID == removedComponentTypeID)
			{
				ComponentBase* componentPtr = fromArchetypeData.m_PackedContainer->GetComponentBasePtr(fromIndex);
				fromArchetypeData.m_StableContainer->Remove(componentPtr);

				fromArchetypeData.m_PackedContainer->RemoveSwapBack(fromIndex);

				fromArchetypeIndex += 1;
			}

			ArchetypeTypeData& updatetFromArchetypeData = fromArchetype->m_TypeData[fromArchetypeIndex];

			thisTypeData.m_PackedContainer->PushBack(
				updatetFromArchetypeData.m_PackedContainer->GetComponentBasePtr(fromIndex)
			);

			updatetFromArchetypeData.m_PackedContainer->RemoveSwapBack(fromIndex);
		}

		fromArchetype->RemoveSwapBackEntityData(fromIndex);
	}

	void Archetype::MoveEntityComponentsAfterRemoveComponent(Archetype* fromArchetype, uint64_t fromIndex, EntityData* entityData)
	{
		uint64_t thisArchetypeIndex = 0;
		uint64_t fromArchetypeIndex = 0;

		this->AddEntityData(entityData);

		for (; thisArchetypeIndex < ComponentCount(); )
		{
			ArchetypeTypeData& thisTypeData = m_TypeData[thisArchetypeIndex];
			ArchetypeTypeData& fromArchetypeData = fromArchetype->m_TypeData[fromArchetypeIndex];

			if (thisTypeData.m_TypeID != fromArchetypeData.m_TypeID)
			{
				ComponentBase* componentPtr = fromArchetypeData.m_PackedContainer->GetComponentBasePtr(fromIndex);
				fromArchetypeData.m_StableContainer->Remove(componentPtr);

				fromArchetypeData.m_PackedContainer->RemoveSwapBack(fromIndex);

				fromArchetypeIndex += 1;
			}
			else
			{
				thisTypeData.m_PackedContainer->PushBack(
					fromArchetypeData.m_PackedContainer->GetComponentBasePtr(fromIndex)
				);

				fromArchetypeData.m_PackedContainer->RemoveSwapBack(fromIndex);

				thisArchetypeIndex += 1;
				fromArchetypeIndex += 1;
			}
		}

		// remove remaining components:
		for (; fromArchetypeIndex < fromArchetype->ComponentCount(); fromArchetypeIndex++)
		{
			ArchetypeTypeData& fromArchetypeData = fromArchetype->m_TypeData[fromArchetypeIndex];
			if (fromArchetypeData.m_StableContainer != nullptr)
			{
				ComponentBase* componentPtr = fromArchetypeData.m_PackedContainer->GetComponentBasePtr(fromIndex);
				fromArchetypeData.m_StableContainer->Remove(componentPtr);
			}

			fromArchetypeData.m_PackedContainer->RemoveSwapBack(fromIndex);
		}

		fromArchetype->RemoveSwapBackEntityData(fromIndex);
	}

	*/

	void Archetype::MoveEntityAfterRemoveComponentWithoutDestroyingFromSource(
		TypeID removedComponentTypeID,
		Archetype* fromArchetype,
		uint64_t fromIndex,
		EntityData* entityData
	)
	{
		uint64_t thisArchetypeIndex = 0;
		uint64_t fromArchetypeIndex = 0;

		this->AddEntityData(entityData);

		for (; thisArchetypeIndex < ComponentCount(); thisArchetypeIndex++, fromArchetypeIndex++)
		{
			ArchetypeTypeData& thisTypeData = m_TypeData[thisArchetypeIndex];
			if (thisTypeData.IsTag())
			{
				continue;
			}

			ArchetypeTypeData& fromArchetypeData = fromArchetype->m_TypeData[fromArchetypeIndex];
			if (fromArchetypeData.m_TypeID == removedComponentTypeID)
			{
				fromArchetypeIndex += 1;
			}

			ArchetypeTypeData& updatetFromArchetypeData = fromArchetype->m_TypeData[fromArchetypeIndex];

			thisTypeData.m_PackedContainer->PushBack(
				updatetFromArchetypeData.m_PackedContainer->GetComponentBasePtr(fromIndex)
			);

		}

		fromArchetype->m_EntitiesData[fromIndex].m_bIsActive = false;
	}

	void Archetype::RemoveSwapBackEntityAfterMoveEntityWithoutDestroyingSource(uint64_t entityIndex, TypeID removedComponentTypeID)
	{
		if (entityIndex >= m_EntitiesCount)
		{
			return;
		}

		const uint32_t componentCount = ComponentCount();
		if (entityIndex == m_EntitiesCount - 1)
		{
			for (uint64_t i = 0; i < componentCount; i++)
			{
				auto& typeData = m_TypeData[i];
				if (!typeData.IsTag())
				{
					if (removedComponentTypeID == typeData.m_TypeID)
					{
						ComponentBase* componentPtr = typeData.m_PackedContainer->GetComponentBasePtr(entityIndex);
						typeData.m_StableContainer->Remove(componentPtr);
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
						ComponentBase* componentPtr = typeData.m_PackedContainer->GetComponentBasePtr(entityIndex);
						typeData.m_StableContainer->Remove(componentPtr);
					}

					typeData.m_PackedContainer->RemoveSwapBack(entityIndex);
				}
			}
		}

		RemoveSwapBackEntityData(entityIndex);

	}

	void Archetype::MoveEntityAfterAddComponentWithoutDestroyingFromSource(Archetype* fromArchetype, uint64_t fromIndex, TypeID newComponentTypeID, EntityData* entityData)
	{
		this->AddEntityData(entityData);

		uint64_t thisArchetypeIndex = 0;
		uint64_t fromArchetypeIndex = 0;

		for (; thisArchetypeIndex < ComponentCount(); thisArchetypeIndex++)
		{
			ArchetypeTypeData& thisTypeData = m_TypeData[thisArchetypeIndex];
			if (thisTypeData.IsTag() || thisTypeData.m_TypeID == newComponentTypeID)
			{
				continue;
			}

			ArchetypeTypeData& fromArchetypeData = fromArchetype->m_TypeData[fromArchetypeIndex];

			thisTypeData.m_PackedContainer->PushBack(
				fromArchetypeData.m_PackedContainer->GetComponentBasePtr(fromIndex)
			);

			fromArchetypeIndex++;
		}
	}

	void Archetype::ShrinkToFit()
	{
		m_EntitiesData.shrink_to_fit();
		for (uint64_t idx = 0; idx < ComponentCount(); idx++)
		{
			m_TypeData[idx].m_PackedContainer->ShrinkToFit();
		}
	}

}