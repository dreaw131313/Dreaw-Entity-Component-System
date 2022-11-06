#pragma once	
#include "decspch.h"
#include "EntityManager.h"

namespace decs
{
	EntityManager::EntityManager()
	{
	}

	EntityManager::EntityManager(const uint64_t& initialEntitiesCapacity)
	{
		if (initialEntitiesCapacity > 0)
		{
			m_EntityData.reserve(initialEntitiesCapacity);
			m_FreeEntities.reserve(initialEntitiesCapacity / 3);
		}
	}

	EntityManager::~EntityManager()
	{
	}

	EntityID EntityManager::CreateEntity(const bool& isActive)
	{
		m_CreatedEntitiesCount += 1;
		if (m_FreeEntitiesCount > 0)
		{
			m_FreeEntitiesCount -= 1;
			EntityData& entityData = m_EntityData[m_FreeEntities.back()];
			m_FreeEntities.pop_back();

			entityData.IsAlive = true;
			entityData.IsActive = isActive;

			return entityData.ID;
		}
		else
		{
			m_enitiesDataCount += 1;
			EntityData& entityData = m_EntityData.emplace_back(m_EntityData.size(), isActive);

			return entityData.ID;
		}
	}

	bool EntityManager::DestroyEntity(const EntityID& entity)
	{
		if (IsEntityAlive(entity))
		{
			EntityData& entityData = m_EntityData[entity];
			m_CreatedEntitiesCount -= 1;
			m_FreeEntitiesCount += 1;
			m_FreeEntities.push_back(entity);
			entityData.Version += 1;
			entityData.IsAlive = false;
			return true;
		}
		return false;
	}
}
