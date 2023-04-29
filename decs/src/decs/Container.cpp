#pragma once
#include "Container.h"
#include "Entity.h"

namespace decs
{
	Container::Container() :
		m_HaveOwnEntityManager(true),
		m_EntityManager(new EntityManager(m_DefaultEntitiesChunkSize)),
		m_HaveOwnComponentContextManager(true),
		m_ComponentContextManager(new ComponentContextsManager(nullptr))
	{

	}

	Container::Container(
		uint64_t enititesChunkSize,
		uint64_t stableComponentDefaultChunkSize,
		uint64_t m_EmptyEntitiesChunkSize
	) :
		m_HaveOwnEntityManager(true),
		m_EntityManager(new EntityManager(enititesChunkSize)),
		m_StableContainers(stableComponentDefaultChunkSize),
		m_HaveOwnComponentContextManager(true),
		m_ComponentContextManager(new ComponentContextsManager(nullptr)),
		m_EmptyEntities(m_EmptyEntitiesChunkSize)
	{
	}

	Container::Container(
		EntityManager* entityManager,
		uint64_t stableComponentDefaultChunkSize,
		uint64_t m_EmptyEntitiesChunkSize
	) :
		m_HaveOwnEntityManager(entityManager == nullptr),
		m_EntityManager(entityManager == nullptr ? new EntityManager(m_DefaultEntitiesChunkSize) : entityManager),
		m_HaveOwnComponentContextManager(true),
		m_ComponentContextManager(new ComponentContextsManager(nullptr)),
		m_StableContainers(stableComponentDefaultChunkSize),
		m_EmptyEntities(m_EmptyEntitiesChunkSize)
	{
	}

	Container::~Container()
	{
		if (m_HaveOwnEntityManager)
		{
			delete m_EntityManager;
		}
		else
		{
			DestroyOwnedEntities(false);
		}
		if (m_HaveOwnComponentContextManager)
		{
			delete m_ComponentContextManager;
		}
		m_StableContainers.DestroyContainers();
	}

	void Container::ValidateInternalState()
	{
		m_IsDestroyingOwnedEntities = false;
		m_CanCreateEntities = true;
		m_CanDestroyEntities = true;
		m_CanSpawn = true;
		m_CanAddComponents = true;
		m_CanRemoveComponents = true;

		m_SpawnData.Clear();

		m_ComponentRefsToInvokeObserverCallbacks.clear();
		m_DelayedEntitiesToDestroy.clear();
		m_DelayedComponentsToDestroy.clear();

		m_EmptyEntities.Clear();
	}

	Entity Container::CreateEntity(bool isActive)
	{
		if (m_CanCreateEntities)
		{
			EntityData* entityData = CreateAliveEntityData(isActive);
			Entity e(entityData, this);
			AddToEmptyEntities(*e.m_EntityData);
			InvokeEntityCreationObservers(e);
			return e;
		}
		return Entity();
	}

	bool Container::DestroyEntity(Entity& entity)
	{
		if (entity.IsValid())
		{
			DestroyEntityInternal(entity);
		}
		return false;
	}

	void Container::DestroyOwnedEntities(bool invokeOnDestroyListeners)
	{
		if (m_IsDestroyingOwnedEntities) return;
		BoolSwitch isDestroyingEntitesFlag(m_IsDestroyingOwnedEntities, true);
		BoolSwitch canCreateSwitch(m_CanCreateEntities, false);
		BoolSwitch canDestroySwitch(m_CanDestroyEntities, false);
		BoolSwitch canSpawnSwitch(m_CanSpawn, false);
		BoolSwitch canAddComponentSwitch(m_CanAddComponents, false);
		BoolSwitch canRemoveComponentSwitch(m_CanRemoveComponents, false);

		auto& archetypes = m_ArchetypesMap.m_Archetypes;

		uint64_t archetypesCount = archetypes.Size();
		for (uint64_t archetypeIdx = 0; archetypeIdx < archetypesCount; archetypeIdx++)
		{
			Archetype& archetype = archetypes[archetypeIdx];
			DestroyEntitesInArchetypes(archetype, invokeOnDestroyListeners);
			archetype.Reset();
		}

		Entity entity = {};
		for (uint64_t i = 0; i < m_EmptyEntities.Size(); i++)
		{
			EntityData& data = *m_EmptyEntities[i];
			entity.Set(data, this);
			data.SetState(decs::EntityState::InDestruction);
			if (invokeOnDestroyListeners)
			{
				InvokeEntityDestructionObservers(entity);
			}
			m_EntityManager->DestroyEntityInternal(data);
		}

		m_EmptyEntities.Clear();
		m_StableContainers.ClearContainers();
	}

