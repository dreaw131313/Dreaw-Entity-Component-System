#pragma once
#include "Container.h"
#include "Entity.h"

#include "decs/Utils/ContainerIterator.h"

namespace decs
{
	Container::Container() :
		m_HaveOwnEntityManager(true),
		m_EntityManager(new EntityManager(m_DefaultEntitiesChunkSize))
	{

	}

	Container::Container(
		uint64_t enititesChunkSize,
		uint32_t stableComponentDefaultChunkSize
	) :
		m_HaveOwnEntityManager(true),
		m_EntityManager(new EntityManager(enititesChunkSize)),
		m_StableContainers(stableComponentDefaultChunkSize)
	{
	}

	Container::Container(
		EntityManager* entityManager,
		uint32_t stableComponentDefaultChunkSize
	) :
		m_HaveOwnEntityManager(entityManager == nullptr),
		m_EntityManager(entityManager == nullptr ? new EntityManager(m_DefaultEntitiesChunkSize) : entityManager),
		m_StableContainers(stableComponentDefaultChunkSize)
	{
	}

	Container::Container(bool bCreateInvalid) :
		m_HaveOwnEntityManager(!bCreateInvalid),
		m_EntityManager(bCreateInvalid ? nullptr : new EntityManager(m_DefaultEntitiesChunkSize))
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

