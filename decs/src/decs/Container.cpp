#pragma once
#include "Container.h"
#include "Entity.h"

namespace decs
{
	Container::Container() :
		m_HaveOwnEntityManager(true),
		m_EntityManager(new EntityManager(1000))
	{

	}

	Container::Container(
		const uint64_t& enititesChunkSize,
		const ChunkSizeType& componentContainerChunkSizeType,
		const uint64_t& componentContainerChunkSize,
		const bool& invokeEntityActivationStateListeners,
		const uint64_t m_EmptyEntitiesChunkSize
	) :
		m_HaveOwnEntityManager(true),
		m_EntityManager(new EntityManager(enititesChunkSize)),
		m_ComponentContainerChunkSize(componentContainerChunkSize),
		m_ContainerSizeType(componentContainerChunkSizeType),
		m_bInvokeEntityActivationStateListeners(invokeEntityActivationStateListeners),
		m_EmptyEntities(m_EmptyEntitiesChunkSize)
	{
	}

	Container::Container(
		EntityManager* entityManager,
		const ChunkSizeType& componentContainerChunkSizeType,
		const uint64_t& componentContainerChunkSize,
		const bool& invokeEntityActivationStateListeners,
		const uint64_t m_EmptyEntitiesChunkSize
	) :
		m_HaveOwnEntityManager(false),
		m_EntityManager(entityManager),
		m_ComponentContainerChunkSize(componentContainerChunkSize),
		m_ContainerSizeType(componentContainerChunkSizeType),
		m_bInvokeEntityActivationStateListeners(invokeEntityActivationStateListeners),
		m_EmptyEntities(m_EmptyEntitiesChunkSize)
	{
	}

