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

	EntityID EntityManager::CreateEntity(bool isActive)
	{
		m_CreatedEntitiesCount += 1;
		if (m_FreeEntitiesCount > 0)
		{
			m_FreeEntitiesCount -= 1;
			EntityData& entityData = m_EntityData[m_FreeEntities.back()];
			entityData.SetState(EntityState::Alive);
			m_FreeEntities.pop_back();
			entityData.m_bIsActive = isActive;

			return entityData.m_ID;
		}
		else
		{
			EntityData& entityData = m_EntityData.EmplaceBack((EntityID)m_EntityData.Size(), isActive);
			return entityData.m_ID;
		}
	}

	bool EntityManager::DestroyEntityInternal(EntityID entity)
	{
		if (entity < m_EntityData.Size())
		{
			EntityData& entityData = m_EntityData[entity];

			if (!entityData.IsDead())
			{
				m_CreatedEntitiesCount -= 1;
				m_FreeEntitiesCount += 1;
				m_FreeEntities.push_back(entity);

				entityData.OnDestroyByEntityManager();
				return true;
			}
		}

		return false;
	}

	bool EntityManager::DestroyEntityInternal(EntityData& entityData)
	{
		if (!entityData.IsDead())
		{
			m_CreatedEntitiesCount -= 1;
			m_FreeEntitiesCount += 1;
			m_FreeEntities.push_back(entityData.m_ID);

			entityData.OnDestroyByEntityManager();
			return true;
		}

		return false;
	}
}