	bool Container::DestroyEntityInternal(Entity& entity)
	{
		if (m_CanDestroyEntities && entity.m_Container == this)
		{
			EntityID entityID = entity.GetID();
			EntityData& entityData = *entity.m_EntityData;
			if (!entityData.CanBeDestructed())
			{
				return false;
			}

			if (m_PerformDelayedDestruction)
			{
				if (entityData.IsDelayedToDestruction()) { return false; }
				AddEntityToDelayedDestroy(entity);
				return true;
			}
			else
			{
				entityData.SetState(EntityState::InDestruction);
			}

			InvokeEntityDestructionObservers(entity);

			Archetype* currentArchetype = entityData.m_Archetype;
			if (currentArchetype != nullptr)
			{
				const uint32_t indexInArchetype = entityData.m_IndexInArchetype;

				// Invoke On destroy methods
				InvokeEntityComponentDestructionObservers(entity);

				// Remove entity from component bucket:
				auto result = currentArchetype->RemoveSwapBackEntity(indexInArchetype);
				ValidateEntityInArchetype(result);
			}
			else
			{
				RemoveFromEmptyEntities(entityData);
			}

			m_EntityManager->DestroyEntityInternal(entityData);
			return true;
		}

		return false;
	}

	void Container::SetEntityActive(Entity& entity, bool isActive)
	{
		if (entity.m_Container == this && entity.m_EntityData->IsAlive() && entity.m_EntityData->m_bIsActive != isActive)
		{
			entity.m_EntityData->SetActiveState(isActive);
			if (isActive)
			{
				InvokeEntityActivationObservers(entity);
			}
			else
			{
				InvokeEntityDeactivationObservers(entity);
			}
		}
	}

	void Container::InvokeEntityComponentDestructionObservers(Entity& entity)
	{
		Archetype* currentArchetype = entity.m_EntityData->m_Archetype;
		const uint32_t componentsCount = currentArchetype->ComponentsCount();
		const uint32_t indexInArchetype = entity.m_EntityData->m_IndexInArchetype;

		// Invoke On destroy methods
		for (uint64_t i = 0; i < componentsCount; i++)
		{
			ArchetypeTypeData& typeData = currentArchetype->m_TypeData[i];
			typeData.m_ComponentContext->InvokeOnDestroyComponent_S(typeData.m_PackedContainer->GetComponentPtrAsVoid(indexInArchetype), entity);
		}
	}

	void Container::ReserveEntities(uint32_t entitiesToReserve)
	{
		m_ReservedEntitiesCount += entitiesToReserve;
		m_ReservedEntityData.reserve(m_ReservedEntitiesCount);
		m_EntityManager->CreateReservedEntityData(entitiesToReserve, m_ReservedEntityData);
	}

	void Container::FreeReservedEntities()
	{
		m_EntityManager->ReturnReservedEntityData(m_ReservedEntityData);
	}

