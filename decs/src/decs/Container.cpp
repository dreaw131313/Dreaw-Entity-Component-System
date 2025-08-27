#pragma once
#include "Container.h"
#include "Entity.h"

#include "decs/Utils/ContainerIterator.h"
#include "decs/Utils/UtilityClasses.h"

namespace decs
{
	Container::Container():
		m_EntityManager(m_DefaultEntitiesChunkSize)
	{

	}

	Container::Container(
		uint64_t enititesChunkSize,
		uint32_t stableComponentDefaultChunkSize
	):
		m_EntityManager(enititesChunkSize),
		m_ComponentContextManager(stableComponentDefaultChunkSize)
	{
	}

	Container::~Container()
	{
		m_ComponentContextManager.ClearStableContainers();
		m_ArchetypesMap.ClearEntityDataAndComponents();
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
	}

	void Container::Clear()
	{
		ReturnOwnedEntitiesToEntityManager_Internal();

		m_DelayedEntitiesToDestroy.clear();
		m_ArchetypesRecordsToDelayedRemove.clear();
		m_ActivationChangeComponentPtrs.clear();
		m_SpawnData.Clear();
		m_EmptyEntities.clear();
		m_ArchetypesMap.ClearEntityDataAndComponents();
		m_ComponentContextManager.ClearStableContainers();
	}


	void Container::MarkEntitiesDead()
	{
		m_EntityManager.MarkEntitiesDead();
	}

	void Container::ReturnOwnedEntitiesToEntityManager_Internal()
	{
		ContainerIterator iterator = {};
		iterator.Foreach(*this, [this](const decs::Entity& entity)
		{
			m_EntityManager.ForceDestroyEntity(entity.m_Handle);
		});
	}