	Container::~Container()
	{
		DestroyComponentsContexts();
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
		if (m_CanDestroyEntities && entity.IsAlive() && entity.m_Container == this)
		{
			EntityID entityID = entity.ID();
			EntityData& entityData = *entity.m_EntityData;
			if (entityData.IsInDestruction()) return false;

			if (m_PerformDelayedDestruction)
			{
				entityData.SetState(EntityDestructionState::DelayedToDestruction);
				AddEntityToDelayedDestroy(entity.ID());
				return true;
			}
			else
			{
				entityData.SetState(EntityDestructionState::InDestruction);
			}

			InvokeEntityDestructionObservers(entity);

			Archetype* currentArchetype = entityData.m_CurrentArchetype;
			if (currentArchetype != nullptr)
			{
				// Invoke On destroy metyhods
				uint64_t firstComponentDataIndexInArch = currentArchetype->GetComponentsCount() * entityData.m_IndexInArchetype;

				for (uint64_t i = 0; i < currentArchetype->GetComponentsCount(); i++)
				{
					auto& compRef = currentArchetype->m_ComponentsRefs[firstComponentDataIndexInArch + i];
					auto compContext = currentArchetype->m_ComponentContexts[i];
					compContext->InvokeOnDestroyComponent_S(compRef.ComponentPointer, entity);
				}

				// Remove entity from component bucket:
				for (uint64_t i = 0; i < currentArchetype->GetComponentsCount(); i++)
				{
					auto& compRef = currentArchetype->m_ComponentsRefs[firstComponentDataIndexInArch + i];
					auto allocator = currentArchetype->m_ComponentContexts[i]->GetAllocator();
					auto result = allocator->RemoveSwapBack(compRef.ChunkIndex, compRef.ElementIndex);

					if (result.IsValid())
					{
						EntityData& fixedEntityData = m_EntityManager->GetEntityData(result.eID);
						uint64_t compTypeIndexInFixedEntityArchetype;
						if (entityData.m_CurrentArchetype == fixedEntityData.m_CurrentArchetype)
						{
							compTypeIndexInFixedEntityArchetype = i;
						}
						else
						{
							compTypeIndexInFixedEntityArchetype = fixedEntityData.m_CurrentArchetype->FindTypeIndex(currentArchetype->m_TypeIDs[i]);
						}

						UpdateEntityComponentAccesDataInArchetype(
							fixedEntityData,
							result.ChunkIndex,
							result.ElementIndex,
							allocator->GetComponentAsVoid(result.ChunkIndex, result.ElementIndex),
							i
						);
					}
				}

				RemoveEntityFromArchetype(*entityData.m_CurrentArchetype, entityData);
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

		uint64_t archetypesCount = archetypes.size();
		for (uint64_t archetypeIdx = 0; archetypeIdx < archetypesCount; archetypeIdx++)
		{
			Archetype& archetype = *archetypes[archetypeIdx];
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

		ClearComponentsContainers();
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

		Archetype* prefabArchetype = prefabEntityData.m_CurrentArchetype;

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
		spawnData.Reserve(componentsCount);

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
			auto& componentContext = spawnData.ComponentContexts[i];
			componentContext.ComponentContext->InvokeOnCreateComponent_S(
				componentContext.ComponentData.Component,
				spawnedEntity
			);
		}

		m_SpawnDataPerSpawn.PopBack();
		return spawnedEntity;
	}

	bool Container::Spawn(const Entity& prefab, const uint64_t& spawnCount, const bool& areActive)
	{
		if (!m_CanSpawn || spawnCount == 0 || prefab.IsNull()) return false;

		Container* prefabContainer = prefab.GetContainer();
		bool isTheSameContainer = prefabContainer == this;
		EntityData prefabEntityData = prefabContainer->m_EntityManager->GetEntityData(prefab.ID());

		Archetype* prefabArchetype = prefabEntityData.m_CurrentArchetype;
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

		m_SpawnDataPerSpawn.PopBack();

		return true;
	}

	bool Container::Spawn(const Entity& prefab, std::vector<Entity>& spawnedEntities, const uint64_t& spawnCount, const bool& areActive)
	{
		if (!m_CanSpawn || spawnCount == 0 || prefab.IsNull()) return true;

		Container* prefabContainer = prefab.GetContainer();
		bool isTheSameContainer = prefabContainer == this;
		EntityData prefabEntityData = prefabContainer->m_EntityManager->GetEntityData(prefab.ID());

		Archetype* prefabArchetype = prefabEntityData.m_CurrentArchetype;
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

		m_SpawnDataPerSpawn.PopBack();

		return true;
	}

	void Container::PreapareSpawnData(
		PrefabSpawnData& spawnData,
		EntityData& prefabEntityData,
		Container* prefabContainer
	)
	{
		Archetype* prefabArchetype = prefabEntityData.m_CurrentArchetype;
		uint64_t componentsCount = prefabArchetype->GetComponentsCount();

		for (uint64_t i = 0; i < componentsCount; i++)
		{
			TypeID typeID = prefabArchetype->ComponentsTypes()[i];
			auto& context = m_ComponentContexts[typeID];
			auto prefabComponentContext = prefabArchetype->m_ComponentContexts[i];

			if (context == nullptr)
			{
				context = prefabComponentContext->CreateOwnEmptyCopy(m_ObserversManager);
			}

			auto& spawnComponentContext = spawnData.ComponentContexts.emplace_back(
				typeID,
				prefabComponentContext,
				context
			);

			auto& prefabComponentData = prefabEntityData.GetComponentRef(i);
			spawnComponentContext.CopiedComponentPointer = prefabComponentData.ComponentPointer;

			if (i == 0)
			{
				spawnData.m_SpawnArchetype = m_ArchetypesMap.GetSingleComponentArchetype(context, typeID);
			}
			else
			{
				spawnData.m_SpawnArchetype = m_ArchetypesMap.GetArchetypeAfterAddComponent(
					*spawnData.m_SpawnArchetype,
					typeID,
					context
				);
			}
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
		for (uint64_t i = 0; i < componentsCount; i++)
		{
			auto& contextData = spawnData.ComponentContexts[i];
			contextData.ComponentData = contextData.ComponentContext->GetAllocator()->CreateCopy(
				entity.ID(),
				contextData.CopiedComponentPointer
			);
		}

		AddSpawnedEntityToArchetype(spawnData, *entity.m_EntityData, spawnData.m_SpawnArchetype);
	}

	bool Container::RemoveComponent(Entity& entity, const TypeID& componentTypeID)
	{
		if (!m_CanRemoveComponents || entity.m_Container != this) return false;

		EntityData& entityData = *entity.m_EntityData;
		if (!entityData.IsValidToPerformComponentOperation() || entityData.m_CurrentArchetype == nullptr) return false;

		uint64_t compIdxInArch = entityData.m_CurrentArchetype->FindTypeIndex(componentTypeID);
		if (compIdxInArch == std::numeric_limits<uint64_t>::max())return false;

		ComponentContextBase* componentContext = m_ComponentContexts[componentTypeID];

		auto& firstRemovedCompData = entityData.GetComponentRef(compIdxInArch);
		if (m_PerformDelayedDestruction)
		{
			firstRemovedCompData.m_DelayedToDestroy = true;
			AddComponentToDelayedDestroy(entity.ID(), componentTypeID);
			return true;
		}

		{
			componentContext->InvokeOnDestroyComponent_S(
				firstRemovedCompData.ComponentPointer,
				entity
			);
		}
		const auto& removedCompData = entityData.GetComponentRef(compIdxInArch);

		auto allocator = componentContext->GetAllocator();
		auto result = allocator->RemoveSwapBack(removedCompData.ChunkIndex, removedCompData.ElementIndex);

		if (result.IsValid())
		{
			uint64_t compTypeIndexInFixedEntityArchetype;
			EntityData& fixedEntity = m_EntityManager->GetEntityData(result.eID);
			if (entityData.m_CurrentArchetype == fixedEntity.m_CurrentArchetype)
			{
				compTypeIndexInFixedEntityArchetype = compIdxInArch;
			}
			else
			{
				compTypeIndexInFixedEntityArchetype = fixedEntity.m_CurrentArchetype->FindTypeIndex(componentTypeID);
			}

			UpdateEntityComponentAccesDataInArchetype(
				fixedEntity,
				result.ChunkIndex,
				result.ElementIndex,
				allocator->GetComponentAsVoid(result.ChunkIndex, result.ElementIndex),
				compTypeIndexInFixedEntityArchetype
			);
		}

		Archetype* newEntityArchetype = m_ArchetypesMap.GetArchetypeAfterRemoveComponent(*entityData.m_CurrentArchetype, componentTypeID);
		if (newEntityArchetype == nullptr)
		{
			RemoveEntityFromArchetype(*entityData.m_CurrentArchetype, entityData);
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
		}

		return true;
	}

	bool Container::RemoveComponent(const EntityID& e, const TypeID& componentTypeID)
	{
		Entity entity(e, this);
		return RemoveComponent(e, componentTypeID);
	}

	void Container::ClearComponentsContainers()
	{
		for (auto& [key, context] : m_ComponentContexts)
		{
			context->GetAllocator()->Clear();
		}
	}

	void Container::InvokeOnCreateComponentFromEntityID(ComponentContextBase* componentContext, void* componentPtr, const EntityID& id)
	{
		Entity e(id, this);
		componentContext->InvokeOnCreateComponent_S(componentPtr, e);
	}

	void Container::AddEntityToArchetype(
		Archetype& newArchetype,
		EntityData& entityData,
		const uint64_t& newCompChunkIndex,
		const uint64_t& newCompElementIndex,
		const TypeID& compTypeID,
		void* newCompPtr,
		const bool& bIsNewComponentAdded,
		const uint64_t& removedComponentIndex
	)
	{
		Archetype& oldArchetype = *entityData.m_CurrentArchetype;

		uint64_t firstCompIndexInOldArchetype = entityData.m_IndexInArchetype * oldArchetype.GetComponentsCount();
		newArchetype.m_EntitiesData.emplace_back(oldArchetype.m_EntitiesData[entityData.m_IndexInArchetype]);

		const TypeID* componentTypes = newArchetype.ComponentsTypes();
		uint64_t componentsCount = newArchetype.GetComponentsCount();

		uint64_t oldArchetypeComponentIndex = 0;
		auto& oldComponentRefs = oldArchetype.m_ComponentsRefs;
		auto& newComponentRefs = newArchetype.m_ComponentsRefs;

		if (bIsNewComponentAdded)
		{
			for (uint64_t compIdx = 0; compIdx < componentsCount; compIdx++)
			{
				TypeID currentCompID = componentTypes[compIdx];

				if (currentCompID == compTypeID)
				{
					newArchetype.m_ComponentsRefs.emplace_back(newCompChunkIndex, newCompElementIndex, newCompPtr);
				}
				else
				{
					uint64_t compDataIndexOldArchetype = firstCompIndexInOldArchetype + oldArchetypeComponentIndex;
					newComponentRefs.emplace_back(oldComponentRefs[compDataIndexOldArchetype]);
					oldArchetypeComponentIndex += 1;
				}
			}
		}
		else
		{
			uint64_t compIdx = 0;
			for (; compIdx < removedComponentIndex; compIdx++)
			{
				uint64_t compDataIndexOldArchetype = firstCompIndexInOldArchetype + oldArchetypeComponentIndex;
				newComponentRefs.emplace_back(oldComponentRefs[compDataIndexOldArchetype]);
				oldArchetypeComponentIndex += 1;
			}
			oldArchetypeComponentIndex += 1;
			for (; compIdx < componentsCount; compIdx++)
			{
				uint64_t compDataIndexOldArchetype = firstCompIndexInOldArchetype + oldArchetypeComponentIndex;
				newComponentRefs.emplace_back(oldComponentRefs[compDataIndexOldArchetype]);
				oldArchetypeComponentIndex += 1;
			}
		}

		RemoveEntityFromArchetype(oldArchetype, entityData);

		entityData.m_CurrentArchetype = &newArchetype;
		entityData.m_IndexInArchetype = newArchetype.m_EntitiesCount;
		newArchetype.m_EntitiesCount += 1;
	}

	void Container::AddEntityToSingleComponentArchetype(
		Archetype& newArchetype,
		EntityData& entityData,
		const uint64_t& newCompChunkIndex,
		const uint64_t& newCompElementIndex,
		void* componentPointer
	)
	{
		newArchetype.m_ComponentsRefs.emplace_back(newCompChunkIndex, newCompElementIndex, componentPointer);
		newArchetype.m_EntitiesData.emplace_back(entityData.m_ID, entityData.m_IsActive);

		entityData.m_CurrentArchetype = &newArchetype;
		entityData.m_IndexInArchetype = newArchetype.EntitiesCount();
		newArchetype.m_EntitiesCount += 1;
	}


	void Container::RemoveEntityFromArchetype(Archetype& archetype, EntityData& entityData)
	{
		if (&archetype != entityData.m_CurrentArchetype) return;

		uint64_t lastEntityIndex = archetype.m_EntitiesCount - 1;
		const uint64_t archetypeComponentCount = archetype.GetComponentsCount();

		uint64_t firstComponentIndex = entityData.m_IndexInArchetype * archetypeComponentCount;
		if (entityData.m_IndexInArchetype == lastEntityIndex)
		{
			// pop back last entity:
			archetype.m_EntitiesData.pop_back();

			/*for (int i = 0; i < archetypeComponentCount; i++)
			{
				archetype.m_ComponentsRefs.pop_back();
			}*/

			archetype.m_ComponentsRefs.erase(archetype.m_ComponentsRefs.begin() + firstComponentIndex, archetype.m_ComponentsRefs.end());
		}
		else
		{
			auto& archEntityData = archetype.m_EntitiesData.back();
			EntityData& lastEntityInArchetypeData = m_EntityManager->GetEntityData(archEntityData.ID());
			uint64_t lastEntityFirstComponentIndex = lastEntityInArchetypeData.m_IndexInArchetype * archetypeComponentCount;

			lastEntityInArchetypeData.m_IndexInArchetype = entityData.m_IndexInArchetype;

			/*for (int i = archetypeComponentCount - 1; i > -1; i--)
			{
				archetype.m_ComponentsRefs[firstComponentIndex + i] = archetype.m_ComponentsRefs[lastEntityFirstComponentIndex + i];
				archetype.m_ComponentsRefs.pop_back();
			}*/

			for (uint64_t i = 0; i < archetypeComponentCount; i++)
			{
				archetype.m_ComponentsRefs[firstComponentIndex + i] = archetype.m_ComponentsRefs[lastEntityFirstComponentIndex + i];
			}
			archetype.m_ComponentsRefs.erase(archetype.m_ComponentsRefs.begin() + lastEntityFirstComponentIndex, archetype.m_ComponentsRefs.end());

			archetype.m_EntitiesData[entityData.m_IndexInArchetype] = archEntityData;
			archetype.m_EntitiesData.pop_back();
		}

		entityData.m_CurrentArchetype = nullptr;
		archetype.m_EntitiesCount -= 1;
	}

	void Container::UpdateEntityComponentAccesDataInArchetype(
		EntityData& data,
		const uint64_t& compChunkIndex,
		const uint64_t& compElementIndex,
		void* compPtr,
		const uint64_t& typeIndex
	)
	{
		uint64_t compDataIndex = data.m_IndexInArchetype * data.m_CurrentArchetype->GetComponentsCount() + typeIndex;

		auto& compData = data.m_CurrentArchetype->m_ComponentsRefs[compDataIndex];

		compData.ChunkIndex = compChunkIndex;
		compData.ElementIndex = compElementIndex;
		compData.ComponentPointer = compPtr;
	}

	void Container::AddSpawnedEntityToArchetype(
		PrefabSpawnData& spawnData,
		EntityData& data,
		Archetype* spawnArchetype
	)
	{
		data.m_CurrentArchetype = spawnArchetype;
		data.m_IndexInArchetype = spawnArchetype->EntitiesCount();

		spawnArchetype->m_EntitiesData.emplace_back(data.m_ID, data.m_IsActive);
		spawnArchetype->m_EntitiesCount += 1;

		// adding component:
		uint64_t componentsCount = spawnData.ComponentContexts.size();

		for (int compIdx = 0; compIdx < componentsCount; compIdx++)
		{
			auto& contextData = spawnData.ComponentContexts[compIdx];

			spawnArchetype->m_ComponentsRefs.emplace_back(
				contextData.ComponentData.ChunkIndex,
				contextData.ComponentData.ElementIndex,
				contextData.ComponentData.Component
			);
		}
	}

	void Container::DestroyEntitesInArchetypes(Archetype& archetype, const bool& invokeOnDestroyListeners)
	{
		auto& entitesData = archetype.m_EntitiesData;
		uint64_t archetypeComponentsCount = archetype.GetComponentsCount();
		uint64_t entityDataCount = entitesData.size();
		uint64_t componentDataIndex = 0;

		if (invokeOnDestroyListeners)
		{
			for (uint64_t entityDataIdx = 0; entityDataIdx < entityDataCount; entityDataIdx++)
			{
				ArchetypeEntityData& archetypeEntityData = entitesData[entityDataIdx];
				m_EntityManager->DestroyEntity(archetypeEntityData.ID());
			}
		}
		else
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
					auto& compRef = archetype.m_ComponentsRefs[componentDataIndex];
					auto compContext = archetype.m_ComponentContexts[i];
					compContext->InvokeOnDestroyComponent_S(compRef.ComponentPointer, entity);
					componentDataIndex += 1;
				}

				m_EntityManager->DestroyEntity(archetypeEntityData.ID());
			}
		}
	}

