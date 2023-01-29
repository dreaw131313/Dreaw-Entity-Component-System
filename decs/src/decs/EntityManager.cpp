#pragma once
#include "EntityManager.h"

#include "Entity.h"

namespace decs
{
	EntityManager::EntityManager()
	{
	}

	EntityManager::EntityManager(const uint64_t& initialEntitiesCapacity) :
		m_EntityData(initialEntitiesCapacity)
	{
		if (initialEntitiesCapacity > 0)
		{
			m_FreeEntities.reserve(initialEntitiesCapacity / 3);
		}
	}

	EntityID EntityManager::CreateEntity(const bool& isActive)
	{
		m_CreatedEntitiesCount += 1;
		if (m_FreeEntitiesCount > 0)
		{
			m_FreeEntitiesCount -= 1;
			EntityData& entityData = m_EntityData[m_FreeEntities.back()];
			entityData.SetState(EntityDestructionState::Alive);
			m_FreeEntities.pop_back();

			entityData.m_IsAlive = true;
			entityData.m_IsActive = isActive;

			return entityData.m_ID;
		}
		else
		{
			m_enitiesDataCount += 1;
			EntityData& entityData = m_EntityData.EmplaceBack(m_EntityData.Size(), isActive);
			return entityData.m_ID;
		}
	}

	bool EntityManager::DestroyEntity(const EntityID& entity)
	{
		if (entity < m_EntityData.Size())
		{
			EntityData& entityData = m_EntityData[entity];

			if (entityData.m_IsAlive)
			{
				m_CreatedEntitiesCount -= 1;
				m_FreeEntitiesCount += 1;
				m_FreeEntities.push_back(entity);

				entityData.m_Archetype = nullptr;
				entityData.m_Version += 1;
				entityData.m_IsAlive = false;
				return true;
			}
		}

		return false;
	}

	bool EntityManager::DestroyEntity(EntityData& entityData)
	{
		if (entityData.m_IsAlive)
		{
			m_CreatedEntitiesCount -= 1;
			m_FreeEntitiesCount += 1;
			m_FreeEntities.push_back(entityData.m_ID);

			entityData.m_Archetype = nullptr;
			entityData.m_Version += 1;
			entityData.m_IsAlive = false;
			return true;
		}

		return false;
	}
}