		m_StableContainers.DestroyContainers();
	}

	void Container::ValidateInternalState()
	{
		m_IsInvokingObserversCallbacks = false;
		m_CanCreateEntities = true;
		m_CanDestroyEntities = true;
		m_CanSpawn = true;
		m_CanAddComponents = true;
		m_CanRemoveComponents = true;

		m_SpawnData.Clear();
		m_DelayedEntitiesToDestroy.clear();
		m_ArchetypesRecordsToDelayedRemove.clear();


		m_ComponentContextManager.IterateOverComponentContexts([](ComponentContextBase* componentContext)
		{
			componentContext->SetCanInvokeCreateObservers(true);
		});

	}

	void Container::SetDataIfCreatedInvalid(
		EntityManager* entityManager,
		uint32_t stableComponentDefaultChunkSize
	)
	{
		if (m_EntityManager == nullptr)
		{
			m_EntityManager = entityManager;
			m_StableContainers.SetDefaultChunkSize(stableComponentDefaultChunkSize);
			//m_EmptyEntities = { emptyEntitiesChunkSize };
		}
	}

	void Container::Clear()
	{
		ReturnOwnedEntitiesToEntityManager_Internal(false);

		m_DelayedEntitiesToDestroy.clear();
		m_ArchetypesRecordsToDelayedRemove.clear();
		m_ActivationChangeComponentRefs.clear();
		m_SpawnData.Clear();
		m_EmptyEntities.clear();
		m_ArchetypesMap.ClearEntityDataAndComponents();
		m_StableContainers.ClearContainers();
		m_EntityCount = 0;
	}

	void Container::ReturnOwnedEntitiesToEntityManager()
	{
		ReturnOwnedEntitiesToEntityManager_Internal(true);
	}

	void Container::ReturnOwnedEntitiesToEntityManager_Internal(bool bNullEntityManagerIfIsNotHisOwner)
	{
		if (m_EntityManager != nullptr)
		{
			ContainerIterator iterator = {};
			iterator.Foreach(*this, [this](const decs::Entity& entity)
			{
				m_EntityManager->ForceDestroyEntity(*entity.m_EntityData);
			});

			FreeReservedEntities();

			if (bNullEntityManagerIfIsNotHisOwner && !m_HaveOwnEntityManager)
			{
				m_EntityManager = nullptr;
			}
		}
	}

	Entity Container::CreateEntity(bool isActive)
	{
		if (m_CanCreateEntities)
		{
			EntityData* entityData = CreateAliveEntityData(isActive);
			Entity e(entityData, this);
			AddToEmptyEntitiesRightAfterNewEntityCreation(*e.m_EntityData);
			InvokeEntityCreateObserver(e);
			InvokeEntityEnableObserver(e);
			return e;
		}
		return Entity();
	}

	bool Container::DestroyEntity(const Entity& entity)
	{
		if (entity.IsValid())
		{
			return DestroyEntityInternal(entity, true);
		}
		return false;
	}

	bool Container::DestroyEntityInternal(Entity entity, bool bInvokeObservers)
	{
		if (m_CanDestroyEntities && entity.m_Container == this)
		{
			EntityID entityID = entity.GetID();
			EntityData& entityData = *entity.m_EntityData;
			if (entityData.GetEntityCallbackState() != EEntityCallbackState::None)
			{
				return false;
			}

			if (!entityData.CanBeDestructed())
			{
				return false;
			}

			if (m_PerformDelayedDestruction)
			{
				if (entityData.IsDelayedToDestruction()) { return false; }
				AddEntityToDelayedDestroy(entity, bInvokeObservers);
				return true;
			}
			else
			{
				entityData.SetState(EEntityState::InDestruction);
			}

			Archetype* currentArchetype = entityData.m_Archetype;

			// Destroy callbacks:
			if (bInvokeObservers)
			{
				entityData.SetEntityCallbackState(EEntityCallbackState::Destroy);
				if (currentArchetype != nullptr)
				{
					const uint32_t indexInArchetype = entityData.m_IndexInArchetype;

					InvokeEntityComponentDestructionObservers(entity);
				}
				InvokeEntityDisableObserver(entity);
				InvokeEntityDestroyObserver(entity);
			}

			entityData.SetEntityCallbackState(EEntityCallbackState::None);

			if (currentArchetype != nullptr)
			{
				const uint32_t indexInArchetype = entityData.m_IndexInArchetype;
				currentArchetype->RemoveSwapBackEntity(indexInArchetype);
			}
			else
			{
				RemoveFromEmptyEntities(entityData);
			}

			m_EntityManager->DestroyEntity(entityData);

			m_EntityCount -= 1;

			return true;
		}

		return false;
	}

	void Container::SetEntityActive(const Entity& entity, bool isActive)
	{
		if (entity.m_Container == this
			&& entity.m_EntityData->IsAlive()
			&& entity.m_EntityData->IsActive() != isActive
			&& entity.m_EntityData->GetEntityCallbackState() == EEntityCallbackState::None
			)
		{
			entity.m_EntityData->SetActiveState(isActive);
			if (isActive)
			{
				entity.m_EntityData->SetEntityCallbackState(EEntityCallbackState::Enable);
				InvokeEntityAndComponentEnableObservers(entity);
			}
			else
			{
				entity.m_EntityData->SetEntityCallbackState(EEntityCallbackState::Disable);
				InvokeEntityAndComponentsDisableObservers(entity);
			}

			entity.m_EntityData->SetEntityCallbackState(EEntityCallbackState::None);
		}
	}

	void Container::AddToEmptyEntitiesRightAfterNewEntityCreation(EntityData& data)
	{
		data.m_Archetype = nullptr;
		data.m_IndexInArchetype = (uint32_t)m_EmptyEntities.size();
		m_EmptyEntities.push_back(&data);
	}

	void Container::AddToEmptyEntities(EntityData& data)
	{
		if (data.m_Archetype == nullptr)
		{
			return;
		}

		data.m_Archetype = nullptr;
		data.m_IndexInArchetype = (uint32_t)m_EmptyEntities.size();
		m_EmptyEntities.push_back(&data);
	}

	void Container::RemoveFromEmptyEntities(EntityData& data)
	{
		if (data.m_Archetype != nullptr)
		{
			return;
		}

		if (data.m_IndexInArchetype < m_EmptyEntities.size() - 1)
		{
			m_EmptyEntities[data.m_IndexInArchetype] = m_EmptyEntities.back();
			m_EmptyEntities.back()->m_IndexInArchetype = data.m_IndexInArchetype;
		}

		m_EmptyEntities.pop_back();
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

		if (entity.IsActive())
		{
			for (uint64_t i = 0; i < componentsCount; i++)
			{
				const auto& orderData = orderDatas[i];
				ArchetypeTypeData& typeData = typeDatas[orderData.m_ComponentIndex];
				typeData.m_ComponentContext->InvokeOnDisableComponent(typeData.m_PackedContainer->GetComponentBasePtr(indexInArchetype), entity);
				typeData.m_ComponentContext->InvokeOnDestroyComponent(typeData.m_PackedContainer->GetComponentBasePtr(indexInArchetype), entity);
			}
		}
		else
		{
			for (uint64_t i = 0; i < componentsCount; i++)
			{
				const auto& orderData = orderDatas[i];
				ArchetypeTypeData& typeData = typeDatas[orderData.m_ComponentIndex];
				// typeData.m_ComponentContext->InvokeOnDisableEntity(typeData.m_PackedContainer->GetComponentBasePtr(indexInArchetype), entity); // no becouse entity is disabled
				typeData.m_ComponentContext->InvokeOnDestroyComponent(typeData.m_PackedContainer->GetComponentBasePtr(indexInArchetype), entity);
			}
		}
	}

	EntityData* Container::CreateAliveEntityData(bool bIsActive)
	{
		m_EntityCount += 1;

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

	void Container::ReserveEntities(uint32_t entitiesToReserve)
	{
		m_ReservedEntitiesCount += entitiesToReserve;
		m_ReservedEntityData.reserve(m_ReservedEntitiesCount);
		m_EntityManager->CreateReservedEntityData(entitiesToReserve, m_ReservedEntityData);
	}

	void Container::FreeReservedEntities()
	{
		m_EntityManager->ReturnReservedEntityData(m_ReservedEntityData);
		m_ReservedEntityData.clear();
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

			spawnedEntityData->SetEntityCallbackState(EEntityCallbackState::Create);
			InvokeEntityCreateObserver(spawnedEntity);

			if (spawnedEntity.IsActive())
			{
				InvokeEntityEnableObserver(spawnedEntity);
			}
			spawnedEntityData->SetEntityCallbackState(EEntityCallbackState::None);

			return spawnedEntity;
		}

		uint64_t componentsCount = prefabArchetype->ComponentCount();
		SpawnDataState spawnState(m_SpawnData);

		PrepareSpawnDataFromPrefab(prefabEntityData, prefabContainer);
		CreateEntityFromSpawnData(*spawnedEntityData, spawnState);

		spawnedEntityData->SetEntityCallbackState(EEntityCallbackState::Create);
		InvokeEntityCreateObserver(spawnedEntity);
		if (isActive)
		{
			InvokeEntityEnableObserver(spawnedEntity);
		}
		InvokeComponentCreateAndEnableObserversOnSpawn(spawnedEntity, m_SpawnData.m_SpawnArchetypes[spawnState.m_ArchetypeIndex], componentsCount, spawnState);

		spawnedEntityData->SetEntityCallbackState(EEntityCallbackState::None);

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
				spawnedEntity.Set(entityData, this);
				AddToEmptyEntitiesRightAfterNewEntityCreation(*entityData);

				entityData->SetEntityCallbackState(EEntityCallbackState::Create);
				InvokeEntityCreateObserver(spawnedEntity);
				if (areActive)
				{
					InvokeEntityEnableObserver(spawnedEntity);
				}
				entityData->SetEntityCallbackState(EEntityCallbackState::None);
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
			spawnedEntity.Set(entityData, this);

			CreateEntityFromSpawnData(*entityData, spawnState);

			entityData->SetEntityCallbackState(EEntityCallbackState::Create);
			InvokeEntityCreateObserver(spawnedEntity);
			if (areActive)
			{
				InvokeEntityEnableObserver(spawnedEntity);
			}
			InvokeComponentCreateAndEnableObserversOnSpawn(spawnedEntity, m_SpawnData.m_SpawnArchetypes[spawnState.m_ArchetypeIndex], componentsCount, spawnState);

			entityData->SetEntityCallbackState(EEntityCallbackState::None);
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
				AddToEmptyEntitiesRightAfterNewEntityCreation(*entityData);

				entityData->SetEntityCallbackState(EEntityCallbackState::Create);
				InvokeEntityCreateObserver(spawnedEntity);
				if (areActive)
				{
					InvokeEntityEnableObserver(spawnedEntity);
				}

				entityData->SetEntityCallbackState(EEntityCallbackState::None);
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

			entityData->SetEntityCallbackState(EEntityCallbackState::Create);
			InvokeEntityCreateObserver(spawnedEntity);
			if (areActive)
			{
				InvokeEntityEnableObserver(spawnedEntity);
			}
			InvokeComponentCreateAndEnableObserversOnSpawn(spawnedEntity, m_SpawnData.m_SpawnArchetypes[spawnState.m_ArchetypeIndex], componentsCount, spawnState);

			entityData->SetEntityCallbackState(EEntityCallbackState::None);
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
				&m_ComponentContextManager,
				&m_StableContainers
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

	void Container::InvokeComponentCreateAndEnableObserversOnSpawn(const Entity& entity, Archetype* archetype, uint64_t componentsCount, const SpawnDataState& spawnState)
	{
		auto& orderContextVector = archetype->m_ComponentContextsInOrder;

		if (entity.IsActive())
		{
			for (uint64_t idx = 0, compRefIdx = spawnState.m_CompRefsStart; idx < componentsCount; idx++)
			{
				auto& orderData = orderContextVector[idx];
				uint32_t componentIdx = orderData.m_ComponentIndex;

				auto& componentRef = m_SpawnData.m_SpawnedEntityComponentRefs[compRefIdx + componentIdx];
				ComponentBase* componentVoidPtr = componentRef.Get();
				if (componentVoidPtr != nullptr)
				{
					orderData.m_ComponentContext->InvokeOnCreateComponent(componentVoidPtr, entity);
					if (entity.m_EntityData->IsActive())
					{
						orderData.m_ComponentContext->InvokeOnEnableComponent(componentVoidPtr, entity);
					}
				}
			}
		}
		else
		{
			for (uint64_t idx = 0, compRefIdx = spawnState.m_CompRefsStart; idx < componentsCount; idx++)
			{
				auto& orderData = orderContextVector[idx];
				uint32_t componentIdx = orderData.m_ComponentIndex;

				auto& componentRef = m_SpawnData.m_SpawnedEntityComponentRefs[compRefIdx + componentIdx];
				ComponentBase* componentVoidPtr = componentRef.Get();
				if (componentVoidPtr != nullptr)
				{
					orderData.m_ComponentContext->InvokeOnCreateComponent(componentVoidPtr, entity);
					//orderData.m_ComponentContext->InvokeOnEnableEntity(componentVoidPtr, entity);
				}
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
			packedContainer->GetComponentBasePtr(entityIndexInOldArchetype),
			entity
		);

		if (m_PerformDelayedDestruction)
		{
			AddArchetypeRecordToDelayedRemove(oldArchetype, static_cast<uint32_t>(entityIndexInOldArchetype), true, componentTypeID);
		}
		else
		{
			oldArchetype->RemoveSwapBackEntityAfterMoveEntityWithoutDestroyingSource(entityIndexInOldArchetype, componentTypeID);
		}

		return true;
	}

	void Container::InvokeEntitesOnCreateListeners()
	{
		if (m_IsInvokingObserversCallbacks) return;
		BoolSwitch invokingObserverCallbackSwitch(m_IsInvokingObserversCallbacks, true);
		BoolSwitch isDestroyingEntitesFlag(m_PerformDelayedDestruction, true);

		Entity entity = {};

		m_ComponentContextManager.IterateOverComponentContexts([](ComponentContextBase* componentContext)
		{
			componentContext->SetCanInvokeCreateObservers(false);
		});

		// invoking entity creation observers:
		{
			for (int64_t i = m_EmptyEntities.size() - 1; i << m_EmptyEntities.size() >= 0; i--)
			{
				entity.Set(m_EmptyEntities[i], this);

				InvokeEntityCreateObserver(entity);
				if (entity.IsActive())
				{
					InvokeEntityEnableObserver(entity);
				}
			}

			m_ArchetypesMap.IterateOverArchetypes([&](Archetype* archetype)
			{
				if (archetype->EntityCount() == 0)
				{
					return;
				}
				const auto& entitiesData = archetype->m_EntitiesData;
				for (int64_t idx = static_cast<int64_t>(archetype->EntityCount()) - 1; idx >= 0; idx--)
				{
					const auto& archetypeEntityData = entitiesData[idx];
					if (!archetypeEntityData.IsIntendedToDelayedDestroy())
					{
						entity.Set(archetypeEntityData.m_EntityData, this);

						InvokeEntityCreateObserver(entity);
						if (entity.IsActive() && !archetypeEntityData.IsIntendedToDelayedDestroy())
						{
							InvokeEntityEnableObserver(entity);
						}
					}
				}
			});
		}

		// invoking components creation observers
		{
			ComponentBaseRef compRef = {};

			m_ComponentContextManager.IterateOverComponentContexts([&](ComponentContextBase* componentContext)
			{
				componentContext->SetCanInvokeCreateObservers(true);
				if (!componentContext->HasCreateObserver())
				{
					return;
				}

				TypeID componentTypeID = componentContext->GetComponentTypeID();

				m_ArchetypesMap.IterateOverArchetypesWithType(componentTypeID, [&](Archetype* archetype)
				{
					const uint64_t entitiesCountToInvokeCallbacks = archetype->EntityCount();
					if (entitiesCountToInvokeCallbacks == 0)
					{
						return;
					}

					const uint64_t compIdx = archetype->FindTypeIndex(componentTypeID);

					const auto& typeData = archetype->m_TypeData[compIdx];
					auto* packedContainer = typeData.m_PackedContainer;
					const auto& entityData = archetype->m_EntitiesData;

					for (int64_t idx = static_cast<int64_t>(entitiesCountToInvokeCallbacks) - 1; idx >= 0; idx--)
					{
						const auto& archetypeEntityData = entityData[idx];
						if (!archetypeEntityData.IsIntendedToDelayedDestroy())
						{
							entity.Set(archetypeEntityData.m_EntityData, this);
							compRef.Set(componentTypeID, *archetypeEntityData.m_EntityData, static_cast<uint32_t>(compIdx));

							auto compPtr = packedContainer->GetComponentBasePtr(idx);
							componentContext->InvokeOnCreateComponent(compPtr, entity);

							if (entity.IsActive())
							{
								auto compPtr = compRef.GetUnsafe();
								if (compPtr != nullptr)
								{
									componentContext->InvokeOnEnableComponent(compPtr, entity);
								}
							}
						}
					}
				});
			});
		}

		PerformDelayedDestruction();
	}

	void Container::InvokeEntitesOnDestroyListeners()
	{
		if (m_IsInvokingObserversCallbacks) return;
		BoolSwitch invokingObserverCallbackSwitch(m_IsInvokingObserversCallbacks, true);
		BoolSwitch canCreateSwitch(m_CanCreateEntities, false);
		BoolSwitch canDestroySwitch(m_CanDestroyEntities, false);
		BoolSwitch canSpawnSwitch(m_CanSpawn, false);
		BoolSwitch canAddComponentSwitch(m_CanAddComponents, false);
		BoolSwitch canRemoveComponentSwitch(m_CanRemoveComponents, false);

		// invoking components creation observers
		{
			Entity entity = {};
			m_ComponentContextManager.IterateOverComponentContextsForDestryObservers([&](ComponentContextBase* componentContext)
			{
				TypeID componentTypeID = componentContext->GetComponentTypeID();

				if (!componentContext->HasDestroyObserver())
				{
					return;
				}

				m_ArchetypesMap.IterateOverArchetypesWithType(componentTypeID, [&](Archetype* archetype)
				{
					const uint64_t entityCount = archetype->EntityCount();
					if (entityCount == 0)
					{
						return;
					}

					const uint64_t compIdx = archetype->FindTypeIndex(componentTypeID);

					const auto& typeData = archetype->m_TypeData[compIdx];
					auto* packedContainer = typeData.m_PackedContainer;
					const auto& entityData = archetype->m_EntitiesData;

					for (int64_t idx = 0; idx < (int64_t)entityCount; idx++)
					{
						const auto& archetypeEntityData = entityData[idx];
						entity.Set(archetypeEntityData.m_EntityData, this);
						auto compPtr = packedContainer->GetComponentBasePtr(idx);
						if (entity.IsActive())
						{
							componentContext->InvokeOnDisableComponent(compPtr, entity);
						}
						componentContext->InvokeOnDestroyComponent(compPtr, entity);
					}
				});
			});
		}

		// invoking entity creation observers:
		{
			ContainerIterator iterator = {};
			iterator.Foreach(*this, [&](const decs::Entity& entity)
			{
				if (entity.IsActive())
				{
					InvokeEntityDisableObserver(entity);
				}
				InvokeEntityDestroyObserver(entity);
			});
		}
	}

	inline void Container::InvokeEntityCreateObserver(const Entity& entity)
	{
		auto entityData = entity.m_EntityData;
		if (!entityData->m_bIsCreatedByContainer)
		{
			entityData->m_bIsCreatedByContainer = true;
			if (m_CreateEntityObserver != nullptr)
			{
				m_CreateEntityObserver->OnCreateEntity(entity);
			}
		}
	}

	inline void Container::InvokeEntityDestroyObserver(const Entity& entity)
	{
		auto entityData = entity.m_EntityData;
		if (entityData->m_bIsCreatedByContainer)
		{
			entityData->m_bIsCreatedByContainer = false;
			if (m_DestroyEntityObserver != nullptr)
			{
				m_DestroyEntityObserver->OnDestroyEntity(entity);
			}
		}
	}

	inline void Container::InvokeEntityEnableObserver(const Entity& entity)
	{
		auto entityData = entity.m_EntityData;
		if (!entityData->m_bIsEnabledByContainer)
		{
			entityData->m_bIsEnabledByContainer = true;
			if (m_EnableEntityObserver != nullptr)
			{
				m_EnableEntityObserver->OnEnableEntity(entity);
			}
		}
	}

	inline void Container::InvokeEntityDisableObserver(const Entity& entity)
	{
		auto entityData = entity.m_EntityData;
		if (entityData->m_bIsEnabledByContainer)
		{
			entityData->m_bIsEnabledByContainer = false;
			if (m_DisableEntityObserver != nullptr)
			{
				m_DisableEntityObserver->OnDisableEntity(entity);
			}
		}
	}

	void Container::InvokeEntityAndComponentEnableObservers(const Entity& entity)
	{
		if (m_EnableEntityObserver != nullptr)
		{
			m_EnableEntityObserver->OnEnableEntity(entity);
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
				ComponentBase* compPtr = compRef.Get();
				if (compPtr != nullptr)
				{
					// invoke activation listener:
					orderData.m_ComponentContext->InvokeOnEnableComponent(compPtr, entity);
				}
			}

			// erase used component refs:
			m_ActivationChangeComponentRefs.erase(m_ActivationChangeComponentRefs.begin() + startRefsIdx, m_ActivationChangeComponentRefs.end());
		}
	}

	void Container::InvokeEntityAndComponentsDisableObservers(const Entity& entity)
	{
		if (m_DisableEntityObserver != nullptr)
		{
			m_DisableEntityObserver->OnDisableEntity(entity);
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
				ComponentBase* compPtr = compRef.Get();
				if (compPtr != nullptr)
				{
					orderData.m_ComponentContext->InvokeOnDisableComponent(compPtr, entity);
				}
			}

			// erase used component refs:
			m_ActivationChangeComponentRefs.erase(m_ActivationChangeComponentRefs.begin() + startRefsIdx, m_ActivationChangeComponentRefs.end());
		}
	}

	void Container::PerformDelayedDestruction()
	{
		RemoveArchetypesRecordsDelayedToRemove();
		DestroyDelayedEntities();
	}

	void Container::DestroyDelayedEntities()
	{
		Entity e = {};
		for (auto& entityDataToDelayedDestroy : m_DelayedEntitiesToDestroy)
		{
			e.Set(*entityDataToDelayedDestroy.entityData, this);
			DestroyDelayedEntity(e, entityDataToDelayedDestroy.bInvokeCallbacks);
		}
		m_DelayedEntitiesToDestroy.clear();
	}

	void Container::RemoveArchetypesRecordsDelayedToRemove()
	{
		for (const auto& record : m_ArchetypesRecordsToDelayedRemove)
		{
			if (record.bRemove)
			{
				record.archetype->RemoveSwapBackEntityAfterMoveEntityWithoutDestroyingSource(static_cast<uint64_t>(record.index), record.removedComponentTypeID);
			}
			else
			{
				record.archetype->RemoveSwapBackRecordRaw(record.index);
			}
		}
		m_ArchetypesRecordsToDelayedRemove.clear();
	}

	void Container::DestroyDelayedEntity(const Entity& entity, bool bInvokeCallbacks)
	{
		Archetype* currentArchetype = entity.m_EntityData->m_Archetype;
		EntityData& entityData = *entity.m_EntityData;

		// Destroy callbacks:

		if (bInvokeCallbacks)
		{
			entityData.SetEntityCallbackState(EEntityCallbackState::Destroy);
			if (currentArchetype != nullptr)
			{
				InvokeEntityComponentDestructionObservers(entity);
			}

			InvokeEntityDisableObserver(entity);
			InvokeEntityDestroyObserver(entity);

			entityData.SetEntityCallbackState(EEntityCallbackState::None);
		}

		if (currentArchetype != nullptr)
		{
			const uint32_t indexInArchetype = entityData.m_IndexInArchetype;
			currentArchetype->RemoveSwapBackEntity(indexInArchetype);
		}
		else
		{
			RemoveFromEmptyEntities(entityData);
		}

		m_EntityManager->DestroyEntity(*entity.m_EntityData);
	}

	void Container::AddEntityToDelayedDestroy(const Entity& entity, bool bInvokeCallbacks)
	{
		entity.m_EntityData->SetState(EEntityState::DelayedToDestruction);
		m_DelayedEntitiesToDestroy.push_back({ entity.m_EntityData,bInvokeCallbacks });
	}

	Entity Container::CreateEntity_NoCallbacks(bool bIsActive)
	{
		if (m_CanCreateEntities)
		{
			EntityData* entityData = CreateAliveEntityData(bIsActive);
			Entity e(entityData, this);
			AddToEmptyEntitiesRightAfterNewEntityCreation(*e.m_EntityData);
			return e;
		}
		return Entity();
	}

	bool Container::DestroyEntity_NoCallback(const Entity& entity)
	{
		if (entity.IsValid() && m_CanDestroyEntities && entity.m_Container == this)
		{
			DestroyEntityInternal(entity, false);
			return true;
		}

		return false;
	}

	void Container::SetEntityActive_NoCallback(const Entity& entity, bool bIsActive)
	{
		if (entity.m_Container == this
			&& entity.m_EntityData->IsAlive()
			&& entity.m_EntityData->IsActive() != bIsActive
			&& entity.m_EntityData->GetEntityCallbackState() == EEntityCallbackState::None
			)
		{
			entity.m_EntityData->SetActiveState(bIsActive);
		}
	}

	Entity Container::Spawn_NoCallback(const Entity& prefab, bool isActive)
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

			return spawnedEntity;
		}

		uint64_t componentsCount = prefabArchetype->ComponentCount();
		SpawnDataState spawnState(m_SpawnData);

		PrepareSpawnDataFromPrefab(prefabEntityData, prefabContainer);
		CreateEntityFromSpawnData(*spawnedEntityData, spawnState);

		m_SpawnData.PopBackSpawnState(spawnState.m_ArchetypeIndex, spawnState.m_CompRefsStart);

		return spawnedEntity;
	}

	bool Container::Spawn_NoCallback(const Entity& prefab, uint64_t spawnCount, bool areActive)
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
				spawnedEntity.Set(entityData, this);
				AddToEmptyEntitiesRightAfterNewEntityCreation(*entityData);
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
			spawnedEntity.Set(entityData, this);
			CreateEntityFromSpawnData(*entityData, spawnState);
		}

		m_SpawnData.PopBackSpawnState(spawnState.m_ArchetypeIndex, spawnState.m_CompRefsStart);

		return true;
	}

	bool Container::Spawn_NoCallback(const Entity& prefab, std::vector<Entity>& spawnedEntities, uint64_t spawnCount, bool areActive)
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
				AddToEmptyEntitiesRightAfterNewEntityCreation(*entityData);
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
		}

		m_SpawnData.PopBackSpawnState(spawnState.m_ArchetypeIndex, spawnState.m_CompRefsStart);

		return true;
	}

	bool Container::RemoveComponent_NoCallback(const Entity& entity, TypeID componentTypeID)
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

		if (m_PerformDelayedDestruction)
		{
			AddArchetypeRecordToDelayedRemove(oldArchetype, static_cast<uint32_t>(entityIndexInOldArchetype), true, componentTypeID);
		}
		else
		{
			oldArchetype->RemoveSwapBackEntityAfterMoveEntityWithoutDestroyingSource(entityIndexInOldArchetype, componentTypeID);
		}

		return true;
	}

}