	Entity Container::CreateEntity(bool bIsActive)
	{
		if (m_CanCreateEntities)
		{
			EntityDataHandle entityData = m_EntityManager.CreateEntity(bIsActive, *this);
			Entity e(entityData);
			AddToEmptyEntitiesRightAfterNewEntityCreation(*e.GetEntityData());
			InvokeEntityCreateObserver_Internal(e);
			InvokeEntityEnableObserver_Internal(e);

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

	bool Container::DestroyEntityInternal(const Entity& entity, bool bInvokeObservers)
	{
		if (m_CanDestroyEntities && entity.GetContainer() == this)
		{
			EntityID entityID = entity.GetID();
			EntityData& entityData = *entity.GetEntityData();
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
				if (currentArchetype != nullptr)
				{
					const uint32_t indexInArchetype = entityData.m_IndexInArchetype;

					InvokeEntityComponentDestructionObservers(entity);
				}
				InvokeEntityDisableObserver_Internal(entity);
				InvokeEntityDestroyObserver_Internal(entity);
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

			m_EntityManager.DestroyEntity(entity.m_Handle);

			return true;
		}

		return false;
	}

	void Container::SetEntityActive(const Entity& entity, bool isActive)
	{
		if (entity.GetContainer() == this
			&& entity.GetEntityData()->IsValidToChangeActiveState()
			&& entity.GetEntityData()->IsActive() != isActive
			)
		{
			entity.GetEntityData()->SetActiveState(isActive);
			if (isActive)
			{
				InvokeEntityAndComponentEnableObservers_Internal(entity);
			}
			else
			{
				InvokeEntityAndComponentsDisableObservers_Internal(entity);
			}

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
		Archetype* currentArchetype = entity.GetEntityData()->m_Archetype;
		const uint32_t componentsCount = currentArchetype->GetComponentOnlyCount();

		auto& typeDatas = currentArchetype->m_TypeData;
		auto& orderDatas = currentArchetype->m_ComponentContextsInOrder;

		// Invoke On destroy methods

		if (entity.IsActive())
		{
			for (uint64_t i = 0; i < componentsCount; i++)
			{
				const uint32_t indexInArchetype = entity.GetEntityData()->m_IndexInArchetype;
				const auto& orderData = orderDatas[i];
				ArchetypeTypeData& typeData = typeDatas[orderData.m_ComponentIndex];
				if (!typeData.IsTag())
				{
					typeData.m_ComponentContext->InvokeOnDisableComponent(typeData.m_PackedContainer->GetComponentBasePtr(indexInArchetype), entity);
					typeData.m_ComponentContext->InvokeOnDestroyComponent(typeData.m_PackedContainer->GetComponentBasePtr(indexInArchetype), entity);
				}
			}
		}
		else
		{
			for (uint64_t i = 0; i < componentsCount; i++)
			{
				const uint32_t indexInArchetype = entity.GetEntityData()->m_IndexInArchetype;
				const auto& orderData = orderDatas[i];
				ArchetypeTypeData& typeData = typeDatas[orderData.m_ComponentIndex];
				// typeData.m_ComponentContext->InvokeOnDisableEntity(typeData.m_PackedContainer->GetComponentBasePtr(indexInArchetype), entity); // no becouse entity is disabled
				if (!typeData.IsTag())
				{
					typeData.m_ComponentContext->InvokeOnDestroyComponent(typeData.m_PackedContainer->GetComponentBasePtr(indexInArchetype), entity);
				}
			}
		}
	}

	Entity Container::Spawn(
		const Entity& prefab,
		bool bIsActive
	)
	{
		if (!m_CanSpawn || prefab.IsNull()) return Entity();

		Container* prefabContainer = prefab.GetContainer();
		EntityData& prefabEntityData = *prefab.GetEntityData();
		Archetype* prefabArchetype = prefabEntityData.m_Archetype;

		BoolSwitch prefabOperationsLock = { prefabEntityData.m_bIsUsedAsPrefab, true };

		Entity spawnedEntity(m_EntityManager.CreateEntity(bIsActive, *this));
		EntityData* spawnedEntityData = spawnedEntity.GetEntityData();

		if (prefabArchetype == nullptr)
		{
			AddToEmptyEntitiesRightAfterNewEntityCreation(*spawnedEntityData);

			InvokeEntityCreateObserver_Internal(spawnedEntity);

			if (spawnedEntity.IsActive())
			{
				InvokeEntityEnableObserver_Internal(spawnedEntity);
			}

			return spawnedEntity;
		}

		SpawnDataState spawnState(m_SpawnData);

		PrepareSpawnDataFromPrefab(prefabEntityData, prefabContainer);
		CreateEntityFromSpawnData(spawnedEntity, spawnState);

		InvokeEntityCreateObserver_Internal(spawnedEntity);
		if (spawnedEntity.IsActive())
		{
			InvokeEntityEnableObserver_Internal(spawnedEntity);
		}
		InvokeComponentCreateAndEnableObserversOnSpawn(spawnedEntity, *m_SpawnData.m_SpawnArchetypes[spawnState.m_ArchetypeIndex], spawnState);


		m_SpawnData.PopBackSpawnState(spawnState.m_ArchetypeIndex, spawnState.m_CompRefsStart);

		return spawnedEntity;
	}

	bool Container::Spawn(const Entity& prefab, uint64_t spawnCount, bool areActive)
	{
		if (!m_CanSpawn || spawnCount == 0 || prefab.IsNull()) return false;

		Container* prefabContainer = prefab.GetContainer();
		EntityData& prefabEntityData = *prefab.GetEntityData();
		Archetype* prefabArchetype = prefabEntityData.m_Archetype;
		BoolSwitch prefabOperationsLock = { prefabEntityData.m_bIsUsedAsPrefab, true };

		if (prefabArchetype == nullptr)
		{
			for (uint64_t i = 0; i < spawnCount; i++)
			{
				Entity spawnedEntity(m_EntityManager.CreateEntity(areActive, *this));

				AddToEmptyEntitiesRightAfterNewEntityCreation(*spawnedEntity.GetEntityData());

				InvokeEntityCreateObserver_Internal(spawnedEntity);
				if (spawnedEntity.IsActive())
				{
					InvokeEntityEnableObserver_Internal(spawnedEntity);
				}
			}
			return true;
		}

		SpawnDataState spawnState(m_SpawnData);

		PrepareSpawnDataFromPrefab(prefabEntityData, prefabContainer);

		Archetype* spawnArchetype = m_SpawnData.m_SpawnArchetypes[spawnState.m_ArchetypeIndex];
		spawnArchetype->ReserveSpaceInArchetype(spawnArchetype->EntityCount() + spawnCount);

		for (uint64_t entityIdx = 0; entityIdx < spawnCount; entityIdx++)
		{
			Entity spawnedEntity(m_EntityManager.CreateEntity(areActive, *this));

			CreateEntityFromSpawnData(spawnedEntity, spawnState);

			InvokeEntityCreateObserver_Internal(spawnedEntity);
			if (spawnedEntity.IsActive())
			{
				InvokeEntityEnableObserver_Internal(spawnedEntity);
			}
			InvokeComponentCreateAndEnableObserversOnSpawn(spawnedEntity, *m_SpawnData.m_SpawnArchetypes[spawnState.m_ArchetypeIndex], spawnState);
		}

		m_SpawnData.PopBackSpawnState(spawnState.m_ArchetypeIndex, spawnState.m_CompRefsStart);

		return true;
	}

	bool Container::Spawn(const Entity& prefab, std::vector<Entity>& spawnedEntities, uint64_t spawnCount, bool areActive)
	{
		if (!m_CanSpawn || spawnCount == 0 || prefab.IsNull()) return false;

		Container* prefabContainer = prefab.GetContainer();
		EntityData& prefabEntityData = *prefab.GetEntityData();
		Archetype* prefabArchetype = prefabEntityData.m_Archetype;
		BoolSwitch prefabOperationsLock = { prefabEntityData.m_bIsUsedAsPrefab, true };

		spawnedEntities.reserve(spawnedEntities.size() + spawnCount);

		if (prefabArchetype == nullptr)
		{
			for (uint64_t i = 0; i < spawnCount; i++)
			{
				Entity& spawnedEntity = spawnedEntities.emplace_back(m_EntityManager.CreateEntity(areActive, *this));
				AddToEmptyEntitiesRightAfterNewEntityCreation(*spawnedEntity.GetEntityData());

				InvokeEntityCreateObserver_Internal(spawnedEntity);
				if (spawnedEntity.IsActive())
				{
					InvokeEntityEnableObserver_Internal(spawnedEntity);
				}
			}
			return true;
		}

		SpawnDataState spawnState(m_SpawnData);

		PrepareSpawnDataFromPrefab(prefabEntityData, prefabContainer);

		Archetype* spawnArchetype = m_SpawnData.m_SpawnArchetypes[spawnState.m_ArchetypeIndex];
		spawnArchetype->ReserveSpaceInArchetype(spawnArchetype->EntityCount() + spawnCount);

		for (uint64_t entityIdx = 0; entityIdx < spawnCount; entityIdx++)
		{
			Entity& spawnedEntity = spawnedEntities.emplace_back(m_EntityManager.CreateEntity(areActive, *this));

			CreateEntityFromSpawnData(spawnedEntity, spawnState);

			InvokeEntityCreateObserver_Internal(spawnedEntity);
			if (spawnedEntity.IsActive())
			{
				InvokeEntityEnableObserver_Internal(spawnedEntity);
			}

			InvokeComponentCreateAndEnableObserversOnSpawn(spawnedEntity, *m_SpawnData.m_SpawnArchetypes[spawnState.m_ArchetypeIndex], spawnState);
		}

		m_SpawnData.PopBackSpawnState(spawnState.m_ArchetypeIndex, spawnState.m_CompRefsStart);

		return true;
	}

	Entity Container::Spawn_WithCallback(SpawnEntityCallback& callback, const Entity& prefab, bool bIsActive)
	{
		if (!m_CanSpawn || prefab.IsNull()) return Entity();

		Container* prefabContainer = prefab.GetContainer();
		EntityData& prefabEntityData = *prefab.GetEntityData();
		Archetype* prefabArchetype = prefabEntityData.m_Archetype;

		BoolSwitch prefabOperationsLock = { prefabEntityData.m_bIsUsedAsPrefab, true };

		Entity spawnedEntity(m_EntityManager.CreateEntity(bIsActive, *this));

		if (prefabArchetype == nullptr)
		{
			AddToEmptyEntitiesRightAfterNewEntityCreation(*spawnedEntity.GetEntityData());

			callback.OnSpawnEntityCallback(spawnedEntity);

			InvokeEntityCreateObserver_Internal(spawnedEntity);

			if (spawnedEntity.IsActive())
			{
				InvokeEntityEnableObserver_Internal(spawnedEntity);
			}

			return spawnedEntity;
		}

		SpawnDataState spawnState(m_SpawnData);

		PrepareSpawnDataFromPrefab(prefabEntityData, prefabContainer);
		CreateEntityFromSpawnData(spawnedEntity, spawnState);

		callback.OnSpawnEntityCallback(spawnedEntity);

		InvokeEntityCreateObserver_Internal(spawnedEntity);
		if (spawnedEntity.IsActive())
		{
			InvokeEntityEnableObserver_Internal(spawnedEntity);
		}
		InvokeComponentCreateAndEnableObserversOnSpawn(spawnedEntity, *m_SpawnData.m_SpawnArchetypes[spawnState.m_ArchetypeIndex], spawnState);

		m_SpawnData.PopBackSpawnState(spawnState.m_ArchetypeIndex, spawnState.m_CompRefsStart);

		return spawnedEntity;
	}

	bool Container::Spawn_WithCallback(SpawnEntityCallback& callback, const Entity& prefab, uint64_t spawnCount, bool bAreActive)
	{
		if (!m_CanSpawn || spawnCount == 0 || prefab.IsNull()) return false;

		Container* prefabContainer = prefab.GetContainer();
		EntityData& prefabEntityData = *prefab.GetEntityData();
		Archetype* prefabArchetype = prefabEntityData.m_Archetype;
		BoolSwitch prefabOperationsLock = { prefabEntityData.m_bIsUsedAsPrefab, true };

		if (prefabArchetype == nullptr)
		{
			for (uint64_t i = 0; i < spawnCount; i++)
			{
				Entity spawnedEntity(m_EntityManager.CreateEntity(bAreActive, *this));
				AddToEmptyEntitiesRightAfterNewEntityCreation(*spawnedEntity.GetEntityData());

				callback.OnSpawnEntityCallback(spawnedEntity);

				InvokeEntityCreateObserver_Internal(spawnedEntity);
				if (spawnedEntity.IsActive())
				{
					InvokeEntityEnableObserver_Internal(spawnedEntity);
				}
			}
			return true;
		}

		SpawnDataState spawnState(m_SpawnData);

		PrepareSpawnDataFromPrefab(prefabEntityData, prefabContainer);

		Archetype* spawnArchetype = m_SpawnData.m_SpawnArchetypes[spawnState.m_ArchetypeIndex];
		spawnArchetype->ReserveSpaceInArchetype(spawnArchetype->EntityCount() + spawnCount);

		for (uint64_t entityIdx = 0; entityIdx < spawnCount; entityIdx++)
		{
			Entity spawnedEntity(m_EntityManager.CreateEntity(bAreActive, *this));

			CreateEntityFromSpawnData(spawnedEntity, spawnState);

			callback.OnSpawnEntityCallback(spawnedEntity);

			InvokeEntityCreateObserver_Internal(spawnedEntity);
			if (spawnedEntity.IsActive())
			{
				InvokeEntityEnableObserver_Internal(spawnedEntity);
			}
			InvokeComponentCreateAndEnableObserversOnSpawn(spawnedEntity, *m_SpawnData.m_SpawnArchetypes[spawnState.m_ArchetypeIndex], spawnState);
		}

		m_SpawnData.PopBackSpawnState(spawnState.m_ArchetypeIndex, spawnState.m_CompRefsStart);

		return true;
	}

	bool Container::Spawn_WithCallback(SpawnEntityCallback& callback, const Entity& prefab, std::vector<Entity>& spawnedEntities, uint64_t spawnCount, bool bAreActive)
	{
		if (!m_CanSpawn || spawnCount == 0 || prefab.IsNull()) return false;

		Container* prefabContainer = prefab.GetContainer();
		EntityData& prefabEntityData = *prefab.GetEntityData();
		Archetype* prefabArchetype = prefabEntityData.m_Archetype;
		BoolSwitch prefabOperationsLock = { prefabEntityData.m_bIsUsedAsPrefab, true };

		spawnedEntities.reserve(spawnedEntities.size() + spawnCount);

		if (prefabArchetype == nullptr)
		{
			for (uint64_t i = 0; i < spawnCount; i++)
			{
				Entity& spawnedEntity = spawnedEntities.emplace_back(m_EntityManager.CreateEntity(bAreActive, *this));
				AddToEmptyEntitiesRightAfterNewEntityCreation(*spawnedEntity.GetEntityData());

				callback.OnSpawnEntityCallback(spawnedEntity);

				InvokeEntityCreateObserver_Internal(spawnedEntity);
				if (spawnedEntity.IsActive())
				{
					InvokeEntityEnableObserver_Internal(spawnedEntity);
				}
			}
			return true;
		}

		SpawnDataState spawnState(m_SpawnData);

		PrepareSpawnDataFromPrefab(prefabEntityData, prefabContainer);

		Archetype* spawnArchetype = m_SpawnData.m_SpawnArchetypes[spawnState.m_ArchetypeIndex];
		spawnArchetype->ReserveSpaceInArchetype(spawnArchetype->EntityCount() + spawnCount);

		for (uint64_t entityIdx = 0; entityIdx < spawnCount; entityIdx++)
		{
			Entity& spawnedEntity = spawnedEntities.emplace_back(m_EntityManager.CreateEntity(bAreActive, *this));
			CreateEntityFromSpawnData(spawnedEntity, spawnState);

			callback.OnSpawnEntityCallback(spawnedEntity);

			InvokeEntityCreateObserver_Internal(spawnedEntity);
			if (spawnedEntity.IsActive())
			{
				InvokeEntityEnableObserver_Internal(spawnedEntity);
			}

			InvokeComponentCreateAndEnableObserversOnSpawn(spawnedEntity, *m_SpawnData.m_SpawnArchetypes[spawnState.m_ArchetypeIndex], spawnState);
		}

		m_SpawnData.PopBackSpawnState(spawnState.m_ArchetypeIndex, spawnState.m_CompRefsStart);

		return true;
	}

	void Container::PrepareSpawnDataFromPrefab(
		EntityData& prefabEntityData,
		Container* prefabContainer
	)
	{
		const Archetype& prefabArchetype = *prefabEntityData.m_Archetype;
		const uint32_t prefabIndexInArchetype = prefabEntityData.m_IndexInArchetype;
		Archetype* spawnedEntityArchetype = nullptr;
		uint64_t componentsCount = prefabArchetype.GetComponentAndTagCount();

		if (prefabContainer == this)
		{
			spawnedEntityArchetype = prefabEntityData.m_Archetype;
		}
		else
		{
			spawnedEntityArchetype = m_ArchetypesMap.GetOrCreateMatchedArchetype(
				*prefabEntityData.m_Archetype,
				&m_ComponentContextManager
			);
		}
		m_SpawnData.m_SpawnArchetypes.push_back(spawnedEntityArchetype);

		Entity e(&prefabEntityData);

		for (uint32_t i = 0; i < componentsCount; i++)
		{
			const ArchetypeTypeData& prefabArchetypeTypeData = prefabArchetype.m_TypeData[i];

			ArchetypeTypeData& spawnedEntityArchetypeTypeData = spawnedEntityArchetype->m_TypeData[i];

			if (prefabArchetypeTypeData.IsTag())
			{
				m_SpawnData.m_PrefabComponentRefs.emplace_back();
			}
			else
			{
				m_SpawnData.m_PrefabComponentRefs.emplace_back(
					spawnedEntityArchetypeTypeData.m_StableContainer,
					prefabArchetypeTypeData.m_PackedContainer->GetComponentBasePtr(prefabIndexInArchetype)
				);
			}
		}

		m_SpawnData.m_SpawnedEntityComponentPtrs.resize(m_SpawnData.m_SpawnedEntityComponentPtrs.size() + componentsCount);
	}

	void Container::CreateEntityFromSpawnData(
		const Entity& entity,
		const SpawnDataState& spawnState
	)
	{
		Archetype* archetype = m_SpawnData.m_SpawnArchetypes[spawnState.m_ArchetypeIndex];
		uint64_t componentsCount = archetype->GetComponentAndTagCount() + spawnState.m_CompRefsStart;

		archetype->AddEntityData(entity.GetEntityData());

		auto& typeDataVector = archetype->m_TypeData;
		for (uint32_t i = spawnState.m_CompRefsStart; i < componentsCount; i++)
		{
			ArchetypeTypeData& currentTypeData = typeDataVector[i];
			SpawnComponentRefData& spawnRefData = m_SpawnData.m_PrefabComponentRefs[i];
			if (spawnRefData.IsTag())
			{
				continue;
			}

			ComponentBase* componentPtr = spawnRefData.m_StableContainer->CreateFromComponentBase(spawnRefData.m_ComponentPtr);
			currentTypeData.m_PackedContainer->PushBack(componentPtr);

			m_SpawnData.m_SpawnedEntityComponentPtrs[i] = componentPtr;

			componentPtr->OnPreCreate(entity);
		}
	}

	void Container::InvokeComponentCreateAndEnableObserversOnSpawn(const Entity& entity, const Archetype& archetype, const SpawnDataState& spawnState)
	{
		auto& orderContextVector = archetype.m_ComponentContextsInOrder;
		const uint32_t observerInvokeCount = static_cast<uint32_t>(orderContextVector.size()); // must use this becouse orderContextVector does not contain observers for tags

		uint64_t compRefIdx = spawnState.m_CompRefsStart;

		for (uint64_t idx = 0; idx < observerInvokeCount; idx++)
		{
			auto& orderData = orderContextVector[idx];
			const uint32_t componentIdx = orderData.m_ComponentIndex;

			ComponentBase* componentPtr = m_SpawnData.m_SpawnedEntityComponentPtrs[compRefIdx + componentIdx];
			if (componentPtr != nullptr)
			{
				orderData.m_ComponentContext->InvokeOnCreateComponent(componentPtr, entity);

				if (entity.IsActive())
				{
					orderData.m_ComponentContext->InvokeOnEnableComponent(componentPtr, entity);
				}
			}
		}
	}

	void Container::OnAddComponentInvokeObservers(
		const Entity& entity,
		ComponentContextBase* componentContext,
		PackedContainerBase* packedContainer,
		TypeID compTypeID
	)
	{
		auto entityData = entity.GetEntityData();
		{
			Archetype* currentArch = entityData->m_Archetype;

			ComponentBase* componentPtr = packedContainer->GetComponentBasePtr(entityData->m_IndexInArchetype);
			componentContext->InvokeOnCreateComponent(packedContainer->GetComponentBasePtr(entityData->m_IndexInArchetype), entity);

			// All this checks are here to check if this entity containe components after OnCreateMethod
			Archetype* newArch = entityData->m_Archetype;
			if (newArch != nullptr && entity.IsActive())
			{
				if (currentArch != newArch)
				{
					uint32_t compIndex = newArch->FindTypeIndex(compTypeID);
					if (compIndex < newArch->GetComponentAndTagCount())
					{
						componentContext->InvokeOnEnableComponent(
							newArch->m_TypeData[compIndex].m_PackedContainer->GetComponentBasePtr(entityData->m_IndexInArchetype),
							entity
						);
					}
				}
				else
				{
					componentContext->InvokeOnEnableComponent(packedContainer->GetComponentBasePtr(entityData->m_IndexInArchetype), entity);
				}
			}
		}
	}

	bool Container::RemoveComponent(const Entity& entity, TypeID componentTypeID)
	{
		if (!m_CanRemoveComponents)
		{
			return false;
		}

		if (entity.GetContainer() != this) return false;

		EntityData& entityData = *entity.GetEntityData();
		if (entityData.m_Archetype == nullptr || !entityData.IsValidToPerformComponentOperation()) return false;

		uint32_t compIdxInArch = entityData.m_Archetype->FindTypeIndex(componentTypeID);
		if (compIdxInArch == std::numeric_limits<uint32_t>::max()) return false;

		Archetype* oldArchetype = entityData.m_Archetype;
		uint64_t entityIndexInOldArchetype = entityData.m_IndexInArchetype;

		ArchetypeTypeData& archetypeTypeData = oldArchetype->m_TypeData[compIdxInArch];
		if (archetypeTypeData.IsTag())
		{
			return false;
		}

		auto packedContainer = archetypeTypeData.m_PackedContainer;
		ComponentBase* componentPtr = packedContainer->GetComponentBasePtr(entityIndexInOldArchetype);
		if (componentPtr->GetDependecyCount() > 0)
		{
			return false;
		}

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

		// Invoking remove observers:
		{
			oldArchetype->SetPlaceHolderEntityData(nullptr, static_cast<uint32_t>(entityIndexInOldArchetype));

			auto componentContext = archetypeTypeData.m_ComponentContext;
			componentContext->InvokeOnDisableComponent(componentPtr, entity);
			componentContext->InvokeOnDestroyComponent(componentPtr, entity);
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

	void Container::InvokeRemoveComponentObserverCallbacks(
		EntityData& entityData,
		ComponentBase* componentPtr,
		Archetype& oldArchetype,
		uint32_t entityIndexInOldArchetype,
		ComponentContextBase& componentContext

	)
	{
		Entity entity(&entityData);
		oldArchetype.SetPlaceHolderEntityData(nullptr, static_cast<uint32_t>(entityIndexInOldArchetype));
		componentContext.InvokeOnDisableComponent(componentPtr, entity);
		componentContext.InvokeOnDestroyComponent(componentPtr, entity);
	}

	ComponentBase* Container::GetComponentAtIndex(EntityData& entityData, uint32_t componentIndex)
	{
		if (entityData.IsAlive() && entityData.m_Archetype != nullptr && componentIndex < entityData.m_Archetype->GetComponentOnlyCount())
		{
			auto& orderData = entityData.m_Archetype->m_ComponentContextsInOrder[componentIndex];
			auto& typeData = entityData.m_Archetype->m_TypeData[orderData.m_ComponentIndex];
			if (typeData.IsTag())
			{
				return nullptr;
			}
			return typeData.m_PackedContainer->GetComponentBasePtr(entityData.m_IndexInArchetype);
		}
		return nullptr;
	}

	bool Container::RemoveTag(EntityData& entityData, TypeID tagType)
	{
		if (!m_CanRemoveComponents
			|| entityData.m_Container != this
			|| entityData.m_Archetype == nullptr
			|| !entityData.IsValidToPerformComponentOperation()
			)
		{
			return false;
		}

		uint32_t compIdxInArch = entityData.m_Archetype->FindTypeIndex(tagType);
		if (compIdxInArch == std::numeric_limits<uint32_t>::max()) return false;

		Archetype* oldArchetype = entityData.m_Archetype;
		ArchetypeTypeData& archetypeTypeData = oldArchetype->m_TypeData[compIdxInArch];
		if (!archetypeTypeData.IsTag())
		{
			return false;
		}

		uint64_t entityIndexInOldArchetype = entityData.m_IndexInArchetype;

		Archetype* newEntityArchetype = GetArchetypeAfterRemoveTag(*oldArchetype, tagType);

		if (newEntityArchetype != nullptr)
		{
			// move entity to new archetype without destroying from source
			newEntityArchetype->MoveEntityAfterRemoveComponentWithoutDestroyingFromSource(
				tagType,
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
			AddArchetypeRecordToDelayedRemove(oldArchetype, static_cast<uint32_t>(entityIndexInOldArchetype), true, tagType);
		}
		else
		{
			oldArchetype->RemoveSwapBackEntityAfterMoveEntityWithoutDestroyingSource(entityIndexInOldArchetype, tagType);
		}

		return true;
	}

	void Container::InvokeEntitesOnCreateListeners()
	{
		if (m_IsInvokingObserversCallbacks) return;
		BoolSwitch invokingObserverCallbackSwitch(m_IsInvokingObserversCallbacks, true);
		BoolSwitch isDestroyingEntitesFlag(m_PerformDelayedDestruction, true);

		Entity entity = {};

		// invoking entity creation observers:
		{
			for (int64_t i = m_EmptyEntities.size() - 1; i << m_EmptyEntities.size() >= 0; i--)
			{
				entity.Set_Internal(EntityDataHandle(m_EmptyEntities[i]));

				InvokeEntityCreateObserver_Internal(entity);
				if (entity.IsActive())
				{
					InvokeEntityEnableObserver_Internal(entity);
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
					if (archetypeEntityData.IsValid())
					{
						entity.Set_Internal(EntityDataHandle(archetypeEntityData.m_EntityData));

						InvokeEntityCreateObserver_Internal(entity);
						if (entity.IsActive())
						{
							InvokeEntityEnableObserver_Internal(entity);
						}
					}
				}
			});
		}

		// invoking components creation observers
		{
			m_ComponentContextManager.IterateOverComponentContexts([&](ComponentContextBase* componentContext)
			{
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
					if (typeData.IsTag())
					{
						return;
					}

					auto* packedContainer = typeData.m_PackedContainer;
					const auto& entityDataArray = archetype->m_EntitiesData;

					for (int64_t idx = static_cast<int64_t>(entitiesCountToInvokeCallbacks) - 1; idx >= 0; idx--)
					{
						const auto& archetypeEntityData = entityDataArray[idx];
						if (archetypeEntityData.IsValid())
						{
							auto entityData = archetypeEntityData.m_EntityData;
							entity.Set_Internal(EntityDataHandle(entityData));

							Archetype* currentArch = entityData->m_Archetype;
							ComponentBase* componentPtr = packedContainer->GetComponentBasePtr(entityData->m_IndexInArchetype);
							componentContext->InvokeOnCreateComponent(componentPtr, entity);

							if (entity.IsActive())
							{
								componentContext->InvokeOnEnableComponent(componentPtr, entity);
							}
						}
					}
				});
			});
		}

		PerformDelayedDestruction();
	}

	void Container::InvokeEntitesOnDestroyListeners(bool bMarkEntitiesDead)
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

				m_ArchetypesMap.IterateOverArchetypesWithType(componentTypeID, [&](Archetype* archetype)
				{
					const uint64_t entityCount = archetype->EntityCount();
					if (entityCount == 0)
					{
						return;
					}

					const uint64_t compIdx = archetype->FindTypeIndex(componentTypeID);

					const auto& typeData = archetype->m_TypeData[compIdx];
					if (typeData.IsTag())
					{
						return;
					}
					auto* packedContainer = typeData.m_PackedContainer;
					const auto& entityData = archetype->m_EntitiesData;

					for (int64_t idx = 0; idx < (int64_t)entityCount; idx++)
					{
						const auto& archetypeEntityData = entityData[idx];
						if (archetypeEntityData.IsValid())
						{
							entity.Set_Internal(EntityDataHandle(archetypeEntityData.m_EntityData));
							auto compPtr = packedContainer->GetComponentBasePtr(idx);
							if (entity.IsActive())
							{
								componentContext->InvokeOnDisableComponent(compPtr, entity);
							}
							componentContext->InvokeOnDestroyComponent(compPtr, entity);
						}
					}
				});
			});
		}

