#pragma once
#include "Container.h"
#include "Entity.h"

namespace decs
{
	Container::Container() :
		m_HaveOwnEntityManager(true),
		m_EntityManager(new EntityManager(10000)),
		m_HaveOwnComponentContextManager(true),
		m_ComponentContextManager(new ComponentContextsManager(nullptr))
	{

	}

	Container::Container(
		const uint64_t& enititesChunkSize,
		const uint64_t& stableComponentDefaultChunkSize,
		const uint64_t& m_EmptyEntitiesChunkSize
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
		ComponentContextsManager* componentContextsManager,
		const uint64_t& stableComponentDefaultChunkSize,
		const uint64_t& m_EmptyEntitiesChunkSize
	) :
		m_HaveOwnEntityManager(false),
		m_EntityManager(entityManager),
		m_HaveOwnComponentContextManager(false),
		m_ComponentContextManager(componentContextsManager),
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

	Entity Container::CreateEntity(const bool& isActive)
	{
		if (m_CanCreateEntities)
		{
			EntityID entityID = m_EntityManager->CreateEntity(isActive);
			Entity e(entityID, this);
			AddToEmptyEntities(*e.m_EntityData);
			InvokeEntityCreationObservers(e);
			return e;
		}
		return Entity();
	}

	bool Container::DestroyEntity(Entity& entity)
	{
		if (m_CanDestroyEntities && entity.m_Container == this)
		{
			EntityID entityID = entity.ID();
			EntityData& entityData = *entity.m_EntityData;
			if (!entityData.CanBeDestructed())
			{
				return false;
			}

			if (m_PerformDelayedDestruction)
			{
				if (entityData.IsDelayedToDestruction()) { return false; }
				entityData.SetState(EntityDestructionState::DelayedToDestruction);
				AddEntityToDelayedDestroy(entity.ID());
				return true;
			}
			else
			{
				entityData.SetState(EntityDestructionState::InDestruction);
			}

			InvokeEntityDestructionObservers(entity);

			Archetype* currentArchetype = entityData.m_Archetype;
			if (currentArchetype != nullptr)
			{
				const uint32_t componentsCount = currentArchetype->ComponentsCount();
				const uint32_t indexInArchetype = entityData.m_IndexInArchetype;

				m_StableComponentDestroyData.clear();

				// Invoke On destroy methods
				for (uint64_t i = 0; i < componentsCount; i++)
				{
					ArchetypeTypeData& typeData = currentArchetype->m_TypeData[i];
					typeData.m_ComponentContext->InvokeOnDestroyComponent_S(typeData.m_PackedContainer->GetComponentPtrAsVoid(indexInArchetype), entity);

					if (typeData.m_StableContainer != nullptr)
					{
						StableComponentRef* compRef = static_cast<StableComponentRef*>(typeData.m_PackedContainer->GetComponentDataAsVoid(indexInArchetype));
						m_StableComponentDestroyData.emplace_back(typeData.m_StableContainer, compRef->m_ChunkIndex, compRef->m_ElementIndex);
					}
				}

				uint64_t m_StableComponentsCount = m_StableComponentDestroyData.size();
				for (uint64_t i = 0; i < m_StableComponentsCount; i++)
				{
					StableComponentDestroyData& destroyData = m_StableComponentDestroyData[i];
					destroyData.m_StableContainer->Remove(destroyData.m_ChunkIndex, destroyData.m_ElementIndex);
				}

				// Remove entity from component bucket:
				auto result = currentArchetype->RemoveSwapBackEntity(indexInArchetype);
				ValidateEntityInArchetype(result);
			}
			else
			{
				RemoveFromEmptyEntities(entityData);
			}

			m_EntityManager->DestroyEntity(entityID);
			return true;
		}

		return false;
	}