	Entity Container::Spawn(const Entity& prefab, bool isActive)
	{
		if (!m_CanSpawn || prefab.IsNull()) return Entity();

		Container* prefabContainer = prefab.GetContainer();
		EntityData& prefabEntityData = prefabContainer->m_EntityManager->GetEntityData(prefab.GetID());
		Archetype* prefabArchetype = prefabEntityData.m_Archetype;

		BoolSwitch prefabOperationsLock = { prefabEntityData.m_bIsUsedAsPrefab, true };

		EntityData* spawnedEntityData = CreateAliveEntityData(isActive);
		Entity spawnedEntity(spawnedEntityData, this);

		if (prefabArchetype == nullptr)
		{
			AddToEmptyEntities(*spawnedEntityData);
			InvokeEntityCreationObservers(spawnedEntity);
			return spawnedEntity;
		}

		uint64_t componentsCount = prefabArchetype->ComponentsCount();
		SpawnDataState spawnState(m_SpawnData);

		PrepareSpawnDataFromPrefab(prefabEntityData, prefabContainer);
		CreateEntityFromSpawnData(*spawnedEntityData, spawnState);

		InvokeEntityCreationObservers(spawnedEntity);

		auto& typeDataVector = m_SpawnData.m_SpawnArchetypes[spawnState.m_ArchetypeIndex]->m_TypeData;
		for (uint64_t idx = 0, compRefIdx = spawnState.m_CompRefsStart; idx < componentsCount; idx++, compRefIdx++)
		{
			typeDataVector[idx].m_ComponentContext->InvokeOnCreateComponent_S(m_SpawnData.m_EntityComponentRefs[compRefIdx].Get(), spawnedEntity);
		}

		m_SpawnData.PopBackSpawnState(spawnState.m_ArchetypeIndex, spawnState.m_CompRefsStart);

		return spawnedEntity;
	}

	bool Container::Spawn(const Entity& prefab, uint64_t spawnCount, bool areActive)
	{
		if (!m_CanSpawn || spawnCount == 0 || prefab.IsNull()) return false;

		Container* prefabContainer = prefab.GetContainer();
		EntityData& prefabEntityData = *prefab.m_EntityData;
		Archetype* prefabArchetype = prefabEntityData.m_Archetype;
		BoolSwitch prefabOperationsLock = { prefabEntityData.m_bIsUsedAsPrefab, true };

		Entity spawnedEntity;

		if (prefabArchetype == nullptr)
		{
			for (uint64_t i = 0; i < spawnCount; i++)
			{
				EntityData* entityData = CreateAliveEntityData(areActive);
				spawnedEntity.Set(
					entityData,
					this
				);
				AddToEmptyEntities(*spawnedEntity.m_EntityData);
				InvokeEntityCreationObservers(spawnedEntity);
			}
			return true;
		}

		uint64_t componentsCount = prefabArchetype->ComponentsCount();
		SpawnDataState spawnState(m_SpawnData);

		PrepareSpawnDataFromPrefab(prefabEntityData, prefabContainer);

		Archetype* spawnArchetype = m_SpawnData.m_SpawnArchetypes[spawnState.m_ArchetypeIndex];
		spawnArchetype->ReserveSpaceInArchetype(spawnArchetype->EntitiesCount() + spawnCount);

		auto& typeDataVector = m_SpawnData.m_SpawnArchetypes[spawnState.m_ArchetypeIndex]->m_TypeData;
		for (uint64_t entityIdx = 0; entityIdx < spawnCount; entityIdx++)
		{
			EntityData* entityData = CreateAliveEntityData(areActive);
			spawnedEntity.Set(
				entityData,
				this
			);

			CreateEntityFromSpawnData(*entityData, spawnState);
			InvokeEntityCreationObservers(spawnedEntity);

			for (uint64_t idx = 0, compRefIdx = spawnState.m_CompRefsStart; idx < componentsCount; idx++, compRefIdx++)
			{
				typeDataVector[idx].m_ComponentContext->InvokeOnCreateComponent_S(m_SpawnData.m_EntityComponentRefs[compRefIdx].Get(), spawnedEntity);
			}
		}

		m_SpawnData.PopBackSpawnState(spawnState.m_ArchetypeIndex, spawnState.m_CompRefsStart);

		return true;
	}

