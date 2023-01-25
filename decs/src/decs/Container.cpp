#pragma once
#include "Container.h"
#include "Entity.h"

namespace decs
{
	Container::Container() :
		m_HaveOwnEntityManager(true),
		m_EntityManager(new EntityManager(1000)),
		m_HaveOwnComponentContextManager(true),
		m_ComponentContexts(new ComponentContextsManager(1000, nullptr))
	{

	}

	Container::Container(
		const uint64_t& enititesChunkSize,
		const uint64_t& componentContainerChunkSize,
		const bool& invokeEntityActivationStateListeners,
		const uint64_t m_EmptyEntitiesChunkSize
	) :
		m_HaveOwnEntityManager(true),
		m_EntityManager(new EntityManager(enititesChunkSize)),
		m_HaveOwnComponentContextManager(true),
		m_ComponentContexts(new ComponentContextsManager(componentContainerChunkSize, nullptr)),
		m_bInvokeEntityActivationStateListeners(invokeEntityActivationStateListeners),
		m_EmptyEntities(m_EmptyEntitiesChunkSize)
	{
	}

	Container::Container(
		EntityManager* entityManager,
		ComponentContextsManager* componentContextsManager,
		const uint64_t& componentContainerChunkSize,
		const bool& invokeEntityActivationStateListeners,
		const uint64_t m_EmptyEntitiesChunkSize
	) :
		m_HaveOwnEntityManager(false),
		m_EntityManager(entityManager),
		m_HaveOwnComponentContextManager(false),
		m_ComponentContexts(componentContextsManager),
		m_bInvokeEntityActivationStateListeners(invokeEntityActivationStateListeners),
		m_EmptyEntities(m_EmptyEntitiesChunkSize)
	{
	}

	Container::~Container()
	{
		if (m_HaveOwnComponentContextManager)
		{
			delete m_ComponentContexts;
		}
		if (m_HaveOwnEntityManager) delete m_EntityManager;
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
				// Invoke On destroy metyhods

				for (uint64_t i = 0; i < currentArchetype->GetComponentsCount(); i++)
				{
					void* compPtr = currentArchetype->GetComponentVoidPtr(entityData.m_IndexInArchetype, i);
					auto compContext = currentArchetype->m_ComponentContexts[i];
					compContext->InvokeOnDestroyComponent_S(compPtr, entity);
				}

				uint64_t firstComponentDataIndexInArch = currentArchetype->GetComponentsCount() * entityData.m_IndexInArchetype;
				// Remove entity from component bucket:

				auto result = currentArchetype->RemoveSwapBackEntity(entityData.m_IndexInArchetype);
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

	}

	void Container::SetEntityActive(const EntityID& entity, const bool& isActive)
	{
		if (m_EntityManager->SetEntityActive(entity, isActive) && m_bInvokeEntityActivationStateListeners)
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
		bool isTheSameContainer = prefabContainer == this;
		EntityData prefabEntityData = prefabContainer->m_EntityManager->GetEntityData(prefab.ID());

		Archetype* prefabArchetype = prefabEntityData.m_Archetype;

		EntityID entityID = m_EntityManager->CreateEntity(isActive);
		Entity spawnedEntity(entityID, this);

		if (prefabArchetype == nullptr)
		{
			AddToEmptyEntities(*spawnedEntity.m_EntityData);
			InvokeEntityCreationObservers(spawnedEntity);
			return spawnedEntity;
		}

		uint64_t componentsCount = prefabArchetype->GetComponentsCount();
		PrefabSpawnData& spawnData = m_SpawnDataPerSpawn.EmplaceBack();
		spawnData.m_ComponentContexts.reserve(componentsCount);

		PreapareSpawnData(spawnData, prefabEntityData, prefabContainer);

		CreateEntityFromSpawnData(
			spawnedEntity,
			spawnData,
			componentsCount,
			prefabEntityData,
			prefabContainer
		);

		InvokeEntityCreationObservers(spawnedEntity);
		for (uint64_t i = 0; i < componentsCount; i++)
		{
			/*auto& componentContext = spawnData.ComponentContexts[i];
			componentContext.ComponentContext->InvokeOnCreateComponent_S(
				componentContext.ComponentData.Component,
				spawnedEntity
			);*/
		}

		m_SpawnDataPerSpawn.PopBack();
		return Entity();
	}

