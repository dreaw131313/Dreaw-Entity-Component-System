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
		uint32_t stableComponentDefaultChunkSize,
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
		uint32_t stableComponentDefaultChunkSize,
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
			ReturnOwnedEntitiesToEntityManager();
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

		m_DelayedEntitiesToDestroy.clear();
	}

	Entity Container::CreateEntity(bool isActive)
	{
		if (m_CanCreateEntities)
		{
			EntityData* entityData = CreateAliveEntityData(isActive);
			Entity e(entityData, this);
			AddToEmptyEntitiesRightAfterNewEntityCreation(*e.m_EntityData);
			InvokeEntityCreationObservers(e);
			m_EntiesCount += 1;
			return e;
		}
		return Entity();
	}

	bool Container::DestroyEntity(const Entity& entity)
	{
		if (entity.IsValid())
		{
			return DestroyEntityInternal(entity);
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
			m_EntityManager->DestroyEntity(data);
		}

		m_EmptyEntities.Clear();
		m_StableContainers.ClearContainers();

		m_EntiesCount = 0;
	}

	bool Container::DestroyEntityInternal(Entity entity)
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
				currentArchetype->RemoveSwapBackEntity(indexInArchetype);
			}
			else
			{
				RemoveFromEmptyEntities(entityData);
			}

			m_EntityManager->DestroyEntity(entityData);

			m_EntiesCount -= 1;

			return true;
		}

		return false;
	}

	void Container::SetEntityActive( const Entity& entity, bool isActive)
	{
		if (entity.m_Container == this && entity.m_EntityData->IsAlive() && entity.m_EntityData->IsActive() != isActive)
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

	void Container::AddToEmptyEntitiesRightAfterNewEntityCreation(EntityData& data)
	{
		data.m_Archetype = nullptr;
		data.m_IndexInArchetype = (uint32_t)m_EmptyEntities.Size();
		m_EmptyEntities.EmplaceBack(&data);
	}

	void Container::AddToEmptyEntities(EntityData& data)
	{
		if (data.m_Archetype == nullptr)
		{
			return;
		}

		data.m_Archetype = nullptr;
		data.m_IndexInArchetype = (uint32_t)m_EmptyEntities.Size();
		m_EmptyEntities.EmplaceBack(&data);
	}

	void Container::RemoveFromEmptyEntities(EntityData& data)
	{
		if (data.m_Archetype != nullptr)
		{
			return;
		}

		if (data.m_IndexInArchetype < m_EmptyEntities.Size() - 1)
		{
			m_EmptyEntities[data.m_IndexInArchetype] = m_EmptyEntities.Back();
			m_EmptyEntities.Back()->m_IndexInArchetype = data.m_IndexInArchetype;
		}

		m_EmptyEntities.PopBack();
		data.m_IndexInArchetype = std::numeric_limits<uint32_t>::max();
	}

	void Container::InvokeEntityComponentDestructionObservers(const Entity& entity)
	{
		Archetype* currentArchetype = entity.m_EntityData->m_Archetype;
		const uint32_t componentsCount = currentArchetype->ComponentCount();
		const uint32_t indexInArchetype = entity.m_EntityData->m_IndexInArchetype;

		auto& typeDatas = currentArchetype->m_TypeData;
		auto& orderDatas = currentArchetype->m_ComponentContextsInOrder;

		// Invoke On destroy methods
		for (uint64_t i = 0; i < componentsCount; i++)
		{
			const auto& orderData = orderDatas[i];
			ArchetypeTypeData& typeData = typeDatas[orderData.m_ComponentIndex];
			typeData.m_ComponentContext->InvokeOnDestroyComponent(typeData.m_PackedContainer->GetComponentPtrAsVoid(indexInArchetype), entity);
		}
	}

	EntityData* Container::CreateAliveEntityData(bool bIsActive)
	{
		if (m_ReservedEntitiesCount > 0)
		{
			m_ReservedEntitiesCount -= 1;
			EntityData* data = m_ReservedEntityData.back();
			m_ReservedEntityData.pop_back();
			m_EntityManager->CreateEntityFromReservedEntityData(data, bIsActive);
			return data;
		}
		else
		{
			return m_EntityManager->CreateEntity(bIsActive);
		}
	}

	void Container::ReturnOwnedEntitiesToEntityManager()
	{
		DestroyOwnedEntities(false);

		//TODO: add returning reserved entities
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
		EntityData& prefabEntityData = *prefab.m_EntityData;
		Archetype* prefabArchetype = prefabEntityData.m_Archetype;

		BoolSwitch prefabOperationsLock = { prefabEntityData.m_bIsUsedAsPrefab, true };

		EntityData* spawnedEntityData = CreateAliveEntityData(isActive);
		Entity spawnedEntity(spawnedEntityData, this);

		if (prefabArchetype == nullptr)
		{
			AddToEmptyEntitiesRightAfterNewEntityCreation(*spawnedEntityData);
			InvokeEntityCreationObservers(spawnedEntity);
			return spawnedEntity;
		}

		uint64_t componentsCount = prefabArchetype->ComponentCount();
		SpawnDataState spawnState(m_SpawnData);

		PrepareSpawnDataFromPrefab(prefabEntityData, prefabContainer);
		CreateEntityFromSpawnData(*spawnedEntityData, spawnState);

		InvokeEntityCreationObservers(spawnedEntity);
		InvokeOnCreateObserversOnSpawn(spawnedEntity, m_SpawnData.m_SpawnArchetypes[spawnState.m_ArchetypeIndex], componentsCount, spawnState);

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
				AddToEmptyEntitiesRightAfterNewEntityCreation(*spawnedEntity.m_EntityData);
				InvokeEntityCreationObservers(spawnedEntity);
			}
			return true;
		}

		uint64_t componentsCount = prefabArchetype->ComponentCount();
		SpawnDataState spawnState(m_SpawnData);

		PrepareSpawnDataFromPrefab(prefabEntityData, prefabContainer);

		Archetype* spawnArchetype = m_SpawnData.m_SpawnArchetypes[spawnState.m_ArchetypeIndex];
		spawnArchetype->ReserveSpaceInArchetype(spawnArchetype->EntityCount() + spawnCount);

		for (uint64_t entityIdx = 0; entityIdx < spawnCount; entityIdx++)
		{
			EntityData* entityData = CreateAliveEntityData(areActive);
			spawnedEntity.Set(
				entityData,
				this
			);

			CreateEntityFromSpawnData(*entityData, spawnState);
			InvokeEntityCreationObservers(spawnedEntity);
			InvokeOnCreateObserversOnSpawn(spawnedEntity, spawnArchetype, componentsCount, spawnState);
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
				AddToEmptyEntitiesRightAfterNewEntityCreation(*spawnedEntity.m_EntityData);
				InvokeEntityCreationObservers(spawnedEntity);
			}
			return true;
		}

		uint64_t componentsCount = prefabArchetype->ComponentCount();
		SpawnDataState spawnState(m_SpawnData);

		PrepareSpawnDataFromPrefab(prefabEntityData, prefabContainer);

		Archetype* spawnArchetype = m_SpawnData.m_SpawnArchetypes[spawnState.m_ArchetypeIndex];
		spawnArchetype->ReserveSpaceInArchetype(spawnArchetype->EntityCount() + spawnCount);

		for (uint64_t entityIdx = 0; entityIdx < spawnCount; entityIdx++)
		{
			EntityData* entityData = CreateAliveEntityData(areActive);
			Entity& spawnedEntity = spawnedEntities.emplace_back(entityData, this);

			CreateEntityFromSpawnData(*entityData, spawnState);
			InvokeEntityCreationObservers(spawnedEntity);
			InvokeOnCreateObserversOnSpawn(spawnedEntity, spawnArchetype, componentsCount, spawnState);
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
		uint64_t componentsCount = prefabArchetype.ComponentCount();

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

		m_SpawnData.m_SpawnedEntityComponentRefs.resize(m_SpawnData.m_SpawnedEntityComponentRefs.size() + componentsCount);
	}

	void Container::CreateEntityFromSpawnData(
		EntityData& spawnedEntityData,
		const SpawnDataState& spawnState
	)
	{
		Archetype* archetype = m_SpawnData.m_SpawnArchetypes[spawnState.m_ArchetypeIndex];
		uint64_t componentsCount = archetype->ComponentCount() + spawnState.m_CompRefsStart;

		archetype->AddEntityData(&spawnedEntityData);

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
			m_SpawnData.m_SpawnedEntityComponentRefs[i].Set(currentTypeData.m_TypeID, spawnedEntityData, i);
		}
	}

	void Container::InvokeOnCreateObserversOnSpawn(const Entity& entity, Archetype* archetype, uint64_t componentsCount, const SpawnDataState& spawnState)
	{
		auto& orderContextVector = archetype->m_ComponentContextsInOrder;
		for (uint64_t idx = 0, compRefIdx = spawnState.m_CompRefsStart; idx < componentsCount; idx++)
		{
			auto& orderData = orderContextVector[idx];
			uint32_t componentIdx = orderData.m_ComponentIndex;

			auto& componentRef = m_SpawnData.m_SpawnedEntityComponentRefs[compRefIdx + componentIdx];
			void* componentVoidPtr = componentRef.Get();
			if (componentVoidPtr != nullptr)
			{
				orderData.m_ComponentContext->InvokeOnCreateComponent(componentVoidPtr, entity);
			}
		}
	}

	bool Container::RemoveComponent(const Entity& entity, TypeID componentTypeID)
	{
		if (entity.m_Container != this) return false;

		EntityData& entityData = *entity.m_EntityData;
		if (entityData.m_Archetype == nullptr || !entityData.IsValidToPerformComponentOperation()) return false;

		uint32_t compIdxInArch = entityData.m_Archetype->FindTypeIndex(componentTypeID);
		if (compIdxInArch == Limits::MaxComponentCount) return false;

		Archetype* oldArchetype = entityData.m_Archetype;
		uint64_t entityIndexInOldArchetype = entityData.m_IndexInArchetype;

		ArchetypeTypeData& archetypeTypeData = oldArchetype->m_TypeData[compIdxInArch];
		auto& packedContainer = archetypeTypeData.m_PackedContainer;

		Archetype* newEntityArchetype = m_ArchetypesMap.GetArchetypeAfterRemoveComponent(
			*entityData.m_Archetype,
			componentTypeID
		);

		if (newEntityArchetype != nullptr)
		{
			newEntityArchetype->MoveEntityAfterRemoveComponentWithoutDestroyingFromSource(
				componentTypeID,
				entityData.m_Archetype,
				entityData.m_IndexInArchetype,
				&entityData
			);
		}
		else
		{
			AddToEmptyEntities(entityData);
		}

		archetypeTypeData.m_ComponentContext->InvokeOnDestroyComponent(
			packedContainer->GetComponentPtrAsVoid(entityIndexInOldArchetype),
			entity
		);

		oldArchetype->RemoveSwapBackEntityAfterMoveEntityWithoutDestroyingSource(entityIndexInOldArchetype, componentTypeID);

		return true;
	}

	void Container::DestroyEntitesInArchetypes(Archetype& archetype, bool invokeOnDestroyListeners)
	{
		auto& entitesData = archetype.m_EntitiesData;
		uint64_t archetypeComponentsCount = archetype.ComponentCount();
		uint64_t entityDataCount = entitesData.size();

		if (invokeOnDestroyListeners)
		{
			Entity entity;
			for (uint64_t entityDataIdx = 0; entityDataIdx < entityDataCount; entityDataIdx++)
			{
				ArchetypeEntityData& archetypeEntityData = entitesData[entityDataIdx];
				EntityData& entityData = *archetypeEntityData.m_EntityData;
				entityData.SetState(decs::EntityState::InDestruction);

				entity.Set(archetypeEntityData.m_EntityData, this);

				InvokeEntityDestructionObservers(entity);

				for (uint64_t i = 0; i < archetypeComponentsCount; i++)
				{
					auto& orderData = archetype.m_ComponentContextsInOrder[i];
					ArchetypeTypeData& archetypeTypeData = archetype.m_TypeData[orderData.m_ComponentIndex];
					archetypeTypeData.m_ComponentContext->InvokeOnDestroyComponent(
						archetypeTypeData.m_PackedContainer->GetComponentPtrAsVoid(entityDataIdx),
						entity
					);
				}

				m_EntityManager->DestroyEntity(entityData);
			}
		}
		else
		{
			for (uint64_t entityDataIdx = 0; entityDataIdx < entityDataCount; entityDataIdx++)
			{
				ArchetypeEntityData& archetypeEntityData = entitesData[entityDataIdx];
				m_EntityManager->DestroyEntity(*archetypeEntityData.GetEntityData());
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
			BoolSwitch canRemoveComponentSwitch(m_CanRemoveComponents, true);

			std::vector<ComponentRefAsVoid> componentRefsToInvokeObserverCallbacks = {};

			auto& archetypes = m_ArchetypesMap.m_Archetypes;
			for (uint64_t i = 0; i < archetypes.Size(); i++)
			{
				archetypes[i].ValidateEntitiesCountToInitialize();
			}

			uint64_t archetypesCount = archetypes.Size();
			for (uint64_t archetypeIdx = 0; archetypeIdx < archetypesCount; archetypeIdx++)
			{
				Archetype& archetype = archetypes[archetypeIdx];
				InvokeArchetypeOnCreateListeners(archetype, componentRefsToInvokeObserverCallbacks);
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

	void Container::InvokeEntityActivationObservers(const Entity& entity)
	{
		if (m_ComponentContextManager->m_ObserversManager != nullptr)
		{
			m_ComponentContextManager->m_ObserversManager->InvokeEntityActivationObservers(entity);
		}

		// TODO: add components activation listeners invoking

		EntityData& entityData = *entity.m_EntityData;
		if (entityData.m_Archetype != nullptr)
		{
			uint64_t startRefsIdx = m_ActivationChangeComponentRefs.size();
			uint64_t refCount = entityData.m_Archetype->ComponentCount();

			m_ActivationChangeComponentRefs.reserve(startRefsIdx + refCount);

			// fetch components refs
			auto& componentsTypeData = entityData.m_Archetype->m_TypeData;
			for (uint64_t idx = 0; idx < refCount; idx++)
			{
				auto& typeData = componentsTypeData[idx];
				m_ActivationChangeComponentRefs.emplace_back(typeData.m_TypeID, entityData, static_cast<uint32_t>(idx));
			}

			// invoke components activation listeners:
			auto& componentOrderData = entityData.m_Archetype->m_ComponentContextsInOrder;
			for (uint64_t idx = 0; idx < refCount; idx++)
			{
				auto& orderData = componentOrderData[idx];
				auto& compRef = m_ActivationChangeComponentRefs[startRefsIdx + orderData.m_ComponentIndex];
				void* compPtr = compRef.Get();
				if (compPtr != nullptr)
				{
					// invoke activation listener:
					orderData.m_ComponentContext->InvokeOnEnableEntity(compPtr, entity);
				}
			}

			// erase used component refs:
			m_ActivationChangeComponentRefs.erase(m_ActivationChangeComponentRefs.begin() + startRefsIdx, m_ActivationChangeComponentRefs.end());
		}
	}

	void Container::InvokeEntityDeactivationObservers(const Entity& entity)
	{
		if (m_ComponentContextManager->m_ObserversManager != nullptr)
		{
			m_ComponentContextManager->m_ObserversManager->InvokeEntityDeactivationObservers(entity);
		}

		// TODO: add components deactivation listeners invoking
		EntityData& entityData = *entity.m_EntityData;
		if (entityData.m_Archetype != nullptr)
		{
			uint64_t startRefsIdx = m_ActivationChangeComponentRefs.size();
			uint64_t refCount = entityData.m_Archetype->ComponentCount();

			m_ActivationChangeComponentRefs.reserve(startRefsIdx + refCount);

			// fetch components refs
			auto& componentsTypeData = entityData.m_Archetype->m_TypeData;
			for (uint64_t idx = 0; idx < refCount; idx++)
			{
				auto& typeData = componentsTypeData[idx];
				m_ActivationChangeComponentRefs.emplace_back(typeData.m_TypeID, entityData, static_cast<uint32_t>(idx));
			}

			// invoke components activation listeners:
			auto& componentOrderData = entityData.m_Archetype->m_ComponentContextsInOrder;
			for (uint64_t idx = 0; idx < refCount; idx++)
			{
				auto& orderData = componentOrderData[idx];
				auto& compRef = m_ActivationChangeComponentRefs[startRefsIdx + orderData.m_ComponentIndex];
				void* compPtr = compRef.Get();
				if (compPtr != nullptr)
				{
					orderData.m_ComponentContext->InvokeOnOnDisableEntity(compPtr, entity);
				}
			}

			// erase used component refs:
			m_ActivationChangeComponentRefs.erase(m_ActivationChangeComponentRefs.begin() + startRefsIdx, m_ActivationChangeComponentRefs.end());
		}
	}

	void Container::InvokeArchetypeOnCreateListeners(Archetype& archetype, std::vector<ComponentRefAsVoid>& componentRefsToInvokeObserverCallbacks)
	{
		auto& entitesData = archetype.m_EntitiesData;
		const uint64_t archetypeComponentsCount = archetype.ComponentCount();
		const uint64_t entityDataCount = archetype.m_EntitesCountToInitialize;
		Entity entity;

		componentRefsToInvokeObserverCallbacks.reserve(archetype.ComponentCount());

		for (int64_t entityDataIdx = entityDataCount - 1; entityDataIdx > -1; entityDataIdx--)
		{
			ArchetypeEntityData& archetypeEntityData = entitesData[entityDataIdx];
			entity.Set(archetypeEntityData.GetEntityData(), this);
			InvokeEntityCreationObservers(entity);

			auto& typeDatas = archetype.m_TypeData;
			auto& orderDatas = archetype.m_ComponentContextsInOrder;

			// fill entity component refs, component refs are needed couse if component will be added to current entity it will chang its archetype.
			componentRefsToInvokeObserverCallbacks.clear();
			for (uint32_t i = 0; i < archetypeComponentsCount; i++)
			{
				componentRefsToInvokeObserverCallbacks.emplace_back(typeDatas[i].m_TypeID, *entity.m_EntityData, i);
			}

			// Invoke On destroy methods
			for (uint64_t i = 0; i < archetypeComponentsCount; i++)
			{
				const auto& orderData = orderDatas[i];
				auto& compRef = componentRefsToInvokeObserverCallbacks[orderData.m_ComponentIndex];
				void* componentVoidPtr = compRef.Get();
				if (componentVoidPtr != nullptr)
				{
					orderData.m_ComponentContext->InvokeOnCreateComponent(componentVoidPtr, entity);
				}
			}
		}
	}

	void Container::InvokeArchetypeOnDestroyListeners(Archetype& archetype)
	{
		auto& entitesData = archetype.m_EntitiesData;
		uint64_t archetypeComponentsCount = archetype.ComponentCount();
		uint64_t entityDataCount = entitesData.size();

		Entity entity;
		for (uint64_t entityDataIdx = 0; entityDataIdx < entityDataCount; entityDataIdx++)
		{
			ArchetypeEntityData& archetypeEntityData = entitesData[entityDataIdx];
			EntityData& data = *archetypeEntityData.GetEntityData();
			data.SetState(decs::EntityState::InDestruction);

			entity.Set(archetypeEntityData.GetEntityData(), this);

			InvokeEntityDestructionObservers(entity);

			for (uint64_t i = 0; i < archetypeComponentsCount; i++)
			{
				auto& orderData = archetype.m_ComponentContextsInOrder[i];
				ArchetypeTypeData& archetypeTypeData = archetype.m_TypeData[orderData.m_ComponentIndex];
				archetypeTypeData.m_ComponentContext->InvokeOnDestroyComponent(
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

	void Container::DestroyDelayedEntity(const Entity& entity)
	{
		Archetype* currentArchetype = entity.m_EntityData->m_Archetype;
		InvokeEntityDestructionObservers(entity);
		if (currentArchetype != nullptr)
		{
			InvokeEntityComponentDestructionObservers(entity);
			currentArchetype->RemoveSwapBackEntity(entity.m_EntityData->m_IndexInArchetype);
		}
		else
		{
			RemoveFromEmptyEntities(*entity.m_EntityData);
		}

		m_EntityManager->DestroyEntity(*entity.m_EntityData);
	}

	void Container::AddEntityToDelayedDestroy(const Entity& entity)
	{
		entity.m_EntityData->SetState(EntityState::DelayedToDestruction);
		m_DelayedEntitiesToDestroy.push_back(entity.m_EntityData);
	}
}