	bool Container::Spawn(const Entity& prefab, std::vector<Entity>& spawnedEntities, uint64_t spawnCount, bool areActive)
	{
		if (!m_CanSpawn || spawnCount == 0 || prefab.IsNull()) return false;

		Container* prefabContainer = prefab.GetContainer();
		EntityData& prefabEntityData = *prefab.m_EntityData;
		Archetype* prefabArchetype = prefabEntityData.m_Archetype;
		BoolSwitch prefabOperationsLock = { prefabEntityData.m_bIsUsedAsPrefab, true };

		spawnedEntities.reserve(spawnedEntities.size() + spawnCount);

		if (prefabArchetype == nullptr)
		{
			for (uint64_t i = 0; i < spawnCount; i++)
			{
				EntityData* entityData = CreateAliveEntityData(areActive);
				Entity& spawnedEntity = spawnedEntities.emplace_back(entityData, this);
				AddToEmptyEntities(*spawnedEntity.m_EntityData);
				InvokeEntityCreationObservers(spawnedEntity);
			}
			return true;
		}

		uint64_t componentsCount = prefabArchetype->ComponentsCount();
		SpawnDataState spawnState(m_SpawnData);

		PrepareSpawnDataFromPrefab(prefabEntityData, prefabContainer);

		Archetype* spawnArchetype = m_SpawnData.m_SpawnArchetypes[spawnState.m_ArchetypeIndex];
		spawnArchetype->ReserveSpaceInArchetype(spawnArchetype->EntitiesCount() + spawnCount);

		auto& typeDataVector = m_SpawnData.m_SpawnArchetypes[spawnState.m_ArchetypeIndex]->m_TypeData;
		for (uint64_t entityIdx = 0; entityIdx < spawnCount; entityIdx++)
		{
			EntityData* entityData = CreateAliveEntityData(areActive);
			Entity& spawnedEntity = spawnedEntities.emplace_back(entityData, this);

			CreateEntityFromSpawnData(*entityData, spawnState);
			InvokeEntityCreationObservers(spawnedEntity);

			for (uint64_t idx = 0, compRefIdx = spawnState.m_CompRefsStart; idx < componentsCount; idx++, compRefIdx++)
			{
				typeDataVector[idx].m_ComponentContext->InvokeOnCreateComponent_S(m_SpawnData.m_EntityComponentRefs[compRefIdx].Get(), spawnedEntity);
			}
		}

		m_SpawnData.PopBackSpawnState(spawnState.m_ArchetypeIndex, spawnState.m_CompRefsStart);

		return true;
	}

	void Container::PrepareSpawnDataFromPrefab(
		EntityData& prefabEntityData,
		Container* prefabContainer
	)
	{
		Archetype& prefabArchetype = *prefabEntityData.m_Archetype;
		Archetype* spawnedEntityArchetype = nullptr;
		uint64_t componentsCount = prefabArchetype.ComponentsCount();

		if (prefabContainer == this)
		{
			spawnedEntityArchetype = prefabEntityData.m_Archetype;
		}
		else
		{
			spawnedEntityArchetype = m_ArchetypesMap.GetOrCreateMatchedArchetype(
				*prefabEntityData.m_Archetype,
				m_ComponentContextManager,
				&m_StableContainers,
				m_ComponentContextManager->m_ObserversManager
			);
		}
		m_SpawnData.m_SpawnArchetypes.push_back(spawnedEntityArchetype);

		for (uint32_t i = 0; i < componentsCount; i++)
		{
			ArchetypeTypeData& spawnedEntityArchetypeTypeData = spawnedEntityArchetype->m_TypeData[i];

			m_SpawnData.m_PrefabComponentRefs.emplace_back(
				spawnedEntityArchetypeTypeData.m_StableContainer != nullptr,
				spawnedEntityArchetypeTypeData.m_StableContainer,
				spawnedEntityArchetypeTypeData.m_TypeID,
				prefabEntityData,
				i
			);
		}
	}