	bool Container::Spawn(const Entity& prefab, const uint64_t& spawnCount, const bool& areActive)
	{
		if (!m_CanSpawn || spawnCount == 0 || prefab.IsNull()) return false;

		/*Container* prefabContainer = prefab.GetContainer();
		bool isTheSameContainer = prefabContainer == this;
		EntityData prefabEntityData = prefabContainer->m_EntityManager->GetEntityData(prefab.ID());

		Archetype* prefabArchetype = prefabEntityData.m_Archetype;
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

		uint64_t componentsCount = prefabArchetype->GetComponentsCount();
		PrefabSpawnData& spawnData = m_SpawnDataPerSpawn.EmplaceBack();
		spawnData.Reserve(componentsCount);

		PreapareSpawnData(spawnData, prefabEntityData, prefabContainer);

		for (uint64_t entityIdx = 0; entityIdx < spawnCount; entityIdx++)
		{
			EntityID entityID = m_EntityManager->CreateEntity(areActive);
			spawnedEntity.Set(
				entityID,
				this
			);

			CreateEntityFromSpawnData(
				spawnedEntity,
				spawnData,
				componentsCount,
				prefabEntityData,
				prefabContainer
			);

			InvokeEntityCreationObservers(spawnedEntity);

			for (uint64_t i = 0; i < componentsCount; i++)
			{
				auto& componentContext = spawnData.ComponentContexts[i];
				componentContext.ComponentContext->InvokeOnCreateComponent_S(
					componentContext.ComponentData.Component,
					spawnedEntity
				);
			}
		}

		m_SpawnDataPerSpawn.PopBack();*/

		return true;
	}

	bool Container::Spawn(const Entity& prefab, std::vector<Entity>& spawnedEntities, const uint64_t& spawnCount, const bool& areActive)
	{
		if (!m_CanSpawn || spawnCount == 0 || prefab.IsNull()) return true;

		/*Container* prefabContainer = prefab.GetContainer();
		bool isTheSameContainer = prefabContainer == this;
		EntityData prefabEntityData = prefabContainer->m_EntityManager->GetEntityData(prefab.ID());

		Archetype* prefabArchetype = prefabEntityData.m_Archetype;
		if (prefabArchetype == nullptr)
		{
			for (uint64_t i = 0; i < spawnCount; i++)
			{
				EntityID entityId = m_EntityManager->CreateEntity(areActive);
				Entity& spawnedEntity = spawnedEntities.emplace_back(entityId, this);
				AddToEmptyEntities(*spawnedEntity.m_EntityData);
				InvokeEntityCreationObservers(spawnedEntity);
			}
			return true;
		}

		uint64_t componentsCount = prefabArchetype->GetComponentsCount();
		PrefabSpawnData& spawnData = m_SpawnDataPerSpawn.EmplaceBack();
		spawnData.Reserve(componentsCount);

		PreapareSpawnData(spawnData, prefabEntityData, prefabContainer);

		for (uint64_t i = 0; i < spawnCount; i++)
		{
			EntityID id = m_EntityManager->CreateEntity(areActive);
			Entity& spawnedEntity = spawnedEntities.emplace_back(id, this);
			CreateEntityFromSpawnData(
				spawnedEntity,
				spawnData,
				componentsCount,
				prefabEntityData,
				prefabContainer
			);

			InvokeEntityCreationObservers(spawnedEntity);
			for (uint64_t i = 0; i < componentsCount; i++)
			{
				auto& componentContext = spawnData.ComponentContexts[i];
				componentContext.ComponentContext->InvokeOnCreateComponent_S(
					componentContext.ComponentData.Component,
					spawnedEntity
				);
			}
		}

		m_SpawnDataPerSpawn.PopBack();*/

		return true;
	}

	void Container::PreapareSpawnData(
		PrefabSpawnData& spawnData,
		EntityData& prefabEntityData,
		Container* prefabContainer
	)
	{
		Archetype& prefabArchetype = *prefabEntityData.m_Archetype;
		uint64_t componentsCount = prefabArchetype.GetComponentsCount();

		for (uint64_t i = 0; i < componentsCount; i++)
		{
			TypeID typeID = prefabArchetype.ComponentsTypes()[i];


		}
	}

