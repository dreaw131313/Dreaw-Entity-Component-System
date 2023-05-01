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
		if (m_FreeEntitiesCount > 0)
		{
			m_FreeEntitiesCount -= 1;

			auto it = m_FreeEntities.begin();
			 
			EntityData* entityData = m_FreeEntities.back();
			m_FreeEntities.pop_back();
			entityData->SetState(EntityState::Alive);
			entityData->m_bIsActive = isActive;

			return entityData;
		}
		else
		{
			EntityData& entityData = m_EntityData.EmplaceBack((EntityID)m_EntityData.Size(), isActive);
			return &entityData;
		}
	}

	bool EntityManager::DestroyEntity(EntityData& entityData)
	{
		if (!entityData.IsDead())
		{
			m_CreatedEntitiesCount -= 1;
			m_FreeEntitiesCount += 1;

			m_FreeEntities.push_back(&entityData);

			entityData.OnDestroyByEntityManager();
			return true;
		}

		return false;
	}

	void EntityManager::CreateReservedEntityData(uint32_t entitesToReserve, std::vector<EntityData*>& reservedEntityData)
	{
		for (uint32_t idx = 0; idx < entitesToReserve; idx++)
		{
			if (m_FreeEntities.size() > 0)
			{
				EntityData* data = m_FreeEntities.back();
				m_FreeEntities.pop_back();
				reservedEntityData.push_back(data);
				m_FreeEntitiesCount -= 1;
			}
			else
			{
				EntityData& data = m_EntityData.EmplaceBack((EntityID)m_EntityData.Size(), false);
				reservedEntityData.push_back(&data);
			}
		}
	}

	void EntityManager::CreateEntityFromReservedEntityData(EntityData* entityData, bool bIsActive)
	{
		m_CreatedEntitiesCount += 1;
		entityData->m_bIsActive = bIsActive;
		entityData->SetState(EntityState::Alive);
	}

	void EntityManager::ReturnReservedEntityData(std::vector<EntityData*> reservedEntityData)
	{
		uint64_t entitiesToReturn = reservedEntityData.size();
		m_FreeEntitiesCount += entitiesToReturn;
		for (uint64_t idx = 0; idx < entitiesToReturn; idx++)
		{
			m_FreeEntities.push_back(reservedEntityData[idx]);
		}
	}
}