	void Container::CreateEntityFromSpawnData(
		EntityData& spawnedEntityData,
		const SpawnDataState& spawnState
	)
	{
		Archetype* archetype = m_SpawnData.m_SpawnArchetypes[spawnState.m_ArchetypeIndex];
		uint64_t componentsCount = archetype->ComponentsCount() + spawnState.m_CompRefsStart;

		spawnedEntityData.m_Archetype = archetype;
		spawnedEntityData.m_IndexInArchetype = archetype->EntitiesCount();
		archetype->AddEntityData(spawnedEntityData.m_ID, spawnedEntityData.m_bIsActive);

		auto& typeDataVector = archetype->m_TypeData;
		for (uint32_t i = spawnState.m_CompRefsStart; i < componentsCount; i++)
		{
			ArchetypeTypeData& currentTypeData = typeDataVector[i];
			SpawnComponentRefData& spawnRefData = m_SpawnData.m_PrefabComponentRefs[i];

			if (spawnRefData.m_IsStable)
			{
				StableComponentRef compNodeInfo = spawnRefData.m_StableContainer->EmplaceFromVoid(spawnRefData.m_ComponentRef.Get());
				currentTypeData.m_PackedContainer->EmplaceFromVoid(&compNodeInfo);
			}
			else
			{
				currentTypeData.m_PackedContainer->EmplaceFromVoid(spawnRefData.m_ComponentRef.Get());
			}
			m_SpawnData.m_EntityComponentRefs.emplace_back(currentTypeData.m_TypeID, spawnedEntityData, i);
		}
	}

	bool Container::RemoveUnstableComponent(Entity& entity, TypeID componentTypeID)
	{
		if (!m_CanRemoveComponents || entity.m_Container != this) return false;

		EntityData& entityData = *entity.m_EntityData;
		if (entityData.m_Archetype == nullptr || !entityData.IsValidToPerformComponentOperation()) return false;

		uint32_t compIdxInArch = entityData.m_Archetype->FindTypeIndex(componentTypeID);
		if (compIdxInArch == Limits::MaxComponentCount) return false;

		if (m_PerformDelayedDestruction)
		{
			return AddComponentToDelayedDestroy(entity.m_EntityData, componentTypeID, false);
		}

		ArchetypeTypeData& archetypeTypeData = entityData.m_Archetype->m_TypeData[compIdxInArch];
		archetypeTypeData.m_ComponentContext->InvokeOnDestroyComponent_S(
			archetypeTypeData.m_PackedContainer->GetComponentPtrAsVoid(entityData.m_IndexInArchetype),
			entity
		);

		Archetype* newEntityArchetype = m_ArchetypesMap.GetArchetypeAfterRemoveComponent(
			*entityData.m_Archetype,
			componentTypeID
		);

		uint64_t oldArchetypeIndex = entityData.m_IndexInArchetype;
		Archetype* oldArchetype = entityData.m_Archetype;
		if (newEntityArchetype != nullptr)
		{
			uint32_t entityIndexBuffor = newEntityArchetype->EntitiesCount();
			newEntityArchetype->AddEntityData(entityData.m_ID, entityData.m_bIsActive);
			newEntityArchetype->MoveEntityAfterRemoveComponent(
				componentTypeID,
				*entityData.m_Archetype,
				entityData.m_IndexInArchetype
			);

			entityData.m_IndexInArchetype = entityIndexBuffor;
			entityData.m_Archetype = newEntityArchetype;
		}
		else
		{
			entityData.m_Archetype = nullptr;
			entityData.m_IndexInArchetype = (uint32_t)m_EmptyEntities.Size();
			m_EmptyEntities.EmplaceBack(&entityData);
		}

		auto result = oldArchetype->RemoveSwapBackEntity(oldArchetypeIndex);
		ValidateEntityInArchetype(result);

		return true;
	}