	void Container::CreateEntityFromSpawnData(
		Entity& entity,
		PrefabSpawnData& spawnData,
		const uint64_t componentsCount,
		const EntityData& prefabEntityData,
		Container* prefabContainer
	)
	{

	}

	bool Container::RemoveComponent(Entity& entity, const TypeID& componentTypeID)
	{
		if (!m_CanRemoveComponents || entity.m_Container != this) return false;

		EntityData& entityData = *entity.m_EntityData;
		if (entityData.m_Archetype == nullptr || !entityData.IsValidToPerformComponentOperation()) return false;

		uint64_t compIdxInArch = entityData.m_Archetype->FindTypeIndex(componentTypeID);
		if (compIdxInArch == std::numeric_limits<uint64_t>::max()) return false;

		ComponentContextBase* componentContext = entityData.m_Archetype->m_ComponentContexts[compIdxInArch];
		auto& container = entityData.m_Archetype->m_PackedContainers[compIdxInArch];

		/*if (m_PerformDelayedDestruction)
		{
			if (!firstRemovedCompData.m_DelayedToDestroy)
			{
				firstRemovedCompData.m_DelayedToDestroy = true;
				AddComponentToDelayedDestroy(entity.ID(), componentTypeID);
				return true;
			}
			return false;
		}*/

		{
			componentContext->InvokeOnDestroyComponent_S(
				container->GetComponentAsVoid(entityData.m_IndexInArchetype),
				entity
			);
		}

		Archetype* newEntityArchetype = this->m_ArchetypesMap.GetArchetypeAfterRemoveComponent(
			*entityData.m_Archetype,
			componentTypeID
		);

		uint64_t oldArchetypeIndex = entityData.m_IndexInArchetype;
		Archetype* oldArchetype = entityData.m_Archetype;
		if (newEntityArchetype != nullptr)
		{
			newEntityArchetype->AddEntityData(entityData.m_ID, entityData.m_IsActive);
			newEntityArchetype->MoveEntityAfterRemoveComponent(
				componentTypeID,
				*entityData.m_Archetype,
				entityData.m_IndexInArchetype
			);

			entityData.m_Archetype = newEntityArchetype;
			entityData.m_IndexInArchetype = newEntityArchetype->EntitiesCount();
			newEntityArchetype->m_EntitiesCount += 1;
		}
		else
		{
			entityData.m_Archetype = nullptr;
			entityData.m_IndexInArchetype = m_EmptyEntities.Size();
			m_EmptyEntities.EmplaceBack(&entityData);
		}

		auto result = oldArchetype->RemoveSwapBackEntity(oldArchetypeIndex);
		ValidateEntityInArchetype(result);

		return true;
	}

	bool Container::RemoveComponent(const EntityID& e, const TypeID& componentTypeID)
	{
		Entity entity(e, this);
		return RemoveComponent(entity, componentTypeID);
	}

	void Container::InvokeOnCreateComponentFromEntityID(ComponentContextBase* componentContext, void* componentPtr, const EntityID& id)
	{
		Entity e(id, this);
		componentContext->InvokeOnCreateComponent_S(componentPtr, e);
	}

