#pragma once
#include "EntityManager.h"

#include "Entity.h"

namespace decs
{
	EntityManager::EntityManager()
	{
	}

	EntityManager::EntityManager(uint64_t initialEntitiesCapacity)
	{
		m_EntityDataHandles.reserve(initialEntitiesCapacity);
		if (initialEntitiesCapacity > 0)
		{
			m_FreeEntities.reserve(initialEntitiesCapacity / 3);
		}
	}

	EntityManager::~EntityManager()
	{
		for (auto& handle : m_EntityDataHandles)
		{
			handle.GetEntityData()->m_Version++;
		}
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
			EntityData* entityData = new EntityData(static_cast<EntityID>(m_EntityDataHandles.size()), isActive);
			entityData->SetIsInManager(false);
			entityData->m_Container = &container;

			return m_EntityDataHandles.emplace_back(entityData);
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
}
