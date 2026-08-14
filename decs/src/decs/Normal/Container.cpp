#pragma once
#pragma once
#include "Container.h"
#include "Entity.h"

#include "Utils/ContainerIterator.h"

namespace decs
{
	Container::Container() :
		m_EntityManager(m_DefaultEntitiesChunkSize),
		m_QueryManager(this),
		m_ArchetypesMap(m_QueryManager, 100, 100)
	{
		InitializeLifeTimeData();
	}

	Container::Container(const ContainerConfig& config) :
		m_EntityManager(config.EntityChunkSize),
		m_ComponentContextManager(static_cast<uint32_t>(config.DefaultComponentChunkSize)),
		m_QueryManager(this),
		m_ArchetypesMap(m_QueryManager, config.ArchetypeChunkSize, 100)
	{
		InitializeLifeTimeData();
	}

	Container::~Container()
	{
		m_QueryManager.OnDestroyContainer();
		m_ComponentContextManager.ClearStableContainers();
		m_ArchetypesMap.ClearEntityDataAndComponents();
		DestroyLifeTimeData();
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
		m_LifeTimeData->m_bIsContainerAlive = false;
	}

	void Container::ReturnOwnedEntitiesToEntityManager_Internal()
	{
		ContainerIterator iterator = {};
		iterator.Foreach(*this, [this] (const decs::Entity& entity)
		{
			m_EntityManager.DestroyEntity(GetEntityData(entity));
		});
	}

	Entity Container::CreateEntity(bool bIsActive)
	{
		if (m_CanCreateEntities)
		{
			EntityData* entityData = m_EntityManager.CreateEntity(bIsActive);
			Entity e(*this, *entityData);
			AddToEmptyEntitiesRightAfterNewEntityCreation(*entityData);
			InvokeEntityCreateObserver_Internal(*entityData, e);
			if (bIsActive)
			{
				InvokeEntityEnableObserver_Internal(*entityData, e);
			}

			return e;
		}
		return Entity();
	}

	bool Container::DestroyEntity(const Entity& entity)
	{
		if (entity.m_LifeTimeData != m_LifeTimeData)
		{
			return false;
		}

		if (EntityData* entityData = entity.TryGetEntityData())
		{
			return DestroyEntityInternal(*entityData, entity, true);
		}
		return false;
	}

	void Container::InitializeLifeTimeData()
	{
		m_LifeTimeData = TRefCountHandle<ContainerLifetimeData>::Create(this);
	}

	void Container::DestroyLifeTimeData()
	{
		m_LifeTimeData->MarkDead();
		m_LifeTimeData.Reset();
	}

	bool Container::DestroyEntityInternal(EntityData& entityData, const Entity& entity, bool bInvokeObservers)
	{
		if (m_CanDestroyEntities && entity.GetContainer() == this)
		{
			EntityID entityID = entity.GetID();
			if (!entityData.CanBeDestructed())
			{
				return false;
			}

			if (m_PerformDelayedDestruction)
			{
				if (entityData.IsDelayedToDestruction())
				{
					return false;
				}
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

					InvokeEntityComponentDestructionObservers(entityData, entity);
				}
				InvokeEntityDisableObserver_Internal(entityData, entity);
				InvokeEntityDestroyObserver_Internal(entityData, entity);
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

			m_EntityManager.DestroyEntity(&entityData);

			return true;
		}

		return false;
	}

	void Container::SetEntityActive(EntityData& entityData, const Entity& entity, bool bIsActive)
	{
		if (entity.GetContainer() != this)
		{
			return;
		}

		if (entityData.IsValidToChangeActiveState()
			&& entityData.IsActiveFlag() != bIsActive
			)
		{
			const bool bOldEntityActiveState = entityData.IsActive();
			entityData.SetActiveState(bIsActive);
			const bool bNewEntityActiveState = entityData.IsActive();

			if (bOldEntityActiveState != bNewEntityActiveState)
			{
				if (bNewEntityActiveState)
				{
					InvokeEntityAndComponentEnableObservers_Internal(entityData, entity);
				}
				else
				{
					InvokeEntityAndComponentsDisableObservers_Internal(entityData, entity);
				}
			}
		}
	}

	bool Container::IsEntityActive(const Entity& entity, const EntityData& entityData) const
	{
		return entityData.IsActiveWithVersion(entity.GetVersion());
	}

	EntityData* Container::GetEntityData(const Entity& entity) const
	{
		return m_EntityManager.GetEntityData(entity.GetID());
	}

	void Container::SetEntityActiveOverride(EntityData& entityData, const Entity& entity, bool bIsActiveOverride)
	{
		if (entity.GetContainer() == this)
		{
			if (entityData.IsValidToChangeActiveState())
			{
				const bool bOldEntityActiveState = entityData.IsActive();
				entityData.SetDisableOverride(bIsActiveOverride);
				const bool bNewEntityActiveState = entityData.IsActive();

				if (bOldEntityActiveState != bNewEntityActiveState)
				{
					if (bNewEntityActiveState)
					{
						InvokeEntityAndComponentEnableObservers_Internal(entityData, entity);
					}
					else
					{
						InvokeEntityAndComponentsDisableObservers_Internal(entityData, entity);
					}
				}
			}
		}
	}