	bool Container::RemoveComponentWithoutInvokingListener(const EntityID& entityID, const TypeID& componentTypeID)
	{
		/*Entity entity = { entityID, this };
		EntityData& entityData = *entity.m_EntityData;
		if (entityData.m_Archetype == nullptr || !entityData.IsValidToPerformComponentOperation()) return false;

		uint64_t compIdxInArch = entityData.m_Archetype->FindTypeIndex(componentTypeID);
		if (compIdxInArch == std::numeric_limits<uint64_t>::max()) return false;

		ComponentContextBase* componentContext = m_ComponentContexts[componentTypeID];

		const auto& removedCompData = entityData.GetComponentRef(compIdxInArch);
		auto allocator = componentContext->GetAllocator();
		auto result = allocator->RemoveSwapBack(removedCompData.ChunkIndex, removedCompData.ElementIndex);

		if (result.IsValid())
		{
			uint64_t compTypeIndexInFixedEntityArchetype;
			EntityData& fixedEntity = m_EntityManager->GetEntityData(result.eID);
			if (entityData.m_Archetype == fixedEntity.m_Archetype)
			{
				compTypeIndexInFixedEntityArchetype = compIdxInArch;
			}
			else
			{
				compTypeIndexInFixedEntityArchetype = fixedEntity.m_Archetype->FindTypeIndex(componentTypeID);
			}

			UpdateEntityComponentAccesDataInArchetype(
				fixedEntity,
				result.ChunkIndex,
				result.ElementIndex,
				allocator->GetComponentAsVoid(result.ChunkIndex, result.ElementIndex),
				compTypeIndexInFixedEntityArchetype
			);
		}

		Archetype* newEntityArchetype = m_ArchetypesMap.GetArchetypeAfterRemoveComponent(*entityData.m_Archetype, componentTypeID);
		if (newEntityArchetype == nullptr)
		{
			RemoveEntityFromArchetype(*entityData.m_Archetype, entityData);
			AddToEmptyEntities(entityData);
		}
		else
		{
			AddEntityToArchetype(
				*newEntityArchetype,
				entityData,
				std::numeric_limits<uint64_t>::max(),
				std::numeric_limits<uint64_t>::max(),
				componentTypeID,
				nullptr,
				false,
				compIdxInArch
			);
		}*/

		return true;
	}

	void Container::DestroyEntitesInArchetypes(Archetype& archetype, const bool& invokeOnDestroyListeners)
	{
		auto& entitesData = archetype.m_EntitiesData;
		uint64_t archetypeComponentsCount = archetype.GetComponentsCount();
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
					void* compPtr = archetype.GetComponentVoidPtr(entityDataIdx, i);
					archetype.m_ComponentContexts[i]->InvokeOnDestroyComponent_S(compPtr, entity);
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
			return m_ComponentContexts->SetObserversManager(observersManager);
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
		/*auto& entitesData = archetype.m_EntitiesData;
		auto& componentRefs = archetype.m_ComponentsRefs;
		uint64_t archetypeComponentsCount = archetype.GetComponentsCount();
		uint64_t entityDataCount = archetype.m_EntitesCountToInitialize;
		Entity entity;

		for (int64_t entityDataIdx = entityDataCount - 1; entityDataIdx > -1; entityDataIdx--)
		{
			ArchetypeEntityData& archetypeEntityData = entitesData[entityDataIdx];
			entity.Set(archetypeEntityData.ID(), this);
			InvokeEntityCreationObservers(entity);

			uint64_t componentDataIndex = entityDataIdx * archetypeComponentsCount;
			for (uint64_t i = 0; i < archetypeComponentsCount; i++)
			{
				auto& compRef = componentRefs[componentDataIndex];
				auto compContext = archetype.m_ComponentContexts[i];
				compContext->InvokeOnCreateComponent_S(compRef.ComponentPointer, entity);
				componentDataIndex += 1;
			}
		}*/
	}

	void Container::InvokeArchetypeOnDestroyListeners(Archetype& archetype)
	{
		/*auto& entitesData = archetype.m_EntitiesData;
		auto& componentRefs = archetype.m_ComponentsRefs;
		uint64_t archetypeComponentsCount = archetype.EntitiesCount();
		uint64_t entityDataCount = entitesData.size();
		Entity entity;

		for (uint64_t entityDataIdx = 0; entityDataIdx < entityDataCount; entityDataIdx++)
		{
			ArchetypeEntityData& archetypeEntityData = entitesData[entityDataIdx];
			entity.Set(archetypeEntityData.ID(), this);
			InvokeEntityDeactivationObservers(entity);

			uint64_t componentDataIndex = entityDataIdx * archetypeComponentsCount;
			for (uint64_t i = 0; i < archetypeComponentsCount; i++)
			{
				auto& compRef = componentRefs[componentDataIndex];
				auto compContext = archetype.m_ComponentContexts[i];
				compContext->InvokeOnDestroyComponent_S(compRef.ComponentPointer, entity);
				componentDataIndex += 1;
			}
		}*/
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
			RemoveComponent(data.m_EntityID, data.m_TypeID);
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

	void Container::AddComponentToDelayedDestroy(const EntityID& entityID, const TypeID typeID)
	{
		m_DelayedComponentsToDestroy.insert({ entityID, typeID });
	}

}