	bool Container::RemoveStableComponent(Entity& entity, TypeID componentTypeID)
	{
		if (!m_CanRemoveComponents || entity.m_Container != this) return false;

		EntityData& entityData = *entity.m_EntityData;
		if (entityData.m_Archetype == nullptr || !entityData.IsValidToPerformComponentOperation()) return false;

		uint32_t compIdxInArch = entityData.m_Archetype->FindTypeIndex(componentTypeID);
		if (compIdxInArch == Limits::MaxComponentCount) return false;

		if (m_PerformDelayedDestruction)
		{
			return AddComponentToDelayedDestroy(entity.m_EntityData, componentTypeID, true);
		}

		ArchetypeTypeData& archetypeTypeData = entityData.m_Archetype->m_TypeData[compIdxInArch];
		auto& packedContainer = archetypeTypeData.m_PackedContainer;

		archetypeTypeData.m_ComponentContext->InvokeOnDestroyComponent_S(
			packedContainer->GetComponentPtrAsVoid(entityData.m_IndexInArchetype),
			entity
		);

		Archetype* newEntityArchetype = m_ArchetypesMap.GetArchetypeAfterRemoveComponent(
			*entityData.m_Archetype,
			componentTypeID
		);

		uint64_t oldArchetypeIndex = entityData.m_IndexInArchetype;
		Archetype* oldArchetype = entityData.m_Archetype;
		if (newEntityArchetype != nullptr)
		{
			uint32_t entityIndexBuffor = newEntityArchetype->EntitiesCount();
			newEntityArchetype->AddEntityData(entityData.m_ID, entityData.m_bIsActive);
			newEntityArchetype->MoveEntityAfterRemoveComponent(
				componentTypeID,
				*entityData.m_Archetype,
				entityData.m_IndexInArchetype
			);

			entityData.m_IndexInArchetype = entityIndexBuffor;
			entityData.m_Archetype = newEntityArchetype;
		}
		else
		{
			entityData.m_Archetype = nullptr;
			entityData.m_IndexInArchetype = (uint32_t)m_EmptyEntities.Size();
			m_EmptyEntities.EmplaceBack(&entityData);
		}

		StableComponentRef* componentRef = static_cast<StableComponentRef*>(packedContainer->GetComponentDataAsVoid(oldArchetypeIndex));
		archetypeTypeData.m_StableContainer->Remove(componentRef->m_ChunkIndex, componentRef->m_Index);

		auto result = oldArchetype->RemoveSwapBackEntity(oldArchetypeIndex);
		ValidateEntityInArchetype(result);

		return true;
	}

	void Container::InvokeOnCreateComponentFromEntityDataAndVoidComponentPtr(Entity& entity, ComponentContextBase* componentContext, void* componentPtr, EntityData& entityData)
	{
		componentContext->InvokeOnCreateComponent_S(componentPtr, entity);
	}

	void Container::DestroyEntitesInArchetypes(Archetype& archetype, bool invokeOnDestroyListeners)
	{
		auto& entitesData = archetype.m_EntitiesData;
		uint64_t archetypeComponentsCount = archetype.ComponentsCount();
		uint64_t entityDataCount = entitesData.size();

		if (invokeOnDestroyListeners)
		{
			Entity entity;
			for (uint64_t entityDataIdx = 0; entityDataIdx < entityDataCount; entityDataIdx++)
			{
				ArchetypeEntityData& archetypeEntityData = entitesData[entityDataIdx];
				EntityData& entityData = m_EntityManager->GetEntityData(archetypeEntityData.m_ID);
				entityData.SetState(decs::EntityState::InDestruction);

				entity.Set(archetypeEntityData.GetID(), this);

				InvokeEntityDestructionObservers(entity);

				for (uint64_t i = 0; i < archetypeComponentsCount; i++)
				{
					ArchetypeTypeData& archetypeTypeData = archetype.m_TypeData[i];
					archetypeTypeData.m_ComponentContext->InvokeOnDestroyComponent_S(
						archetypeTypeData.m_PackedContainer->GetComponentPtrAsVoid(entityDataIdx),
						entity
					);
				}

				m_EntityManager->DestroyEntityInternal(entityData);
			}
		}
		else
		{
			for (uint64_t entityDataIdx = 0; entityDataIdx < entityDataCount; entityDataIdx++)
			{
				ArchetypeEntityData& archetypeEntityData = entitesData[entityDataIdx];
				m_EntityManager->DestroyEntityInternal(archetypeEntityData.GetID());
			}
		}

	}

