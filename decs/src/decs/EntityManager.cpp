#pragma once
#include "EntityManager.h"

#include "Entity.h"

namespace decs
{
	EntityManager::EntityManager()
	{
	}

	EntityManager::EntityManager(uint64_t initialEntitiesCapacity) :
		m_EntityData(initialEntitiesCapacity)
	{
		if (initialEntitiesCapacity > 0)
		{
			m_FreeEntities.reserve(initialEntitiesCapacity / 3);
		}
	}

	EntityData* EntityManager::CreateEntity(bool isActive)
	{
		m_CreatedEntitiesCount += 1;
		if (GetFreeEntitiesCount() > 0)
		{
			auto it = m_FreeEntities.begin();

			EntityData* entityData = m_FreeEntities.back();
			m_FreeEntities.pop_back();
			entityData->SetState(EEntityState::Alive);
			entityData->SetActiveState(isActive);
			entityData->SetIsInManager(false);
			entityData->m_bIsCreatedByContainer = false;
			entityData->m_bIsEnabledByContainer = false;

			return entityData;
		}
		else
		{
			EntityData& entityData = m_EntityData.EmplaceBack((EntityID)m_EntityData.Size(), isActive);
			entityData.SetIsInManager(false);
			return &entityData;
		}
	}

	bool EntityManager::DestroyEntity(EntityData& entityData)
	{
		if (!entityData.IsDead())
		{
			m_CreatedEntitiesCount -= 1;

			m_FreeEntities.push_back(&entityData);
			entityData.SetIsInManager(true);

			entityData.OnDestroyByEntityManager();
			return true;
		}

		return false;
	}

	void EntityManager::ForceDestroyEntity(EntityData& entityData)
	{
		if (!entityData.IsInManager())
		{
			m_CreatedEntitiesCount -= 1;

			m_FreeEntities.push_back(&entityData);
			entityData.SetIsInManager(true);

			entityData.OnDestroyByEntityManager();
		}
	}

	void EntityManager::CreateReservedEntityData(uint32_t entitesToReserve, std::vector<EntityData*>& reservedEntityData)
	{
		for (uint32_t idx = 0; idx < entitesToReserve; idx++)
		{
			if (m_FreeEntities.size() > 0)
			{
				EntityData* data = m_FreeEntities.back();
				m_FreeEntities.pop_back();
				data->SetIsInManager(false);
				reservedEntityData.push_back(data);
			}
			else
			{
				EntityData& data = m_EntityData.EmplaceBack((EntityID)m_EntityData.Size(), false);
				data.SetIsInManager(false);
				reservedEntityData.push_back(&data);
			}
		}
	}

	void EntityManager::CreateEntityFromReservedEntityData(EntityData* entityData, bool bIsActive)
	{
		m_CreatedEntitiesCount += 1;
		entityData->SetActiveState(bIsActive);
		entityData->SetState(EEntityState::Alive);
		entityData->m_bIsCreatedByContainer = false;
		entityData->m_bIsEnabledByContainer = false;
	}

	void EntityManager::ReturnReservedEntityData(std::vector<EntityData*> reservedEntityData)
	{
		uint64_t entitiesToReturn = reservedEntityData.size();
		for (uint64_t idx = 0; idx < entitiesToReturn; idx++)
		{
			auto entityData = reservedEntityData[idx];
			entityData->SetIsInManager(true);
			m_FreeEntities.push_back(entityData);
		}
	}
}