	bool Container::SetObserversManager(ObserversManager* observersManager)
	{
		if (observersManager == m_ObserversManager) return false;

		m_ObserversManager = observersManager;
		for (auto& [typeID, context] : m_ComponentContexts)
		{
			context->SetObserverManager(m_ObserversManager);
		}

		return true;
	}

	void Container::ReassignObservers()
	{
		if (m_ObserversManager == nullptr) return;

		for (auto& [typeID, context] : m_ComponentContexts)
		{
			context->SetObserverManager(m_ObserversManager);
		}
	}

	void Container::InvokeEntitesOnCreateListeners()
	{
		{
			BoolSwitch delayedDestroySwitch(m_PerformDelayedDestruction, true);

			auto& archetypes = m_ArchetypesMap.m_Archetypes;
			for (uint64_t i = 0; i < archetypes.size(); i++)
			{
				archetypes[i]->ValidateEntitiesCountToInitialize();
			}

			uint64_t archetypesCount = archetypes.size();
			for (uint64_t archetypeIdx = 0; archetypeIdx < archetypesCount; archetypeIdx++)
			{
				Archetype& archetype = *archetypes[archetypeIdx];
				InvokeArchetypeOnCreateListeners(archetype);
			}
		}

		Entity entity = {};
		uint64_t emptyEntitiesSize = m_EmptyEntities.Size();
		for (uint64_t i = 0; i < emptyEntitiesSize; i++)
		{
			EntityData* data = m_EmptyEntities[i];
			entity.Set(*data, this);
			InvokeEntityCreationObservers(entity);
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
		uint64_t archetypesCount = archetypes.size();
		for (uint64_t archetypeIdx = 0; archetypeIdx < archetypesCount; archetypeIdx++)
		{
			Archetype& archetype = *archetypes[archetypeIdx];
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

	void Container::InvokeEntitesOnCreateListeners_2()
	{
		BoolSwitch canDestroySwitch(m_CanDestroyEntities, false);
		BoolSwitch canRemoveComponentSwitch(m_CanRemoveComponents, false);

		for (auto& [key, value] : m_ComponentContexts)
		{
			value->m_ComponentsCountToIterate = value->GetAllocator()->ElementsCount();
		}

		Entity entity = {};
		for (uint64_t i = 0; i < m_EmptyEntities.Size(); i++)
		{
			EntityData& data = *m_EmptyEntities[i];
			entity.Set(data, this);
			InvokeEntityCreationObservers(entity);
		}

		for (auto& [key, value] : m_ComponentContexts)
		{
			InvokeComponentOnCreateListeners(value);
		}
	}

	void Container::InvokeEntitesOnDestroyListeners_2()
	{
	}

	void Container::InvokeArchetypeOnCreateListeners(Archetype& archetype)
	{
		auto& entitesData = archetype.m_EntitiesData;
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
		}
	}

	void Container::InvokeArchetypeOnDestroyListeners(Archetype& archetype)
	{
		auto& entitesData = archetype.m_EntitiesData;
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
		}
	}

	void Container::InvokeComponentOnCreateListeners(ComponentContextBase* coponentContext)
	{
		BaseComponentAllocator* container = coponentContext->GetAllocator();
		int64_t containerSize = std::min(coponentContext->m_ComponentsCountToIterate, container->ElementsCount());

		Entity entity = {};
		for (int64_t elementIndex = containerSize - 1; elementIndex > -1; elementIndex--)
		{
			std::pair<EntityID, void*> element = container->GetEntityAndComponentData(elementIndex);
			entity.Set(element.first, this);
			coponentContext->InvokeOnCreateComponent_S(element.second, entity);
		}
	}

	void Container::InvokeComponentOnDestroyListeners(ComponentContextBase* coponentContext)
	{
		BaseComponentAllocator* container = coponentContext->GetAllocator();
		int64_t containerSize = container->ElementsCount();

		Entity entity = {};
		for (int64_t elementIndex = containerSize - 1; elementIndex > -1; elementIndex--)
		{
			std::pair<EntityID, void*> element = container->GetEntityAndComponentData(elementIndex);
			entity.Set(element.first, this);
			coponentContext->InvokeOnDestroyComponent_S(element.second, entity);
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