	void Container::DestroyOwnedEntities(const bool& invokeOnDestroyListeners)
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
			data.SetState(decs::EntityDestructionState::InDestruction);
			if (invokeOnDestroyListeners)
			{
				InvokeEntityDestructionObservers(entity);
			}
			m_EntityManager->DestroyEntity(data);
		}
		m_EmptyEntities.Clear();

		m_StableContainers.ClearContainers();
	}

	void Container::SetEntityActive(const EntityID& entity, const bool& isActive)
	{
		if (m_EntityManager->SetEntityActive(entity, isActive))
		{
			Entity e{ entity , this };
			if (isActive)
				InvokeEntityActivationObservers(e);
			else
				InvokeEntityDeactivationObservers(e);
		}
	}

	Entity Container::Spawn(const Entity& prefab, const bool& isActive)
	{
		if (!m_CanSpawn || prefab.IsNull()) return Entity();

		Container* prefabContainer = prefab.GetContainer();
		EntityData& prefabEntityData = prefabContainer->m_EntityManager->GetEntityData(prefab.ID());
		Archetype* prefabArchetype = prefabEntityData.m_Archetype;

		BoolSwitch prefabOperationsLock = { prefabEntityData.m_IsUsedAsPrefab, true };

		EntityID entityID = m_EntityManager->CreateEntity(isActive);
		Entity spawnedEntity(entityID, this);
		EntityData& spawnedEntityData = *spawnedEntity.m_EntityData;

		if (prefabArchetype == nullptr)
		{
			AddToEmptyEntities(spawnedEntityData);
			InvokeEntityCreationObservers(spawnedEntity);
			return spawnedEntity;
		}

		uint64_t componentsCount = prefabArchetype->ComponentsCount();
		SpawnDataState spawnState(m_SpawnData);

		PrepareSpawnDataFromPrefab(prefabEntityData, prefabContainer);
		CreateEntityFromSpawnData(spawnedEntityData, spawnState);

		InvokeEntityCreationObservers(spawnedEntity);

		auto& typeDataVector = m_SpawnData.m_SpawnArchetypes[spawnState.m_ArchetypeIndex]->m_TypeData;
		for (uint64_t idx = 0, compRefIdx = spawnState.m_CompRefsStart; idx < componentsCount; idx++, compRefIdx++)
		{
			typeDataVector[idx].m_ComponentContext->InvokeOnCreateComponent_S(m_SpawnData.m_EntityComponentRefs[compRefIdx].Get(), spawnedEntity);
		}

		m_SpawnData.PopBackSpawnState(spawnState.m_ArchetypeIndex, spawnState.m_CompRefsStart);

		return spawnedEntity;
	}

	bool Container::Spawn(const Entity& prefab, const uint64_t& spawnCount, const bool& areActive)
	{
		if (!m_CanSpawn || spawnCount == 0 || prefab.IsNull()) return false;

		Container* prefabContainer = prefab.GetContainer();
		EntityData& prefabEntityData = *prefab.m_EntityData;
		Archetype* prefabArchetype = prefabEntityData.m_Archetype;
		BoolSwitch prefabOperationsLock = { prefabEntityData.m_IsUsedAsPrefab, true };

		Entity spawnedEntity;

		if (prefabArchetype == nullptr)
		{
			for (uint64_t i = 0; i < spawnCount; i++)
			{
				EntityID entityId = m_EntityManager->CreateEntity(areActive);
				spawnedEntity.Set(
					entityId,
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
			EntityID entityID = m_EntityManager->CreateEntity(areActive);
			spawnedEntity.Set(
				entityID,
				this
			);

			CreateEntityFromSpawnData(*spawnedEntity.m_EntityData, spawnState);
			InvokeEntityCreationObservers(spawnedEntity);

			for (uint64_t idx = 0, compRefIdx = spawnState.m_CompRefsStart; idx < componentsCount; idx++, compRefIdx++)
			{
				typeDataVector[idx].m_ComponentContext->InvokeOnCreateComponent_S(m_SpawnData.m_EntityComponentRefs[compRefIdx].Get(), spawnedEntity);
			}
		}

		m_SpawnData.PopBackSpawnState(spawnState.m_ArchetypeIndex, spawnState.m_CompRefsStart);

		return true;
	}

	bool Container::Spawn(const Entity& prefab, std::vector<Entity>& spawnedEntities, const uint64_t& spawnCount, const bool& areActive)
	{
		if (!m_CanSpawn || spawnCount == 0 || prefab.IsNull()) return false;

		Container* prefabContainer = prefab.GetContainer();
		EntityData& prefabEntityData = *prefab.m_EntityData;
		Archetype* prefabArchetype = prefabEntityData.m_Archetype;
		BoolSwitch prefabOperationsLock = { prefabEntityData.m_IsUsedAsPrefab, true };

		spawnedEntities.reserve(spawnedEntities.size() + spawnCount);

		if (prefabArchetype == nullptr)
		{
			for (uint64_t i = 0; i < spawnCount; i++)
			{
				EntityID entityID = m_EntityManager->CreateEntity(areActive);
				Entity& spawnedEntity = spawnedEntities.emplace_back(entityID, this);
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
			EntityID entityID = m_EntityManager->CreateEntity(areActive);
			Entity& spawnedEntity = spawnedEntities.emplace_back(entityID, this);

			CreateEntityFromSpawnData(*spawnedEntity.m_EntityData, spawnState);
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
		uint64_t componentsCount = prefabArchetype.ComponentsCount();

		if (prefabContainer == this)
		{
			m_SpawnData.m_SpawnArchetypes.push_back(prefabEntityData.m_Archetype);
		}
		else
		{
			m_SpawnData.m_SpawnArchetypes.push_back(m_ArchetypesMap.FindArchetypeFromOther(
				*prefabEntityData.m_Archetype,
				m_ComponentContextManager,
				&m_StableContainers,
				m_ObserversManager
			));
		}

		for (uint32_t i = 0; i < componentsCount; i++)
		{
			m_SpawnData.m_PrefabComponentRefs.emplace_back(
				prefabArchetype.GetTypeID(i),
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
		archetype->AddEntityData(spawnedEntityData.m_ID, spawnedEntityData.m_IsActive);

		auto& typeDataVector = archetype->m_TypeData;
		for (uint32_t i = spawnState.m_CompRefsStart; i < componentsCount; i++)
		{
			ArchetypeTypeData& currentTypeData = typeDataVector[i];
			currentTypeData.m_PackedContainer->EmplaceFromVoid(m_SpawnData.m_PrefabComponentRefs[i].Get());
			m_SpawnData.m_EntityComponentRefs.emplace_back(currentTypeData.m_TypeID, spawnedEntityData, i);
		}
	}

	bool Container::RemoveUnstableComponent(Entity& entity, const TypeID& componentTypeID)
	{
		if (!m_CanRemoveComponents || entity.m_Container != this) return false;

		EntityData& entityData = *entity.m_EntityData;
		if (entityData.m_Archetype == nullptr || !entityData.IsValidToPerformComponentOperation()) return false;

		uint32_t compIdxInArch = entityData.m_Archetype->FindTypeIndex(componentTypeID);
		if (compIdxInArch == Limits::MaxComponentCount) return false;

		ArchetypeTypeData& archetypeTypeData = entityData.m_Archetype->m_TypeData[compIdxInArch];
		ComponentContextBase* componentContext = archetypeTypeData.m_ComponentContext;
		auto& container = archetypeTypeData.m_PackedContainer;

		if (m_PerformDelayedDestruction)
		{
			return AddComponentToDelayedDestroy(entity.ID(), componentTypeID);
		}

		componentContext->InvokeOnDestroyComponent_S(
			container->GetComponentPtrAsVoid(entityData.m_IndexInArchetype),
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
			newEntityArchetype->AddEntityData(entityData.m_ID, entityData.m_IsActive);
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

	bool Container::RemoveUnstableComponent(const EntityID& e, const TypeID& componentTypeID)
	{
		Entity entity(e, this);
		return RemoveUnstableComponent(entity, componentTypeID);
	}

	bool Container::RemoveStableComponent(Entity& entity, const TypeID& componentTypeID)
	{
		if (!m_CanRemoveComponents || entity.m_Container != this) return false;

		EntityData& entityData = *entity.m_EntityData;
		if (entityData.m_Archetype == nullptr || !entityData.IsValidToPerformComponentOperation()) return false;

		uint32_t compIdxInArch = entityData.m_Archetype->FindTypeIndex(componentTypeID);
		if (compIdxInArch == Limits::MaxComponentCount) return false;

		ArchetypeTypeData& archetypeTypeData = entityData.m_Archetype->m_TypeData[compIdxInArch];
		ComponentContextBase* componentContext = archetypeTypeData.m_ComponentContext;
		auto& container = archetypeTypeData.m_PackedContainer;

		if (m_PerformDelayedDestruction)
		{
			return AddComponentToDelayedDestroy(entity.ID(), componentTypeID);
		}

		componentContext->InvokeOnDestroyComponent_S(
			container->GetComponentPtrAsVoid(entityData.m_IndexInArchetype),
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
			newEntityArchetype->AddEntityData(entityData.m_ID, entityData.m_IsActive);
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

		StableComponentRef* componentRef = static_cast<StableComponentRef*>(container->GetComponentDataAsVoid(oldArchetypeIndex));
		StableContainerBase* stableContainer = m_StableContainers.GetStableContainer(componentTypeID);
		stableContainer->Remove(componentRef->m_ChunkIndex, componentRef->m_ElementIndex);

		auto result = oldArchetype->RemoveSwapBackEntity(oldArchetypeIndex);
		ValidateEntityInArchetype(result);

		return true;
	}

	bool Container::RemoveStableComponent(const EntityID& e, const TypeID& componentTypeID)
	{
		Entity entity(e, this);
		return RemoveStableComponent(entity, componentTypeID);
	}

	void Container::InvokeOnCreateComponentFromEntityDataAndVoidComponentPtr(ComponentContextBase* componentContext, void* componentPtr, EntityData& entityData)
	{
		Entity e = { entityData, this };
		componentContext->InvokeOnCreateComponent_S(componentPtr, e);
	}

	void Container::DestroyEntitesInArchetypes(Archetype& archetype, const bool& invokeOnDestroyListeners)
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
				EntityData& data = m_EntityManager->GetEntityData(archetypeEntityData.m_ID);
				data.SetState(decs::EntityDestructionState::InDestruction);

				entity.Set(archetypeEntityData.ID(), this);

				InvokeEntityDestructionObservers(entity);

				for (uint64_t i = 0; i < archetypeComponentsCount; i++)
				{
					ArchetypeTypeData& archetypeTypeData = archetype.m_TypeData[i];
					archetypeTypeData.m_ComponentContext->InvokeOnDestroyComponent_S(
						archetypeTypeData.m_PackedContainer->GetComponentPtrAsVoid(entityDataIdx),
						entity
					);
				}

				m_EntityManager->DestroyEntity(archetypeEntityData.ID());
			}
		}
		else
		{
			for (uint64_t entityDataIdx = 0; entityDataIdx < entityDataCount; entityDataIdx++)
			{
				ArchetypeEntityData& archetypeEntityData = entitesData[entityDataIdx];
				m_EntityManager->DestroyEntity(archetypeEntityData.ID());
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
			entity.Set(archetypeEntityData.ID(), this);

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
			data.SetState(decs::EntityDestructionState::InDestruction);

			entity.Set(archetypeEntityData.ID(), this);

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
		for (auto& entityID : m_DelayedEntitiesToDestroy)
		{
			e.Set(entityID, this);
			e.Destroy();
		}
		m_DelayedEntitiesToDestroy.clear();
	}

	void Container::DestroyDelayedComponents()
	{
		for (auto& data : m_DelayedComponentsToDestroy)
		{
			//RemoveComponent(data.m_EntityID, data.m_TypeID);
		}
		m_DelayedComponentsToDestroy.clear();
	}

	void Container::AddEntityToDelayedDestroy(const Entity& entity)
	{
		m_DelayedEntitiesToDestroy.push_back(entity.ID());
	}

	void Container::AddEntityToDelayedDestroy(const EntityID& entityID)
	{
		m_DelayedEntitiesToDestroy.push_back(entityID);
	}

}