		// invoking entity creation observers:
		if (bMarkEntitiesDead)
		{
			ContainerIterator iterator = {};
			iterator.Foreach(*this, [&](const decs::Entity& entity)
			{
				if (entity.IsActive())
				{
					InvokeEntityDisableObserver_Internal(entity);
				}
				InvokeEntityDestroyObserver_Internal(entity);
				entity.GetEntityData()->SetState(EEntityState::Dead);
			});
		}
		else
		{
			ContainerIterator iterator = {};
			iterator.Foreach(*this, [&](const decs::Entity& entity)
			{
				if (entity.IsActive())
				{
					InvokeEntityDisableObserver_Internal(entity);
				}
				InvokeEntityDestroyObserver_Internal(entity);
			});
		}
	}

	void Container::InvokeEntityCreateEnableObservers(const decs::Entity& entity)
	{
		if (entity.IsValid())
		{
			auto entityData = entity.GetEntityData();
			if (!entityData->m_bIsCreatedByContainer)
			{
				entityData->m_bIsCreatedByContainer = true;
				if (m_CreateEntityObserver != nullptr)
				{
					m_CreateEntityObserver->OnCreateEntity(entity);
				}

				if (entity.IsActive() && !entityData->m_bIsEnabledByContainer)
				{
					entityData->m_bIsEnabledByContainer = true;
					if (m_EnableEntityObserver != nullptr)
					{
						m_EnableEntityObserver->OnEnableEntity(entity);
					}
				}
			}

			Archetype* archetype = entityData->m_Archetype;
			if (archetype != nullptr)
			{
				uint32_t observerInvokeCount = static_cast<uint32_t>(archetype->m_ComponentContextsInOrder.size()); // must use this becouse orderContextVector does not contain observers for tags

				Archetype* lastArchetype = archetype;

				for (uint64_t componentIdx = 0; componentIdx < observerInvokeCount; componentIdx++)
				{
					uint32_t indexInArchetype = entityData->m_IndexInArchetype;
					auto& orderData = archetype->m_ComponentContextsInOrder[componentIdx];
					auto& typeData = archetype->m_TypeData[orderData.m_ComponentIndex];

					TypeID lastComponentTypeID = typeData.m_TypeID;
					int observerOrder = orderData.m_ComponentContext->GetObserverOrder();

					ComponentBase* componentPtr = typeData.m_PackedContainer->GetComponentBasePtr(indexInArchetype);

					if (componentPtr != nullptr)
					{
						orderData.m_ComponentContext->InvokeOnCreateComponent(componentPtr, entity);

						if (entity.IsActive())
						{
							orderData.m_ComponentContext->InvokeOnEnableComponent(componentPtr, entity);
						}
					}

				#pragma region RESOLVING ARCHETYPE CHANGE
					lastArchetype = entityData->m_Archetype;

					if (lastArchetype != archetype)
					{
						// if archetype chagned in callbacks then we find where to start continuing invoking observers

						if (lastArchetype == nullptr)
						{
							break;
						}

						if (lastArchetype->FindTypeIndex(lastComponentTypeID) == std::numeric_limits<uint32_t>::max())
						{
							// we removed current component so we need find where to start invoking based on componnet order value
							for (uint32_t oi = 0; oi < lastArchetype->m_ComponentContextsInOrder.size(); oi++)
							{
								if (lastArchetype->m_ComponentContextsInOrder[oi].m_ComponentContext->GetObserverOrder() >= observerOrder)
								{
									// we make minus one becaouse for loop will increment index by one
									componentIdx = oi - 1;
									break;
								}
							}
						}
						else
						{
							for (uint32_t oi = 0; oi < lastArchetype->m_ComponentContextsInOrder.size(); oi++)
							{
								if (lastArchetype->m_ComponentContextsInOrder[oi].m_ComponentContext->GetComponentTypeID() == lastComponentTypeID)
								{
									// we asigning oi index becaouse for loop will increment index by one
									componentIdx = oi;
									break;
								}
							}
						}

						archetype = lastArchetype;
						observerInvokeCount = static_cast<uint32_t>(archetype->m_ComponentContextsInOrder.size());
					}
				#pragma endregion
				}
			}
		}
	}

	void Container::InvokeEntityCreateObserver_Internal(const Entity& entity)
	{
		auto entityData = entity.GetEntityData();
		if (!entityData->m_bIsCreatedByContainer)
		{
			entityData->m_bIsCreatedByContainer = true;
			if (m_CreateEntityObserver != nullptr)
			{
				m_CreateEntityObserver->OnCreateEntity(entity);
			}
		}
	}

	void Container::InvokeEntityDestroyObserver_Internal(const Entity& entity)
	{
		auto entityData = entity.GetEntityData();
		if (entityData->m_bIsCreatedByContainer)
		{
			entityData->m_bIsCreatedByContainer = false;
			if (m_DestroyEntityObserver != nullptr)
			{
				m_DestroyEntityObserver->OnDestroyEntity(entity);
			}
		}
	}

	void Container::InvokeEntityEnableObserver_Internal(const Entity& entity)
	{
		auto entityData = entity.GetEntityData();
		if (!entityData->m_bIsEnabledByContainer)
		{
			entityData->m_bIsEnabledByContainer = true;
			if (m_EnableEntityObserver != nullptr)
			{
				m_EnableEntityObserver->OnEnableEntity(entity);
			}
		}
	}

	void Container::InvokeEntityDisableObserver_Internal(const Entity& entity)
	{
		auto entityData = entity.GetEntityData();
		if (entityData->m_bIsEnabledByContainer)
		{
			entityData->m_bIsEnabledByContainer = false;
			if (m_DisableEntityObserver != nullptr)
			{
				m_DisableEntityObserver->OnDisableEntity(entity);
			}
		}
	}

	void Container::InvokeEntityAndComponentEnableObservers_Internal(const Entity& entity)
	{
		if (m_EnableEntityObserver != nullptr)
		{
			m_EnableEntityObserver->OnEnableEntity(entity);
		}

		EntityData& entityData = *entity.GetEntityData();
		uint32_t entityIndexInArchetype = entityData.m_IndexInArchetype;

		if (entityData.m_Archetype != nullptr)
		{
			uint64_t startRefsIdx = m_ActivationChangeComponentPtrs.size();
			uint64_t refCount = entityData.m_Archetype->GetComponentAndTagCount();

			m_ActivationChangeComponentPtrs.reserve(startRefsIdx + refCount);

			// fetch components refs
			auto& componentsTypeData = entityData.m_Archetype->m_TypeData;
			for (uint64_t idx = 0; idx < refCount; idx++)
			{
				auto& typeData = componentsTypeData[idx];
				if (!typeData.IsTag())
				{
					m_ActivationChangeComponentPtrs.push_back(
						typeData.m_PackedContainer->GetComponentBasePtr(entityIndexInArchetype)
					);
				}
				else
				{
					m_ActivationChangeComponentPtrs.push_back(nullptr);
				}
			}

			// invoke components activation listeners:
			auto& componentOrderData = entityData.m_Archetype->m_ComponentContextsInOrder;
			uint32_t componentInOrderCount = entityData.m_Archetype->GetComponentOnlyCount();
			for (uint64_t idx = 0; idx < componentInOrderCount; idx++)
			{
				auto& orderData = componentOrderData[idx];
				ComponentBase* compPtr = m_ActivationChangeComponentPtrs[startRefsIdx + orderData.m_ComponentIndex];
				if (compPtr != nullptr)
				{
					orderData.m_ComponentContext->InvokeOnEnableComponent(compPtr, entity);
				}
			}

			// erase used component refs:
			m_ActivationChangeComponentPtrs.erase(m_ActivationChangeComponentPtrs.begin() + startRefsIdx, m_ActivationChangeComponentPtrs.end());
		}
	}

	void Container::InvokeEntityAndComponentsDisableObservers_Internal(const Entity& entity)
	{
		if (m_DisableEntityObserver != nullptr)
		{
			m_DisableEntityObserver->OnDisableEntity(entity);
		}

		// TODO: add components deactivation listeners invoking
		EntityData& entityData = *entity.GetEntityData();
		uint32_t entityIndexInArchetype = entityData.m_IndexInArchetype;

		if (entityData.m_Archetype != nullptr)
		{
			uint64_t startRefsIdx = m_ActivationChangeComponentPtrs.size();
			uint64_t refCount = entityData.m_Archetype->GetComponentAndTagCount();

			m_ActivationChangeComponentPtrs.reserve(startRefsIdx + refCount);

			// fetch components refs
			auto& componentsTypeData = entityData.m_Archetype->m_TypeData;
			for (uint64_t idx = 0; idx < refCount; idx++)
			{
				auto& typeData = componentsTypeData[idx];
				if (!typeData.IsTag())
				{
					m_ActivationChangeComponentPtrs.push_back(typeData.m_PackedContainer->GetComponentBasePtr(entityIndexInArchetype));
				}
				else
				{
					m_ActivationChangeComponentPtrs.push_back(nullptr);
				}
			}

			// invoke components activation listeners:
			auto& componentOrderData = entityData.m_Archetype->m_ComponentContextsInOrder;
			uint32_t componentInOrderCount = entityData.m_Archetype->GetComponentOnlyCount();
			for (uint64_t idx = 0; idx < componentInOrderCount; idx++)
			{
				auto& orderData = componentOrderData[idx];
				ComponentBase* compPtr = m_ActivationChangeComponentPtrs[startRefsIdx + orderData.m_ComponentIndex];
				if (compPtr != nullptr)
				{
					orderData.m_ComponentContext->InvokeOnDisableComponent(compPtr, entity);
				}
			}

			// erase used component refs:
			m_ActivationChangeComponentPtrs.erase(m_ActivationChangeComponentPtrs.begin() + startRefsIdx, m_ActivationChangeComponentPtrs.end());
		}
	}

	void Container::PerformDelayedDestroy(uint64_t maxEntitiesToDestroy)
	{
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
			e.Set_Internal(EntityDataHandle(entityDataToDelayedDestroy.entityData));
			DestroyDelayedEntity(e, entityDataToDelayedDestroy.bInvokeCallbacks);
		}
		m_DelayedEntitiesToDestroy.clear();
	}

	void Container::RemoveArchetypesRecordsDelayedToRemove()
	{
		for (const auto& record : m_ArchetypesRecordsToDelayedRemove)
		{
			if (record.bRemoveAfterRemoveComponent)
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
		Archetype* currentArchetype = entity.GetEntityData()->m_Archetype;
		EntityData& entityData = *entity.GetEntityData();

		// Destroy callbacks:

		if (bInvokeCallbacks)
		{
			if (currentArchetype != nullptr)
			{
				InvokeEntityComponentDestructionObservers(entity);
			}

			InvokeEntityDisableObserver_Internal(entity);
			InvokeEntityDestroyObserver_Internal(entity);
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

		m_EntityManager.DestroyEntity(entity.m_Handle);
	}

	void Container::AddEntityToDelayedDestroy(const Entity& entity, bool bInvokeCallbacks)
	{
		entity.GetEntityData()->SetState(EEntityState::DelayedToDestruction);
		m_DelayedEntitiesToDestroy.push_back({ entity.GetEntityData(), bInvokeCallbacks });
	}

	Entity Container::CreateEntity_NoCallbacks(bool bIsActive)
	{
		if (m_CanCreateEntities)
		{
			Entity e(m_EntityManager.CreateEntity(bIsActive, *this));
			AddToEmptyEntitiesRightAfterNewEntityCreation(*e.GetEntityData());
			return e;
		}
		return Entity();
	}

	bool Container::DestroyEntity_NoCallback(const Entity& entity)
	{
		if (entity.IsValid() && m_CanDestroyEntities && entity.GetContainer() == this)
		{
			DestroyEntityInternal(entity, false);
			return true;
		}

		return false;
	}

	Entity Container::Spawn_NoCallback(const Entity& prefab, bool bIsActive)
	{
		if (!m_CanSpawn || prefab.IsNull()) return Entity();

		Container* prefabContainer = prefab.GetContainer();
		EntityData& prefabEntityData = *prefab.GetEntityData();
		Archetype* prefabArchetype = prefabEntityData.m_Archetype;

		BoolSwitch prefabOperationsLock = { prefabEntityData.m_bIsUsedAsPrefab, true };

		Entity spawnedEntity(m_EntityManager.CreateEntity(bIsActive, *this));

		if (prefabArchetype == nullptr)
		{
			AddToEmptyEntitiesRightAfterNewEntityCreation(*spawnedEntity.GetEntityData());

			return spawnedEntity;
		}

		SpawnDataState spawnState(m_SpawnData);

		PrepareSpawnDataFromPrefab(prefabEntityData, prefabContainer);
		CreateEntityFromSpawnData(spawnedEntity, spawnState);

		m_SpawnData.PopBackSpawnState(spawnState.m_ArchetypeIndex, spawnState.m_CompRefsStart);

		return spawnedEntity;
	}

	bool Container::Spawn_NoCallback(const Entity& prefab, uint64_t spawnCount, bool bAreActive)
	{
		if (!m_CanSpawn || spawnCount == 0 || prefab.IsNull()) return false;

		Container* prefabContainer = prefab.GetContainer();
		EntityData& prefabEntityData = *prefab.GetEntityData();
		Archetype* prefabArchetype = prefabEntityData.m_Archetype;
		BoolSwitch prefabOperationsLock = { prefabEntityData.m_bIsUsedAsPrefab, true };

		if (prefabArchetype == nullptr)
		{
			for (uint64_t i = 0; i < spawnCount; i++)
			{
				Entity spawnedEntity(m_EntityManager.CreateEntity(bAreActive, *this));
				AddToEmptyEntitiesRightAfterNewEntityCreation(*spawnedEntity.GetEntityData());
			}
			return true;
		}

		SpawnDataState spawnState(m_SpawnData);

		PrepareSpawnDataFromPrefab(prefabEntityData, prefabContainer);

		Archetype* spawnArchetype = m_SpawnData.m_SpawnArchetypes[spawnState.m_ArchetypeIndex];
		spawnArchetype->ReserveSpaceInArchetype(spawnArchetype->EntityCount() + spawnCount);

		for (uint64_t entityIdx = 0; entityIdx < spawnCount; entityIdx++)
		{
			Entity spawnedEntity(m_EntityManager.CreateEntity(bAreActive, *this));
			CreateEntityFromSpawnData(spawnedEntity, spawnState);
		}

		m_SpawnData.PopBackSpawnState(spawnState.m_ArchetypeIndex, spawnState.m_CompRefsStart);

		return true;
	}

	bool Container::Spawn_NoCallback(const Entity& prefab, std::vector<Entity>& spawnedEntities, uint64_t spawnCount, bool bAreActive)
	{
		if (!m_CanSpawn || spawnCount == 0 || prefab.IsNull()) return false;

		Container* prefabContainer = prefab.GetContainer();
		EntityData& prefabEntityData = *prefab.GetEntityData();
		Archetype* prefabArchetype = prefabEntityData.m_Archetype;
		BoolSwitch prefabOperationsLock = { prefabEntityData.m_bIsUsedAsPrefab, true };

		spawnedEntities.reserve(spawnedEntities.size() + spawnCount);

		if (prefabArchetype == nullptr)
		{
			for (uint64_t i = 0; i < spawnCount; i++)
			{
				Entity& spawnedEntity = spawnedEntities.emplace_back(m_EntityManager.CreateEntity(bAreActive, *this));
				AddToEmptyEntitiesRightAfterNewEntityCreation(*spawnedEntity.GetEntityData());
			}
			return true;
		}

		SpawnDataState spawnState(m_SpawnData);

		PrepareSpawnDataFromPrefab(prefabEntityData, prefabContainer);

		Archetype* spawnArchetype = m_SpawnData.m_SpawnArchetypes[spawnState.m_ArchetypeIndex];
		spawnArchetype->ReserveSpaceInArchetype(spawnArchetype->EntityCount() + spawnCount);

		for (uint64_t entityIdx = 0; entityIdx < spawnCount; entityIdx++)
		{
			Entity& spawnedEntity = spawnedEntities.emplace_back(m_EntityManager.CreateEntity(bAreActive, *this));
			CreateEntityFromSpawnData(spawnedEntity, spawnState);
		}

		m_SpawnData.PopBackSpawnState(spawnState.m_ArchetypeIndex, spawnState.m_CompRefsStart);

		return true;
	}

	void Container::SetEntityActive_NoCallback(const Entity& entity, bool bIsActive)
	{
		if (entity.GetContainer() == this
			&& entity.GetEntityData()->IsAlive()
			&& entity.GetEntityData()->IsActive() != bIsActive)
		{
			entity.GetEntityData()->SetActiveState(bIsActive);
		}
	}

	bool Container::RemoveComponent_NoCallback(const Entity& entity, TypeID componentTypeID)
	{
		if (entity.GetContainer() != this) return false;

		EntityData& entityData = *entity.GetEntityData();
		if (entityData.m_Archetype == nullptr || !entityData.IsValidToPerformComponentOperation()) return false;

		uint32_t compIdxInArch = entityData.m_Archetype->FindTypeIndex(componentTypeID);
		if (compIdxInArch == std::numeric_limits<uint32_t>::max()) return false;

		Archetype* oldArchetype = entityData.m_Archetype;
		uint64_t entityIndexInOldArchetype = entityData.m_IndexInArchetype;

		ArchetypeTypeData& archetypeTypeData = oldArchetype->m_TypeData[compIdxInArch];
		if (archetypeTypeData.IsTag())
		{
			return false;
		}

		auto& packedContainer = archetypeTypeData.m_PackedContainer;

		auto componentPtr = packedContainer->GetComponentBasePtr(entityIndexInOldArchetype);
		if (componentPtr->GetDependecyCount() > 0)
		{
			return false;
		}

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
