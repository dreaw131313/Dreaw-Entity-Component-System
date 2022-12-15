#pragma once	
#include "decspch.h"
#include "EntityManager.h"

#include "Entity.h"

namespace decs
{
	EntityManager::EntityManager()
	{
	}

	EntityManager::EntityManager(const uint64_t& initialEntitiesCapacity):
		m_Entities(initialEntitiesCapacity),
		m_EntityData(initialEntitiesCapacity)
	{
		if (initialEntitiesCapacity > 0)
		{
			m_FreeEntities.reserve(initialEntitiesCapacity / 3);
		}
	}

	EntityManager::~EntityManager()
	{
	}

	Entity* EntityManager::CreateEntity(Container* forContainer, const bool& isActive)
	{
		m_CreatedEntitiesCount += 1;
		if (m_FreeEntitiesCount > 0)
		{
			m_FreeEntitiesCount -= 1;
			EntityData& entityData = m_EntityData[m_FreeEntities.back()];
			m_FreeEntities.pop_back();

			entityData.m_IsAlive = true;
			entityData.m_IsActive = isActive;

			entityData.m_EntityPtr->m_Container = forContainer;

			return entityData.m_EntityPtr;
		}
		else
		{
			m_enitiesDataCount += 1;
			EntityData& entityData = m_EntityData.emplace_back(m_EntityData.size(), isActive);
			entityData.m_EntityPtr = &m_Entities.EmplaceBack(entityData.m_ID, forContainer);
			return entityData.m_EntityPtr;
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
			entityData.m_Version += 1;
			entityData.m_IsAlive = false;
			return true;
		}
		return false;
	}
}