	bool Container::SetObserversManager(ObserversManager* observersManager)
	{
		if (m_HaveOwnComponentContextManager)
		{
			return m_ComponentContextManager->SetObserversManager(observersManager);
		}
		return false;
	}

	void Container::InvokeEntitesOnCreateListeners()
	{
		{
			BoolSwitch delayedDestroySwitch(m_PerformDelayedDestruction, true);

			auto& archetypes = m_ArchetypesMap.m_Archetypes;
			for (uint64_t i = 0; i < archetypes.Size(); i++)
			{
				archetypes[i].ValidateEntitiesCountToInitialize();
			}

			uint64_t archetypesCount = archetypes.Size();
			for (uint64_t archetypeIdx = 0; archetypeIdx < archetypesCount; archetypeIdx++)
			{
				Archetype& archetype = archetypes[archetypeIdx];
				InvokeArchetypeOnCreateListeners(archetype);
			}

			Entity entity = {};
			uint64_t emptyEntitiesSize = m_EmptyEntities.Size();
			for (uint64_t i = 0; i < emptyEntitiesSize; i++)
			{
				EntityData* data = m_EmptyEntities[i];
				entity.Set(*data, this);
				InvokeEntityCreationObservers(entity);
			}
		}

		DestroyDelayedEntities();
		DestroyDelayedComponents();
	}

	void Container::InvokeEntitesOnDestroyListeners()
	{
		if (m_IsDestroyingOwnedEntities) return;
		BoolSwitch isDestroyingEntitesFlag(m_IsDestroyingOwnedEntities, true);
		BoolSwitch canCreateSwitch(m_CanCreateEntities, false);
		BoolSwitch canDestroySwitch(m_CanDestroyEntities, false);
		BoolSwitch canSpawnSwitch(m_CanSpawn, false);
		BoolSwitch canAddComponentSwitch(m_CanAddComponents, false);
		BoolSwitch canRemoveComponentSwitch(m_CanRemoveComponents, false);

		auto& archetypes = m_ArchetypesMap.m_Archetypes;
		uint64_t archetypesCount = archetypes.Size();
		for (uint64_t archetypeIdx = 0; archetypeIdx < archetypesCount; archetypeIdx++)
		{
			Archetype& archetype = archetypes[archetypeIdx];
			InvokeArchetypeOnDestroyListeners(archetype);
		}

		Entity entity = {};
		for (uint64_t i = 0; i < m_EmptyEntities.Size(); i++)
		{
			EntityData& data = *m_EmptyEntities[i];
			entity.Set(data, this);
			InvokeEntityDestructionObservers(entity);
		}
	}

