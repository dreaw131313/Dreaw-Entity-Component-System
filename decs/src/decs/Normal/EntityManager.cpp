#pragma once
#include "EntityManager.h"

#include "Entity.h"

namespace decs
{
	EntityManager::EntityManager()
	{
	}

	EntityManager::EntityManager(uint64_t entityDataHandleChunkSize):
		m_EntityDatas(entityDataHandleChunkSize)
	{
		if (entityDataHandleChunkSize > 0)
		{
			m_FreeEntities.reserve(entityDataHandleChunkSize / 3);
		}
	}

	EntityManager::~EntityManager()
	{
	}

	EntityData* EntityManager::CreateEntity(bool isActive)
	{
		m_CreatedEntityCount++;

		if (GetFreeEntitiesCount() > 0)
		{
			auto it = m_FreeEntities.begin();

			EntityData* entityData = std::move(m_FreeEntities.back());
			m_FreeEntities.pop_back();

			entityData->SetState(EEntityState::Alive);
			entityData->SetActiveState(isActive);
			entityData->m_bIsCreatedByContainer = false;
			entityData->m_bIsEnabledByContainer = false;
			entityData->m_bIsInManager = true;

			return entityData;
		}
		else
		{
			uint32_t id = static_cast<uint32_t>(m_EntityDatas.Size());
			EntityData* entityData = &m_EntityDatas.EmplaceBack(id, isActive);
			entityData->m_bIsInManager = true;

			return entityData;
		}
	}

	bool EntityManager::DestroyEntity(EntityData* entityData)
	{
		if (entityData!= nullptr && !entityData->IsDead())
		{
			m_FreeEntities.push_back(entityData);
			entityData->OnDestroyByEntityManager();

			m_CreatedEntityCount--;

			return true;
		}

		return false;
	}

	void EntityManager::ForceDestroyEntity(EntityData* entityData)
	{
		if (entityData != nullptr && !entityData->IsInManager())
		{
			m_FreeEntities.push_back(entityData);
			entityData->OnDestroyByEntityManager();

			m_CreatedEntityCount--;
		}
	}
}
