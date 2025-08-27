#pragma once
#include "EntityManager.h"

#include "Entity.h"

namespace decs
{
	EntityManager::EntityManager()
	{
		InitializeLifeTimeData();
	}

	EntityManager::EntityManager(uint64_t entityDataHandleChunkSize):
		m_EntityDataHandles(entityDataHandleChunkSize)
	{
		InitializeLifeTimeData();

		if (entityDataHandleChunkSize > 0)
		{
			m_FreeEntities.reserve(entityDataHandleChunkSize / 3);
		}
	}

	EntityManager::~EntityManager()
	{
		DestroyLifeTimeData();
	}

	EntityDataHandle EntityManager::CreateEntity(bool isActive, Container& container)
	{
		if (GetFreeEntitiesCount() > 0)
		{
			auto it = m_FreeEntities.begin();

			EntityDataHandle entityDataHandle = std::move(m_FreeEntities.back());
			m_FreeEntities.pop_back();

			auto entityData = entityDataHandle.GetEntityData();
			entityData->SetState(EEntityState::Alive);
			entityData->SetActiveState(isActive);
			entityData->SetIsInManager(false);
			entityData->m_bIsCreatedByContainer = false;
			entityData->m_bIsEnabledByContainer = false;
			entityData->m_Container = &container;

			return entityDataHandle;
		}
		else
		{
			EntityData* entityData = new EntityData(m_LifeTimeData, static_cast<EntityID>(m_EntityDataHandles.Size()), isActive);
			entityData->SetIsInManager(false);
			entityData->m_Container = &container;

			return m_EntityDataHandles.EmplaceBack(entityData);
		}
	}

	bool EntityManager::DestroyEntity(const EntityDataHandle& entityDataHandle)
	{
		if (entityDataHandle.IsValid() && !entityDataHandle.GetEntityData()->IsDead())
		{
			m_FreeEntities.push_back(entityDataHandle);
			entityDataHandle.GetEntityData()->SetIsInManager(true);
			entityDataHandle.GetEntityData()->OnDestroyByEntityManager();

			return true;
		}

		return false;
	}

	void EntityManager::ForceDestroyEntity(const EntityDataHandle& entityDataHandle)
	{
		auto entityData = entityDataHandle.GetEntityData();
		if (entityData != nullptr && !entityData->IsInManager())
		{
			m_FreeEntities.push_back(entityDataHandle);
			entityData->SetIsInManager(true);
			entityData->OnDestroyByEntityManager();
		}
	}

	void EntityManager::InitializeLifeTimeData()
	{
		m_LifeTimeData = new EnityLifeTimeData();
		m_LifeTimeData->IncrementRefCount();
	}
	void EntityManager::DestroyLifeTimeData()
	{
		m_LifeTimeData->m_bIsContainerAlive.store(false);
		m_LifeTimeData->DecrementRefCount();
	}
}