	void Container::InvokeArchetypeOnCreateListeners(Archetype& archetype)
	{
		auto& entitesData = archetype.m_EntitiesData;
		const uint64_t archetypeComponentsCount = archetype.ComponentsCount();
		const uint64_t entityDataCount = archetype.m_EntitesCountToInitialize;
		Entity entity;

		for (int64_t entityDataIdx = entityDataCount - 1; entityDataIdx > -1; entityDataIdx--)
		{
			ArchetypeEntityData& archetypeEntityData = entitesData[entityDataIdx];
			entity.Set(archetypeEntityData.GetID(), this);
			InvokeEntityCreationObservers(entity);

			// fill entity component refs, component refs are needed couse if component will be added to current entity it will chang its archetype.
			m_ComponentRefsToInvokeObserverCallbacks.clear();
			for (uint32_t i = 0; i < archetypeComponentsCount; i++)
			{
				m_ComponentRefsToInvokeObserverCallbacks.emplace_back(archetype.m_TypeData[i].m_TypeID, *entity.m_EntityData, i);
			}

			for (uint32_t i = 0; i < archetypeComponentsCount; i++)
			{
				ArchetypeTypeData& archetypeTypeData = archetype.m_TypeData[i];
				auto& compRef = m_ComponentRefsToInvokeObserverCallbacks[i];

				if (compRef)
				{
					archetypeTypeData.m_ComponentContext->InvokeOnCreateComponent_S(compRef.Get(), entity);
				}
			}
		}
	}

	void Container::InvokeArchetypeOnDestroyListeners(Archetype& archetype)
	{
		auto& entitesData = archetype.m_EntitiesData;
		uint64_t archetypeComponentsCount = archetype.ComponentsCount();
		uint64_t entityDataCount = entitesData.size();

		Entity entity;
		for (uint64_t entityDataIdx = 0; entityDataIdx < entityDataCount; entityDataIdx++)
		{
			ArchetypeEntityData& archetypeEntityData = entitesData[entityDataIdx];
			EntityData& data = m_EntityManager->GetEntityData(archetypeEntityData.m_ID);
			data.SetState(decs::EntityState::InDestruction);

			entity.Set(archetypeEntityData.GetID(), this);

			InvokeEntityDestructionObservers(entity);

			for (uint64_t i = 0; i < archetypeComponentsCount; i++)
			{
				ArchetypeTypeData& archetypeTypeData = archetype.m_TypeData[i];
				archetypeTypeData.m_ComponentContext->InvokeOnDestroyComponent_S(
					archetypeTypeData.m_PackedContainer->GetComponentPtrAsVoid(entityDataIdx),
					entity
				);
			}
		}
	}

	void Container::DestroyDelayedEntities()
	{
		Entity e = {};
		for (EntityData* entityData : m_DelayedEntitiesToDestroy)
		{
			e.Set(*entityData, this);
			DestroyDelayedEntity(e);
		}
		m_DelayedEntitiesToDestroy.clear();
	}

	void Container::DestroyDelayedEntity(Entity& entity)
	{
		Archetype* currentArchetype = entity.m_EntityData->m_Archetype;
		InvokeEntityDestructionObservers(entity);
		if (currentArchetype != nullptr)
		{
			InvokeEntityComponentDestructionObservers(entity);
			auto result = currentArchetype->RemoveSwapBackEntity(entity.m_EntityData->m_IndexInArchetype);
			ValidateEntityInArchetype(result);
		}
		else
		{
			RemoveFromEmptyEntities(*entity.m_EntityData);
		}

		m_EntityManager->DestroyEntityInternal(*entity.m_EntityData);
	}

	void Container::DestroyDelayedComponents()
	{
		Entity entity = {};
		for (auto& data : m_DelayedComponentsToDestroy)
		{
			entity.Set(*data.m_EntityData, this);

			if (data.m_IsStable)
			{
				RemoveStableComponent(entity, data.m_TypeID);
			}
			else
			{
				RemoveUnstableComponent(entity, data.m_TypeID);
			}
		}
		m_DelayedComponentsToDestroy.clear();
	}

	void Container::AddEntityToDelayedDestroy(Entity& entity)
	{
		entity.m_EntityData->SetState(EntityState::DelayedToDestruction);
		m_DelayedEntitiesToDestroy.push_back(entity.m_EntityData);
	}
}