	void Container::SetEntityDisabledOverrideCount(EntityData& entityData, const Entity& entity, uint32_t disabledOverrideCount)
	{
		if (entity.GetContainer() == this)
		{
			if (entityData.IsValidToChangeActiveState())
			{
				const bool bOldEntityActiveState = entityData.IsActive();
				entityData.SetDisabledOverrideCount(disabledOverrideCount);
				const bool bNewEntityActiveState = entityData.IsActive();

				if (bOldEntityActiveState != bNewEntityActiveState)
				{
					if (bNewEntityActiveState)
					{
						InvokeEntityAndComponentEnableObservers_Internal(entityData, entity);
					}
					else
					{
						InvokeEntityAndComponentsDisableObservers_Internal(entityData, entity);
					}
				}
			}
		}
	}

	void Container::ResetDisabledOverrideCount(EntityData& entityData, const Entity& entity)
	{
		SetEntityDisabledOverrideCount(entityData, entity, 0);
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

	void Container::InvokeEntityComponentDestructionObservers(EntityData& entityData, const Entity& entity)
	{
		Archetype* currentArchetype = entityData.m_Archetype;
		const uint32_t componentsCount = currentArchetype->GetComponentOnlyCount();

		auto& typeDatas = currentArchetype->m_TypeData;
		auto& orderDatas = currentArchetype->m_ComponentContextsInOrder;

		// Invoke On destroy methods

		if (entityData.IsActiveWithVersion(entity.GetVersion()))
		{
			for (uint64_t i = 0; i < componentsCount; i++)
			{
				const uint32_t indexInArchetype = entityData.m_IndexInArchetype;
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
				const uint32_t indexInArchetype = entityData.m_IndexInArchetype;
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

		Container* prefabContainer = prefab.GetContainer_Internal();
		EntityData& prefabEntityData = *prefabContainer->GetEntityData(prefab);
		Archetype* prefabArchetype = prefabEntityData.m_Archetype;

		EntityData* spawnedEntityData = m_EntityManager.CreateEntity(bIsActive);
		Entity spawnedEntity(*this, *spawnedEntityData);

		if (prefabArchetype == nullptr)
		{
			AddToEmptyEntitiesRightAfterNewEntityCreation(*spawnedEntityData);

			InvokeEntityCreateObserver_Internal(*spawnedEntityData, spawnedEntity);

			if (spawnedEntityData->IsActiveWithVersion(spawnedEntity.GetVersion()))
			{
				InvokeEntityEnableObserver_Internal(*spawnedEntityData, spawnedEntity);
			}

			return spawnedEntity;
		}

		SpawnDataState spawnState(m_SpawnData);

		Archetype* spawnArchetype = nullptr;
		PrepareSpawnDataFromPrefab(prefabEntityData, *prefabContainer, spawnArchetype);
		DECS_ASSERT(spawnArchetype != nullptr, "Archetype must not be nullptr!");

		CreateEntityFromSpawnData(spawnState, *spawnedEntityData, spawnedEntity, *spawnArchetype);

		InvokeEntityCreateObserver_Internal(*spawnedEntityData, spawnedEntity);
		if (spawnedEntityData->IsActiveWithVersion(spawnedEntity.GetVersion()))
		{
			InvokeEntityEnableObserver_Internal(*spawnedEntityData, spawnedEntity);
		}
		InvokeComponentCreateAndEnableObserversOnSpawn(*spawnedEntityData, spawnedEntity, *spawnArchetype, spawnState);

		m_SpawnData.PopBackSpawnState(spawnState.m_ComponentDataStart);

		return spawnedEntity;
	}

	bool Container::Spawn(const Entity& prefab, uint64_t spawnCount, bool areActive)
	{
		if (!m_CanSpawn || spawnCount == 0 || prefab.IsNull()) return false;

		Container* prefabContainer = prefab.GetContainer_Internal();
		EntityData& prefabEntityData = *prefabContainer->GetEntityData(prefab);
		Archetype* prefabArchetype = prefabEntityData.m_Archetype;

		if (prefabArchetype == nullptr)
		{
			for (uint64_t i = 0; i < spawnCount; i++)
			{
				EntityData* spawnedEntityData = m_EntityManager.CreateEntity(areActive);
				Entity spawnedEntity(*this, *spawnedEntityData);

				AddToEmptyEntitiesRightAfterNewEntityCreation(*spawnedEntityData);

				InvokeEntityCreateObserver_Internal(*spawnedEntityData, spawnedEntity);
				if (spawnedEntityData->IsActiveWithVersion(spawnedEntity.GetVersion()))
				{
					InvokeEntityEnableObserver_Internal(*spawnedEntityData, spawnedEntity);
				}
			}
			return true;
		}

		SpawnDataState spawnState(m_SpawnData);

		Archetype* spawnArchetype = nullptr;
		PrepareSpawnDataFromPrefab(prefabEntityData, *prefabContainer, spawnArchetype);
		DECS_ASSERT(spawnArchetype != nullptr, "Archetype must not be nullptr!");

		spawnArchetype->ReserveSpaceInArchetype(spawnArchetype->EntityCount() + spawnCount);

		for (uint64_t entityIdx = 0; entityIdx < spawnCount; entityIdx++)
		{
			EntityData* spawnedEntityData = m_EntityManager.CreateEntity(areActive);
			Entity spawnedEntity(*this, *spawnedEntityData);

			CreateEntityFromSpawnData(spawnState, *spawnedEntityData, spawnedEntity, *spawnArchetype);

			InvokeEntityCreateObserver_Internal(*spawnedEntityData, spawnedEntity);
			if (spawnedEntityData->IsActiveWithVersion(spawnedEntity.GetVersion()))
			{
				InvokeEntityEnableObserver_Internal(*spawnedEntityData, spawnedEntity);
			}
			InvokeComponentCreateAndEnableObserversOnSpawn(*spawnedEntityData, spawnedEntity, *spawnArchetype, spawnState);
		}

		m_SpawnData.PopBackSpawnState(spawnState.m_ComponentDataStart);

		return true;
	}

	bool Container::Spawn(const Entity& prefab, ecsVector<Entity>& spawnedEntities, uint64_t spawnCount, bool areActive)
	{
		if (!m_CanSpawn || spawnCount == 0 || prefab.IsNull()) return false;

		Container* prefabContainer = prefab.GetContainer_Internal();
		EntityData& prefabEntityData = *prefabContainer->GetEntityData(prefab);
		Archetype* prefabArchetype = prefabEntityData.m_Archetype;

		spawnedEntities.reserve(spawnedEntities.size() + spawnCount);

		if (prefabArchetype == nullptr)
		{
			for (uint64_t i = 0; i < spawnCount; i++)
			{
				EntityData* spawnedEntityData = m_EntityManager.CreateEntity(areActive);
				Entity& spawnedEntity = spawnedEntities.emplace_back(*this, *spawnedEntityData);
				AddToEmptyEntitiesRightAfterNewEntityCreation(*spawnedEntityData);

				InvokeEntityCreateObserver_Internal(*spawnedEntityData, spawnedEntity);
				if (spawnedEntityData->IsActiveWithVersion(spawnedEntity.GetVersion()))
				{
					InvokeEntityEnableObserver_Internal(*spawnedEntityData, spawnedEntity);
				}
			}
			return true;
		}

		SpawnDataState spawnState(m_SpawnData);

		Archetype* spawnArchetype = nullptr;
		PrepareSpawnDataFromPrefab(prefabEntityData, *prefabContainer, spawnArchetype);
		DECS_ASSERT(spawnArchetype != nullptr, "Archetype must not be nullptr!");

		spawnArchetype->ReserveSpaceInArchetype(spawnArchetype->EntityCount() + spawnCount);

		for (uint64_t entityIdx = 0; entityIdx < spawnCount; entityIdx++)
		{
			EntityData* spawnedEntityData = m_EntityManager.CreateEntity(areActive);
			Entity& spawnedEntity = spawnedEntities.emplace_back(*this, *spawnedEntityData);

			CreateEntityFromSpawnData(spawnState, *spawnedEntityData, spawnedEntity, *spawnArchetype);

			InvokeEntityCreateObserver_Internal(*spawnedEntityData, spawnedEntity);
			if (spawnedEntityData->IsActiveWithVersion(spawnedEntity.GetVersion()))
			{
				InvokeEntityEnableObserver_Internal(*spawnedEntityData, spawnedEntity);
			}

			InvokeComponentCreateAndEnableObserversOnSpawn(*spawnedEntityData, spawnedEntity, *spawnArchetype, spawnState);
		}

		m_SpawnData.PopBackSpawnState(spawnState.m_ComponentDataStart);

		return true;
	}

	void Container::PrepareSpawnDataFromPrefab(
		const EntityData& prefabEntityData,
		const Container& prefabContainer,
		Archetype*& spawnArchetype
	)
	{
		const Archetype& prefabArchetype = *prefabEntityData.m_Archetype;
		const uint32_t prefabIndexInArchetype = prefabEntityData.m_IndexInArchetype;
		const uint64_t typeCount = prefabArchetype.GetComponentTagCount();

		if ((&prefabContainer) == this)
		{
			spawnArchetype = prefabEntityData.m_Archetype;
		}
		else
		{
			spawnArchetype = m_ArchetypesMap.GetOrCreateMatchedArchetype(*prefabEntityData.m_Archetype, &m_ComponentContextManager);
		}

		for (uint32_t i = 0; i < typeCount; i++)
		{
			const ArchetypeTypeData& prefabTypeData = prefabArchetype.m_TypeData[i];
			ArchetypeTypeData& spawnTypeData = spawnArchetype->m_TypeData[i];

			if (prefabTypeData.IsTag())
			{
				m_SpawnData.m_ComponentData.emplace_back();
			}
			else
			{
				m_SpawnData.m_ComponentData.emplace_back(
					prefabTypeData.m_PackedContainer->GetComponentBasePtr(prefabIndexInArchetype),
					spawnTypeData.m_StableContainer,
					spawnTypeData.m_ComponentContext
				);
			}
		}
	}

	void Container::CreateEntityFromSpawnData(
		const SpawnDataState& spawnState,
		EntityData& spawnedEntityData,
		const Entity& spawnedEntity,
		Archetype& spawnArchetype
	)
	{
		spawnArchetype.AddEntityData(&spawnedEntityData);

		auto& archetypeTypeData = spawnArchetype.m_TypeData;
		uint64_t typeCount = spawnArchetype.GetComponentAndTagCount();
		for (uint32_t i = 0; i < typeCount; i++)
		{
			ArchetypeTypeData& currentTypeData = archetypeTypeData[i];
			SpawnComponentData& spawnComponentData = m_SpawnData.m_ComponentData[i + spawnState.m_ComponentDataStart];

			if (spawnComponentData.IsTag())
			{
				continue;
			}

			auto spawnedCompPtr = spawnComponentData.m_SpawnStableContainer->CreateFromComponentBase(spawnedEntity, spawnComponentData.m_PrefabComponent);
			currentTypeData.m_PackedContainer->PushBackFromBase(spawnedCompPtr);

			spawnComponentData.m_SpawnedComponent = spawnedCompPtr;
		}
	}

	void Container::InvokeComponentCreateAndEnableObserversOnSpawn(const EntityData& entityData, const Entity& entity, const Archetype& archetype, const SpawnDataState& spawnState)
	{
		auto& orderContextVector = archetype.m_ComponentContextsInOrder;
		const uint32_t observerInvokeCount = static_cast<uint32_t>(orderContextVector.size()); // must use this becouse orderContextVector does not contain observers for tags

		for (uint64_t idx = 0; idx < observerInvokeCount; idx++)
		{
			auto& orderData = orderContextVector[idx];
			const uint32_t componentIdx = orderData.m_ComponentIndex;

			EntityComponent* spawnedComponent = m_SpawnData.m_ComponentData[spawnState.m_ComponentDataStart + componentIdx].m_SpawnedComponent;
			if (spawnedComponent != nullptr)
			{
				orderData.m_ComponentContext->InvokeOnCreateComponent(spawnedComponent, entity);

				if (entityData.IsActiveWithVersion(entity.GetVersion()))
				{
					orderData.m_ComponentContext->InvokeOnEnableComponent(spawnedComponent, entity);
				}
			}
		}
	}

	void Container::OnAddComponentInvokeObservers(
		const EntityData& entityData,
		const Entity& entity,
		IComponentContext* componentContext,
		IPackedComponentContainer* packedContainer,
		TypeID compTypeID
	)
	{
		Archetype* currentArch = entityData.m_Archetype;

		componentContext->InvokeOnCreateComponent(packedContainer->GetComponentBasePtr(entityData.m_IndexInArchetype), entity);

		// All this checks are here to check if this entity containe components after OnCreateMethod
		Archetype* newArch = entityData.m_Archetype;
		if (newArch != nullptr && entityData.IsActiveWithVersion(entity.GetVersion()))
		{
			if (currentArch != newArch)
			{
				uint32_t compIndex = newArch->FindTypeIndex(compTypeID);
				if (compIndex < newArch->GetComponentAndTagCount())
				{
					componentContext->InvokeOnEnableComponent(
						newArch->m_TypeData[compIndex].m_PackedContainer->GetComponentBasePtr(entityData.m_IndexInArchetype),
						entity
					);
				}
			}
			else
			{
				componentContext->InvokeOnEnableComponent(packedContainer->GetComponentBasePtr(entityData.m_IndexInArchetype), entity);
			}
		}
	}

	bool Container::RemoveComponent_Impl(EntityData& entityData, const Entity& entity, TypeID componentTypeID, bool bInvokeObservers)
	{
		if (!m_CanRemoveComponents)
		{
			return false;
		}

		if (entity.GetContainer() != this) return false;

		if (entityData.m_Archetype == nullptr || !entityData.IsValidToPerformComponentOperation()) return false;

		uint32_t compIdxInArch = entityData.m_Archetype->FindTypeIndex(componentTypeID);
		if (compIdxInArch == std::numeric_limits<uint32_t>::max()) return false;

		Archetype* oldArchetype = entityData.m_Archetype;
		uint64_t indexInOldArchetype = entityData.m_IndexInArchetype;

		ArchetypeTypeData& oldArchetypeTypeData = oldArchetype->m_TypeData[compIdxInArch];
		if (oldArchetypeTypeData.IsTag())
		{
			return false;
		}

		auto packedContainer = oldArchetypeTypeData.m_PackedContainer;
		EntityComponent* componentPtr = packedContainer->GetComponentBasePtr(indexInOldArchetype);
		if (componentPtr->GetDependecyCount() > 0)
		{
			return false;
		}

		Archetype* newArchetype = m_ArchetypesMap.GetOrCreateArchetypeAfterRemoveComponent(*entityData.m_Archetype, componentTypeID);

		if (newArchetype != nullptr)
		{
			Archetype::MoveEntityAfterRemoveComponentWithoutDestroyingFromSource(*oldArchetype, *newArchetype, indexInOldArchetype, componentTypeID);
		}
		else
		{
			AddToEmptyEntities(entityData);
		}

		if (m_PerformDelayedDestruction)
		{
			AddArchetypeRecordToDelayedRemove(oldArchetype, static_cast<uint32_t>(indexInOldArchetype), true, componentTypeID);
		}
		else
		{
			oldArchetype->RemoveSwapBackEntityAfterRemoveComponent(indexInOldArchetype);
		}

		// Invoking remove observers:
		if (bInvokeObservers)
		{
			auto componentContext = oldArchetypeTypeData.m_ComponentContext;
			componentContext->InvokeOnDisableComponent(componentPtr, entity);
			componentContext->InvokeOnDestroyComponent(componentPtr, entity);
		}

		if (!m_PerformDelayedDestruction)
		{
			oldArchetypeTypeData.m_StableContainer->Destroy(componentPtr);
		}

		return true;
	}

	EntityComponent* Container::GetComponentAtIndex_ObserversOrder(EntityData& entityData, uint32_t componentIndex)
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

	EntityComponent* Container::GetComponentAtIndex_TypeIDOrder(EntityData& entityData, uint32_t componentIndex)
	{
		if (entityData.IsAlive() && entityData.m_Archetype != nullptr && componentIndex < entityData.m_Archetype->GetComponentAndTagCount())
		{
			auto& typeData = entityData.m_Archetype->m_TypeData[componentIndex];
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
			|| entityData.m_Archetype == nullptr
			|| !entityData.IsValidToPerformComponentOperation()
			)
		{
			return false;
		}

		uint32_t compIdxInArch = entityData.m_Archetype->FindTypeIndex(tagType);
		if (compIdxInArch == std::numeric_limits<uint32_t>::max())
		{
			return false;
		}

		Archetype* oldArchetype = entityData.m_Archetype;
		const uint64_t entityIndexInOldArchetype = entityData.m_IndexInArchetype;

		ArchetypeTypeData& archetypeTypeData = oldArchetype->m_TypeData[compIdxInArch];
		if (!archetypeTypeData.IsTag())
		{
			return false;
		}

		Archetype* newArchetype = GetArchetypeAfterRemoveTag(*oldArchetype, tagType);

		if (newArchetype != nullptr)
		{
			Archetype::MoveEntityAfterRemoveComponentWithoutDestroyingFromSource(*oldArchetype, *newArchetype, entityIndexInOldArchetype, tagType);
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
				EntityData* entiytData = m_EmptyEntities[i];
				entity.Set_Internal(*this, *entiytData);

				InvokeEntityCreateObserver_Internal(*entiytData, entity);
				if (entiytData->IsActiveWithVersion(entity.GetVersion()))
				{
					InvokeEntityEnableObserver_Internal(*entiytData, entity);
				}
			}

			m_ArchetypesMap.IterateOverArchetypes([&] (Archetype& archetype)
			{
				if (archetype.EntityCount() == 0)
				{
					return;
				}

				const auto& entityStorage = archetype.GetEntityStorage();

				for (int64_t idx = static_cast<int64_t>(archetype.EntityCount()) - 1; idx >= 0; idx--)
				{
					const auto archetypeEntityData = entityStorage.GetEntityRecord(idx);
					if (archetypeEntityData.IsValid())
					{
						entity.Set_Internal(*this, *archetypeEntityData.m_EntityData);

						InvokeEntityCreateObserver_Internal(*archetypeEntityData.m_EntityData, entity);
						if (archetypeEntityData.m_EntityData->IsActiveWithVersion(entity.GetVersion()))
						{
							InvokeEntityEnableObserver_Internal(*archetypeEntityData.m_EntityData, entity);
						}
					}
				}
			});
		}

		// invoking components creation observers
		{
			m_ComponentContextManager.IterateOverComponentContexts([&] (IComponentContext* componentContext)
			{
				InvokeComponentTypeCreateEnableObservers(*componentContext);
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
			m_ComponentContextManager.IterateOverComponentContextsForDestryObservers([&] (IComponentContext* componentContext)
			{
				InvokeComponentTypeDestroyDisableObservers(*componentContext);
			});
		}

		// invoking entity creation observers:
		if (bMarkEntitiesDead)
		{
			ContainerIterator iterator = {};
			iterator.Foreach(*this, [&] (const decs::Entity& entity)
			{
				EntityData* entityData = GetEntityData(entity);
				if (entityData->IsActiveWithVersion(entity.GetVersion()))
				{
					InvokeEntityDisableObserver_Internal(*entityData, entity);
				}
				InvokeEntityDestroyObserver_Internal(*entityData, entity);
				entityData->SetState(EEntityState::Dead);
			});
		}
		else
		{
			ContainerIterator iterator = {};
			iterator.Foreach(*this, [&] (const decs::Entity& entity)
			{
				EntityData* entityData = GetEntityData(entity);
				if (entityData->IsActiveWithVersion(entity.GetVersion()))
				{
					InvokeEntityDisableObserver_Internal(*entityData, entity);
				}
				InvokeEntityDestroyObserver_Internal(*entityData, entity);
			});
		}
	}

	bool Container::InvokeComponentOnCreateListeners(TypeID componentTypeID)
	{
		auto componentCtx = m_ComponentContextManager.GetComponentContext(componentTypeID);
		if (componentCtx == nullptr)
		{
			return false;
		}

		return InvokeComponentTypeCreateEnableObservers(*componentCtx);
	}

	bool Container::InvokeComponentOnDestroyListeners(TypeID componentTypeID)
	{
		auto componentCtx = m_ComponentContextManager.GetComponentContext(componentTypeID);
		if (componentCtx == nullptr)
		{
			return false;
		}

		return InvokeComponentTypeDestroyDisableObservers(*componentCtx);
	}

	void Container::InvokeEntityCreateEnableObservers(const decs::Entity& entity)
	{
		if (entity.m_LifeTimeData != m_LifeTimeData)
		{
			return;
		}
		auto entityData = entity.TryGetEntityData();
		if (entityData== nullptr)
		{
			return;
		}

		if (!entityData->m_bIsCreatedByContainer)
		{
			entityData->m_bIsCreatedByContainer = true;
			m_CreateEntityObservers.Invoke(entity);

			if (entityData->IsActiveWithVersion(entity.GetVersion()) && !entityData->m_bIsEnabledByContainer)
			{
				entityData->m_bIsEnabledByContainer = true;
				m_EnableEntityObservers.Invoke(entity);
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

				EntityComponent* componentPtr = typeData.m_PackedContainer->GetComponentBasePtr(indexInArchetype);

				if (componentPtr != nullptr)
				{
					orderData.m_ComponentContext->InvokeOnCreateComponent(componentPtr, entity);

					if (entityData->IsActiveWithVersion(entity.GetVersion()))
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

	void Container::InvokeEntityCreateObserver_Internal(EntityData& entityData, const Entity& entity)
	{
		if (!entityData.m_bIsCreatedByContainer)
		{
			entityData.m_bIsCreatedByContainer = true;
			m_CreateEntityObservers.Invoke(entity);
		}
	}

	void Container::InvokeEntityDestroyObserver_Internal(EntityData& entityData, const Entity& entity)
	{
		if (entityData.m_bIsCreatedByContainer)
		{
			entityData.m_bIsCreatedByContainer = false;
			m_DestroyEntityObservers.Invoke(entity);
		}
	}

	void Container::InvokeEntityEnableObserver_Internal(EntityData& entityData, const Entity& entity)
	{
		if (!entityData.m_bIsEnabledByContainer)
		{
			entityData.m_bIsEnabledByContainer = true;
			m_EnableEntityObservers.Invoke(entity);
		}
	}

	void Container::InvokeEntityDisableObserver_Internal(EntityData& entityData, const Entity& entity)
	{
		if (entityData.m_bIsEnabledByContainer)
		{
			entityData.m_bIsEnabledByContainer = false;
			m_DisableEntityObservers.Invoke(entity);
		}
	}

	void Container::InvokeEntityAndComponentEnableObservers_Internal(EntityData& entityData, const Entity& entity)
	{
		m_EnableEntityObservers.Invoke(entity);

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
				EntityComponent* compPtr = m_ActivationChangeComponentPtrs[startRefsIdx + orderData.m_ComponentIndex];
				if (compPtr != nullptr)
				{
					orderData.m_ComponentContext->InvokeOnEnableComponent(compPtr, entity);
				}
			}

			// erase used component refs:
			m_ActivationChangeComponentPtrs.erase(m_ActivationChangeComponentPtrs.begin() + startRefsIdx, m_ActivationChangeComponentPtrs.end());
		}
	}

	void Container::InvokeEntityAndComponentsDisableObservers_Internal(EntityData& entityData, const Entity& entity)
	{
		m_DisableEntityObservers.Invoke(entity);

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
				EntityComponent* compPtr = m_ActivationChangeComponentPtrs[startRefsIdx + orderData.m_ComponentIndex];
				if (compPtr != nullptr)
				{
					orderData.m_ComponentContext->InvokeOnDisableComponent(compPtr, entity);
				}
			}

			// erase used component refs:
			m_ActivationChangeComponentPtrs.erase(m_ActivationChangeComponentPtrs.begin() + startRefsIdx, m_ActivationChangeComponentPtrs.end());
		}
	}

	bool Container::InvokeComponentTypeCreateEnableObservers(IComponentContext& componentCtx)
	{
		const TypeID componentTypeID = componentCtx.GetComponentTypeID();

		Entity entity = {};
		entity.SetLifeTimeData_Internal(m_LifeTimeData);

		m_ArchetypesMap.IterateOverArchetypesWithType(componentTypeID, [&] (Archetype* archetype)
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
			const auto& entityStorage = archetype->GetEntityStorage();

			for (int64_t idx = static_cast<int64_t>(entityCount) - 1; idx >= 0; idx--)
			{
				const auto archetypeEntityData = entityStorage.GetEntityRecord(idx);
				if (archetypeEntityData.IsValid())
				{
					auto entityData = archetypeEntityData.m_EntityData;
					entity.SetWithoutLifeTimeDataInvalidation_Internal(*entityData);

					EntityComponent* componentPtr = packedContainer->GetComponentBasePtr(entityData->m_IndexInArchetype);
					componentCtx.InvokeOnCreateComponent(componentPtr, entity);

					if (entityData->IsActiveWithVersion(entity.GetVersion()))
					{
						componentCtx.InvokeOnEnableComponent(componentPtr, entity);
					}
				}
			}
		});

		return true;
	}

	bool Container::InvokeComponentTypeDestroyDisableObservers(IComponentContext& componentCtx)
	{
		const TypeID componentTypeID = componentCtx.GetComponentTypeID();
		Entity entity = {};
		entity.SetLifeTimeData_Internal(m_LifeTimeData);

		m_ArchetypesMap.IterateOverArchetypesWithType(componentTypeID, [&] (Archetype* archetype)
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
			const auto& entityStorage = archetype->GetEntityStorage();

			for (int64_t idx = 0; idx < static_cast<int64_t>(entityCount); idx++)
			{
				const auto archetypeEntityData = entityStorage.GetEntityRecord(idx);
				if (archetypeEntityData.IsValid())
				{
					entity.SetWithoutLifeTimeDataInvalidation_Internal(*archetypeEntityData.m_EntityData);
					auto compPtr = packedContainer->GetComponentBasePtr(idx);
					if (archetypeEntityData.m_EntityData->IsActiveWithVersion(entity.GetVersion()))
					{
						componentCtx.InvokeOnDisableComponent(compPtr, entity);
					}
					componentCtx.InvokeOnDestroyComponent(compPtr, entity);
				}
			}
		});
		return true;
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
			e.Set_Internal(*this, *entityDataToDelayedDestroy.entityData);
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
		EntityData* entityData = GetEntityData(entity);
		Archetype* currentArchetype = entityData->m_Archetype;

		// Destroy callbacks:
		if (bInvokeCallbacks)
		{
			if (currentArchetype != nullptr)
			{
				InvokeEntityComponentDestructionObservers(*entityData, entity);
			}

			InvokeEntityDisableObserver_Internal(*entityData, entity);
			InvokeEntityDestroyObserver_Internal(*entityData, entity);
		}

		if (currentArchetype != nullptr)
		{
			const uint32_t indexInArchetype = entityData->m_IndexInArchetype;
			currentArchetype->RemoveSwapBackEntity(indexInArchetype);
		}
		else
		{
			RemoveFromEmptyEntities(*entityData);
		}

		m_EntityManager.DestroyEntity(entityData);
	}

	void Container::AddEntityToDelayedDestroy(const Entity& entity, bool bInvokeCallbacks)
	{
		EntityData* entityData = GetEntityData(entity);
		entityData->SetState(EEntityState::DelayedToDestruction);
		m_DelayedEntitiesToDestroy.push_back({ entityData, bInvokeCallbacks });
	}

	Entity Container::CreateEntity_NoObserver(bool bIsActive)
	{
		if (m_CanCreateEntities)
		{
			EntityData* entityData = m_EntityManager.CreateEntity(bIsActive);
			Entity e(*this, *entityData);
			AddToEmptyEntitiesRightAfterNewEntityCreation(*entityData);
			return e;
		}
		return Entity();
	}

	bool Container::DestroyEntity_NoObserver(const Entity& entity)
	{
		if (entity.m_LifeTimeData != m_LifeTimeData || !m_CanDestroyEntities)
		{
			return false;
		}

		if (EntityData* entityData = entity.TryGetEntityData())
		{
			DestroyEntityInternal(*entityData, entity, false);
			return true;
		}

		return false;
	}

	Entity Container::Spawn_NoObserver(const Entity& prefab, bool bIsActive)
	{
		if (!m_CanSpawn || prefab.IsNull()) return Entity();

		Container* prefabContainer = prefab.GetContainer_Internal();
		EntityData& prefabEntityData = *prefabContainer->GetEntityData(prefab);
		Archetype* prefabArchetype = prefabEntityData.m_Archetype;

		EntityData* spawnedEntityData = m_EntityManager.CreateEntity(bIsActive);
		Entity spawnedEntity(*this, *spawnedEntityData);

		if (prefabArchetype == nullptr)
		{
			AddToEmptyEntitiesRightAfterNewEntityCreation(*spawnedEntityData);

			return spawnedEntity;
		}

		SpawnDataState spawnState(m_SpawnData);

		Archetype* spawnArchetype = nullptr;
		PrepareSpawnDataFromPrefab(prefabEntityData, *prefabContainer, spawnArchetype);
		DECS_ASSERT(spawnArchetype != nullptr, "Archetype must not be nullptr!");

		CreateEntityFromSpawnData(spawnState, *spawnedEntityData, spawnedEntity, *spawnArchetype);

		m_SpawnData.PopBackSpawnState(spawnState.m_ComponentDataStart);

		return spawnedEntity;
	}

	bool Container::Spawn_NoObserver(const Entity& prefab, uint64_t spawnCount, bool bAreActive)
	{
		if (!m_CanSpawn || spawnCount == 0 || prefab.IsNull()) return false;

		Container* prefabContainer = prefab.GetContainer_Internal();
		EntityData& prefabEntityData = *prefabContainer->GetEntityData(prefab);
		Archetype* prefabArchetype = prefabEntityData.m_Archetype;

		if (prefabArchetype == nullptr)
		{
			for (uint64_t i = 0; i < spawnCount; i++)
			{
				EntityData* spawnedEntityData = m_EntityManager.CreateEntity(bAreActive);
				Entity spawnedEntity(*this, *spawnedEntityData);
				AddToEmptyEntitiesRightAfterNewEntityCreation(*spawnedEntityData);
			}
			return true;
		}

		SpawnDataState spawnState(m_SpawnData);

		Archetype* spawnArchetype = nullptr;
		PrepareSpawnDataFromPrefab(prefabEntityData, *prefabContainer, spawnArchetype);
		DECS_ASSERT(spawnArchetype != nullptr, "Archetype must not be nullptr!");

		spawnArchetype->ReserveSpaceInArchetype(spawnArchetype->EntityCount() + spawnCount);

		for (uint64_t entityIdx = 0; entityIdx < spawnCount; entityIdx++)
		{
			EntityData* spawnedEntityData = m_EntityManager.CreateEntity(bAreActive);
			Entity spawnedEntity(*this, *spawnedEntityData);
			CreateEntityFromSpawnData(spawnState, *spawnedEntityData, spawnedEntity, *spawnArchetype);
		}

		m_SpawnData.PopBackSpawnState(spawnState.m_ComponentDataStart);

		return true;
	}

	bool Container::Spawn_NoObserver(const Entity& prefab, ecsVector<Entity>& spawnedEntities, uint64_t spawnCount, bool bAreActive)
	{
		if (!m_CanSpawn || spawnCount == 0 || prefab.IsNull()) return false;

		Container* prefabContainer = prefab.GetContainer_Internal();
		EntityData& prefabEntityData = *prefabContainer->GetEntityData(prefab);
		Archetype* prefabArchetype = prefabEntityData.m_Archetype;

		spawnedEntities.reserve(spawnedEntities.size() + spawnCount);

		if (prefabArchetype == nullptr)
		{
			for (uint64_t i = 0; i < spawnCount; i++)
			{
				EntityData* spawnedEntityData = m_EntityManager.CreateEntity(bAreActive);
				Entity& spawnedEntity = spawnedEntities.emplace_back(*this, *spawnedEntityData);
				AddToEmptyEntitiesRightAfterNewEntityCreation(*spawnedEntityData);
			}
			return true;
		}

		SpawnDataState spawnState(m_SpawnData);

		Archetype* spawnArchetype = nullptr;
		PrepareSpawnDataFromPrefab(prefabEntityData, *prefabContainer, spawnArchetype);
		DECS_ASSERT(spawnArchetype != nullptr, "Archetype must not be nullptr!");

		spawnArchetype->ReserveSpaceInArchetype(spawnArchetype->EntityCount() + spawnCount);

		for (uint64_t entityIdx = 0; entityIdx < spawnCount; entityIdx++)
		{
			EntityData* spawnedEntityData = m_EntityManager.CreateEntity(bAreActive);
			Entity& spawnedEntity = spawnedEntities.emplace_back(*this, *spawnedEntityData);
			CreateEntityFromSpawnData(spawnState, *spawnedEntityData, spawnedEntity, *spawnArchetype);
		}

		m_SpawnData.PopBackSpawnState(spawnState.m_ComponentDataStart);

		return true;
	}

	void Container::SetEntityActive_NoObserver(EntityData& entityData, const Entity& entity, bool bIsActive)
	{
		if (entityData.IsAlive() && entityData.IsActiveFlag() != bIsActive)
		{
			entityData.SetActiveState(bIsActive);
		}
	}

	void Container::SetEntityActiveOverride_NoObserver(EntityData& entityData, const Entity& entity, bool bIsActiveOverride)
	{
		if (m_LifeTimeData != entity.m_LifeTimeData)
		{
			return;
		}
		if (entityData.IsValidToChangeActiveState())
		{
			const bool bOldEntityActiveState = entityData.IsActive();
			entityData.SetDisableOverride(bIsActiveOverride);
			const bool bNewEntityActiveState = entityData.IsActive();
		}
	}

	void Container::SetEntityDisabledOverrideCount_NoObserver(EntityData& entityData, const Entity& entity, uint32_t disabledOverrideCount)
	{
		if (m_LifeTimeData != entity.m_LifeTimeData)
		{
			return;
		}
		if (entityData.IsValidToChangeActiveState())
		{
			const bool bOldEntityActiveState = entityData.IsActive();
			entityData.SetDisabledOverrideCount(disabledOverrideCount);
			const bool bNewEntityActiveState = entityData.IsActive();
		}
	}

	void Container::ResetDisabledOverrideCount_NoObserver(EntityData& entityData, const Entity& entity)
	{
		SetEntityDisabledOverrideCount_NoObserver(entityData, entity, 0);
	}

	void Container::TryDestroyArchetypes(ArchetypeDestroyState& state, const ArchetypeDestroyConfig& config)
	{
		m_ArchetypesMap.TryDestroyArchetypes(state, config);
	}

	void Container::AddQuery(IQuery* query)
	{
		m_QueryManager.AddQuery(query);
	}

	void Container::RemoveQuery(IQuery* query)
	{
		m_QueryManager.RemoveQuery(query);
	}

	void Container::AddMultiQuery(IMultiQuery* query)
	{
		m_QueryManager.AddMultiQuery(query);
	}

	void Container::RemoveMultiQuery(IMultiQuery* query)
	{
		m_QueryManager.RemoveMultiQuery(query);
	}
}
