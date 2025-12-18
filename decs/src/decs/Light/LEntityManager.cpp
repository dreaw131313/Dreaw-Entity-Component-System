#pragma once
#include "LEntityManager.h"

#include "LEntity.h"

namespace decs::light
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

	EntityData* EntityManager::CreateEntity(Container& container)
	{
		m_CreatedEntityCount++;

		if (GetFreeEntitiesCount() > 0)
		{
			auto it = m_FreeEntities.begin();

			EntityData* entityData = std::move(m_FreeEntities.back());
			m_FreeEntities.pop_back();

			entityData->m_Container = &container;
			entityData->m_bIsAlive = true;

			return entityData;
		}
		else
		{
			uint32_t id = static_cast<uint32_t>(m_EntityDatas.Size());
			EntityData* entityData = &m_EntityDatas.EmplaceBack(id);
			entityData->m_Container = &container;

			return entityData;
		}
	}

	bool EntityManager::DestroyEntity(EntityData* entityData)
	{
		if (entityData!= nullptr)
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
