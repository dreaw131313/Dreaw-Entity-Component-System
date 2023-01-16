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
		const bool& invokeEntityActivationStateListeners
	) :
		m_HaveOwnEntityManager(true),
		m_EntityManager(new EntityManager(enititesChunkSize)),
		m_ComponentContainerChunkSize(componentContainerChunkSize),
		m_ContainerSizeType(componentContainerChunkSizeType),
		m_bInvokeEntityActivationStateListeners(invokeEntityActivationStateListeners)
	{
	}

	Container::Container(
		EntityManager* entityManager,
		const ChunkSizeType& componentContainerChunkSizeType,
		const uint64_t& componentContainerChunkSize,
		const bool& invokeEntityActivationStateListeners
	) :
		m_HaveOwnEntityManager(false),
		m_EntityManager(entityManager),
		m_ComponentContainerChunkSize(componentContainerChunkSize),
		m_ContainerSizeType(componentContainerChunkSizeType),
		m_bInvokeEntityActivationStateListeners(invokeEntityActivationStateListeners)
	{
	}

	Container::~Container()
	{
		DestroyComponentsContexts();
		if (m_HaveOwnEntityManager) delete m_EntityManager;
	}

	Entity Container::CreateEntity(const bool& isActive)
	{
		EntityID entityID = m_EntityManager->CreateEntity(isActive);
		Entity e(entityID, this);
		InvokeEntityCreationObservers(e);
		return e;
	}


	void Container::DestroyOwnedEntities(const bool& invokeOnDestroyListeners)
	{
		auto& archetypes = m_ArchetypesMap.m_Archetypes;

		uint64_t archetypesCount = archetypes.size();
		for (uint64_t archetypeIdx = 0; archetypeIdx < archetypesCount; archetypeIdx++)
		{
			Archetype& archetype = *archetypes[archetypeIdx];
			DestroyEntitesInArchetypes(archetype);
			archetype.Reset();
		}

		ClearComponentsContainers();
	}

	bool Container::DestroyEntity(Entity& entity)
	{
		if (entity.IsAlive() && entity.m_Container == this)
		{
			EntityID entityID = entity.ID();
			EntityData& entityData = *entity.m_EntityData;
			if (entityData.m_IsEntityInDestruction) return false;
			entityData.m_IsEntityInDestruction = true;

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

			entityData.m_IsEntityInDestruction = false;
			m_EntityManager->DestroyEntity(entityID);
			return true;
		}

		return false;
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
				data.m_IsEntityInDestruction = true;

				entity.Set(archetypeEntityData.ID(), this);

				InvokeEntityDestructionObservers(entity);

				for (uint64_t i = 0; i < archetypeComponentsCount; i++)
				{
					auto& compRef = archetype.m_ComponentsRefs[componentDataIndex];
					auto compContext = archetype.m_ComponentContexts[i];
					compContext->InvokeOnDestroyComponent_S(compRef.ComponentPointer, entity);
					componentDataIndex += 1;
				}

				data.m_IsEntityInDestruction = false;
				m_EntityManager->DestroyEntity(archetypeEntityData.ID());
			}
		}
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
		if (prefab.IsNull()) return Entity();

		Container* prefabContainer = prefab.GetContainer();
		bool isTheSameContainer = prefabContainer == this;
		EntityData prefabEntityData = prefabContainer->m_EntityManager->GetEntityData(prefab.ID());

		Archetype* prefabArchetype = prefabEntityData.m_CurrentArchetype;

		EntityID entityID = m_EntityManager->CreateEntity(isActive);
		Entity spawnedEntity(entityID, this);

		if (prefabArchetype == nullptr)
		{
			return spawnedEntity;
		}

		uint64_t componentsCount = prefabArchetype->GetComponentsCount();

		PreapareSpawnData(prefabArchetype);


		CreateEntityFromSpawnData(
			spawnedEntity,
			componentsCount,
			prefabEntityData,
			prefabContainer
		);

		InvokeEntityCreationObservers(spawnedEntity);
		for (uint64_t i = 0; i < componentsCount; i++)
		{
			auto& componentContext = m_SpawnData.ComponentContexts[i];
			m_SpawnData.ComponentContexts[i].ComponentContext->InvokeOnCreateComponent_S(
				componentContext.ComponentData.Component,
				spawnedEntity
			);
		}

		return spawnedEntity;
	}

	bool Container::Spawn(const Entity& prefab, const uint64_t& spawnCount, const bool& areActive)
	{
		if (spawnCount == 0) return true;
		if (prefab.IsNull()) return false;

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
				InvokeEntityCreationObservers(spawnedEntity);
			}
			return true;
		}

		uint64_t componentsCount = prefabArchetype->GetComponentsCount();
		PreapareSpawnData(prefabArchetype);

		for (uint64_t entityIdx = 0; entityIdx < spawnCount; entityIdx++)
		{
			EntityID entityID = m_EntityManager->CreateEntity(areActive);
			spawnedEntity.Set(
				entityID,
				this
			);

			CreateEntityFromSpawnData(
				spawnedEntity,
				componentsCount,
				prefabEntityData,
				prefabContainer
			);

			InvokeEntityCreationObservers(spawnedEntity);

			for (uint64_t i = 0; i < componentsCount; i++)
			{
				auto& componentContext = m_SpawnData.ComponentContexts[i];
				m_SpawnData.ComponentContexts[i].ComponentContext->InvokeOnCreateComponent_S(
					componentContext.ComponentData.Component,
					spawnedEntity
				);
			}
		}

		return true;
	}

	bool Container::Spawn(const Entity& prefab, std::vector<Entity>& spawnedEntities, const uint64_t& spawnCount, const bool& areActive)
	{
		if (prefab.IsNull()) return false;
		if (spawnCount == 0) return true;

		Container* prefabContainer = prefab.GetContainer();
		bool isTheSameContainer = prefabContainer == this;
		EntityData prefabEntityData = prefabContainer->m_EntityManager->GetEntityData(prefab.ID());

		Archetype* prefabArchetype = prefabEntityData.m_CurrentArchetype;
		if (prefabArchetype == nullptr)
		{
			for (uint64_t i = 0; i < spawnCount; i++)
			{
				Entity spawnedEntity = spawnedEntities.emplace_back(CreateEntity(areActive), this);
				InvokeEntityCreationObservers(spawnedEntity);
			}
			return true;
		}

		uint64_t componentsCount = prefabArchetype->GetComponentsCount();
		PreapareSpawnData(prefabArchetype);

		for (uint64_t i = 0; i < spawnCount; i++)
		{
			EntityID id = m_EntityManager->CreateEntity(areActive);
			Entity& spawnedEntity = spawnedEntities.emplace_back(id, this);
			CreateEntityFromSpawnData(
				spawnedEntity,
				componentsCount,
				prefabEntityData,
				prefabContainer
			);

			InvokeEntityCreationObservers(spawnedEntity);
			for (uint64_t i = 0; i < componentsCount; i++)
			{
				auto& componentContext = m_SpawnData.ComponentContexts[i];
				m_SpawnData.ComponentContexts[i].ComponentContext->InvokeOnCreateComponent_S(
					componentContext.ComponentData.Component,
					spawnedEntity
				);
			}

		}

		return true;
	}

	void Container::PreapareSpawnData(Archetype* prefabArchetype)
	{
		uint64_t componentsCount = prefabArchetype->GetComponentsCount();
		m_SpawnData.Clear();
		m_SpawnData.Reserve(componentsCount);

		for (uint64_t i = 0; i < componentsCount; i++)
		{
			TypeID typeID = prefabArchetype->ComponentsTypes()[i];
			auto& context = m_ComponentContexts[typeID];

			if (context == nullptr)
			{
				context = prefabArchetype->m_ComponentContexts[i]->CreateOwnEmptyCopy(m_ObserversManager);
			}

			m_SpawnData.ComponentContexts.emplace_back(
				typeID,
				prefabArchetype->m_ComponentContexts[i],
				context
			);

			if (i == 0)
			{
				m_SpawnData.m_SpawnArchetype = m_ArchetypesMap.GetSingleComponentArchetype(context, typeID);
			}
			else
			{
				m_SpawnData.m_SpawnArchetype = m_ArchetypesMap.GetArchetypeAfterAddComponent(
					*m_SpawnData.m_SpawnArchetype,
					typeID,
					context
				);
			}
		}
	}

	void Container::CreateEntityFromSpawnData(
		Entity& entity,
		const uint64_t componentsCount,
		const EntityData& prefabEntityData,
		Container* prefabContainer
	)
	{
		for (uint64_t i = 0; i < componentsCount; i++)
		{
			auto& contextData = m_SpawnData.ComponentContexts[i];
			auto& prefabComponentData = prefabContainer->GetComponentRefFromArchetype(prefabEntityData, i);

			m_SpawnData.ComponentContexts[i].ComponentData = contextData.ComponentContext->GetAllocator()->CreateCopy(
				contextData.PrefabComponentContext->GetAllocator(),
				entity.ID(),
				prefabComponentData.ChunkIndex,
				prefabComponentData.ElementIndex
			);

		}

		AddSpawnedEntityToArchetype(entity.ID(), m_SpawnData.m_SpawnArchetype);

	}

	bool Container::RemoveComponent(Entity& entity, const TypeID& componentTypeID)
	{
		if (!entity.IsAlive() && entity.m_Container != this) return false;

		EntityID entityID = entity.ID();
		EntityData& entityData = m_EntityManager->GetEntityData(entityID);
		if (entityData.m_IsEntityInDestruction || entityData.m_CurrentArchetype == nullptr) return false;

		auto compIdxInArch = entityData.m_CurrentArchetype->m_TypeIDsIndexes.find(componentTypeID);
		if (compIdxInArch == entityData.m_CurrentArchetype->m_TypeIDsIndexes.end())return false;

		auto compContextIt = m_ComponentContexts.find(componentTypeID);
		if (compContextIt == m_ComponentContexts.end()) return false;
		ComponentContextBase* componentContext = compContextIt->second;

		auto removedCompData = GetComponentRefFromArchetype(entityData, compIdxInArch->second);
		{
			componentContext->InvokeOnDestroyComponent_S(
				removedCompData.ComponentPointer,
				entity
			);
		}
		removedCompData = GetComponentRefFromArchetype(entityData, compIdxInArch->second);

		auto allocator = componentContext->GetAllocator();
		auto result = allocator->RemoveSwapBack(removedCompData.ChunkIndex, removedCompData.ElementIndex);

		if (result.IsValid())
		{
			uint64_t compTypeIndexInFixedEntityArchetype;
			EntityData& fixedEntity = m_EntityManager->GetEntityData(result.eID);
			if (entityData.m_CurrentArchetype == fixedEntity.m_CurrentArchetype)
			{
				compTypeIndexInFixedEntityArchetype = compIdxInArch->second;
			}
			else
			{
				//compTypeIndexInFixedEntityArchetype = fixedEntity.m_CurrentArchetype->m_TypeIDsIndexes[componentTypeID];
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
				false
			);
		}

		return true;
	}

	void Container::AddEntityToArchetype(
		Archetype& newArchetype,
		EntityData& entityData,
		const uint64_t& newCompChunkIndex,
		const uint64_t& newCompElementIndex,
		const TypeID& compTypeID,
		void* newCompPtr,
		const bool& bIsNewComponentAdded
	)
	{
		Archetype& oldArchetype = *entityData.m_CurrentArchetype;
		uint64_t firstCompIndexInOldArchetype = entityData.m_IndexInArchetype * oldArchetype.GetComponentsCount();
		newArchetype.m_EntitiesData.emplace_back(oldArchetype.m_EntitiesData[entityData.m_IndexInArchetype]);

		for (uint64_t compIdx = 0; compIdx < newArchetype.GetComponentsCount(); compIdx++)
		{
			TypeID currentCompID = newArchetype.ComponentsTypes()[compIdx];

			if (bIsNewComponentAdded && currentCompID == compTypeID)
			{
				newArchetype.m_ComponentsRefs.emplace_back(newCompChunkIndex, newCompElementIndex, newCompPtr);
			}
			else
			{
				//uint64_t typeIndexInOldArchetype = oldArchetype.m_TypeIDsIndexes[currentCompID];
				uint64_t typeIndexInOldArchetype = oldArchetype.FindTypeIndex(currentCompID);
				uint64_t compDataIndexOldArchetype = firstCompIndexInOldArchetype + typeIndexInOldArchetype;
				newArchetype.m_ComponentsRefs.emplace_back(oldArchetype.m_ComponentsRefs[compDataIndexOldArchetype]);
			}
		}

		RemoveEntityFromArchetype(oldArchetype, entityData);

		entityData.m_CurrentArchetype = &newArchetype;
		entityData.m_IndexInArchetype = newArchetype.m_EntitiesCount;
		newArchetype.m_EntitiesCount += 1;
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

		uint64_t firstComponentPointer = entityData.m_IndexInArchetype * archetypeComponentCount;
		if (entityData.m_IndexInArchetype == lastEntityIndex)
		{
			// pop back last entity:

			archetype.m_EntitiesData.pop_back();

			archetype.m_ComponentsRefs.erase(archetype.m_ComponentsRefs.begin() + firstComponentPointer, archetype.m_ComponentsRefs.end());
		}
		else
		{
			auto& archEntityData = archetype.m_EntitiesData.back();
			EntityData& lastEntityInArchetypeData = m_EntityManager->GetEntityData(archEntityData.ID());
			uint64_t lastEntityFirstComponentIndex = lastEntityInArchetypeData.m_IndexInArchetype * archetypeComponentCount;

			lastEntityInArchetypeData.m_IndexInArchetype = entityData.m_IndexInArchetype;

			for (uint64_t i = 0; i < archetypeComponentCount; i++)
			{
				archetype.m_ComponentsRefs[firstComponentPointer + i] = archetype.m_ComponentsRefs[lastEntityFirstComponentIndex + i];
			}

			archetype.m_EntitiesData[entityData.m_IndexInArchetype] = archEntityData;
			archetype.m_EntitiesData.pop_back();

			archetype.m_ComponentsRefs.erase(archetype.m_ComponentsRefs.begin() + lastEntityFirstComponentIndex, archetype.m_ComponentsRefs.end());
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
		const EntityID& entityID,
		Archetype* spawnArchetype
	)
	{
		EntityData& data = m_EntityManager->GetEntityData(entityID);
		data.m_CurrentArchetype = spawnArchetype;
		data.m_IndexInArchetype = spawnArchetype->EntitiesCount();

		spawnArchetype->m_EntitiesData.emplace_back(entityID, data.m_IsActive);
		spawnArchetype->m_EntitiesCount += 1;

		// adding component:
		uint64_t componentsCount = m_SpawnData.ComponentContexts.size();

		for (int compIdx = 0; compIdx < componentsCount; compIdx++)
		{
			auto& contextData = m_SpawnData.ComponentContexts[compIdx];

			spawnArchetype->m_ComponentsRefs.emplace_back(
				contextData.ComponentData.ChunkIndex,
				contextData.ComponentData.ElementIndex,
				contextData.ComponentData.Component
			);
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
		auto& archetypes = m_ArchetypesMap.m_Archetypes;
		uint64_t archetypesCount = archetypes.size();
		for (uint64_t archetypeIdx = 0; archetypeIdx < archetypesCount; archetypeIdx++)
		{
			Archetype& archetype = *archetypes[archetypeIdx];
			InvokeArchetypeOnCreateListeners(archetype);
		}
	}

	void Container::InvokeEntitesOnDestroyListeners()
	{
		auto& archetypes = m_ArchetypesMap.m_Archetypes;
		uint64_t archetypesCount = archetypes.size();
		for (uint64_t archetypeIdx = 0; archetypeIdx < archetypesCount; archetypeIdx++)
		{
			Archetype& archetype = *archetypes[archetypeIdx];
			InvokeArchetypeOnDestroyListeners(archetype);
		}
	}

	void Container::InvokeArchetypeOnCreateListeners(Archetype& archetype)
	{
		auto& entitesData = archetype.m_EntitiesData;
		uint64_t archetypeComponentsCount = archetype.GetComponentsCount();
		uint64_t entityDataCount = entitesData.size();
		uint64_t componentDataIndex = 0;

		Entity entity;

		for (uint64_t entityDataIdx = 0; entityDataIdx < entityDataCount; entityDataIdx++)
		{
			ArchetypeEntityData& archetypeEntityData = entitesData[entityDataIdx];
			entity.Set(archetypeEntityData.ID(), this);
			InvokeEntityCreationObservers(entity);

			for (uint64_t i = 0; i < archetypeComponentsCount; i++)
			{
				auto& compRef = archetype.m_ComponentsRefs[componentDataIndex];
				auto compContext = archetype.m_ComponentContexts[i];
				compContext->InvokeOnCreateComponent_S(compRef.ComponentPointer, entity);
				componentDataIndex += 1;
			}
		}
	}

	void Container::InvokeArchetypeOnDestroyListeners(Archetype& archetype)
	{
		auto& entitesData = archetype.m_EntitiesData;
		uint64_t archetypeComponentsCount = archetype.GetComponentsCount();
		uint64_t entityDataCount = entitesData.size();
		uint64_t componentDataIndex = 0;

		Entity entity;

		for (uint64_t entityDataIdx = 0; entityDataIdx < entityDataCount; entityDataIdx++)
		{
			ArchetypeEntityData& archetypeEntityData = entitesData[entityDataIdx];
			entity.Set(archetypeEntityData.ID(), this);
			InvokeEntityDestructionObservers(entity);

			for (uint64_t i = 0; i < archetypeComponentsCount; i++)
			{
				auto& compRef = archetype.m_ComponentsRefs[componentDataIndex];
				auto compContext = archetype.m_ComponentContexts[i];
				compContext->InvokeOnDestroyComponent_S(compRef.ComponentPointer, entity);
				componentDataIndex += 1;
			}
		}
	}

}
