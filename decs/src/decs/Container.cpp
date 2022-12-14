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

	/*Container::Container(const Container& other)
	{
		throw std::runtime_error("Container copy constructor not implemented");
	}

	Container::Container(Container&& other) noexcept
	{
		throw std::runtime_error("Container move constructor not implemented");
	}*/

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

	Entity& Container::CreateEntity(const bool& isActive)
	{
		Entity* entity = m_EntityManager->CreateEntity(this, isActive);
		InvokeEntityCreationObservers(*entity);
		return *entity;
	}

	bool Container::DestroyEntity(const EntityID& entityID)
	{
		if (IsEntityAlive(entityID))
		{
			EntityData& entityData = m_EntityManager->GetEntityData(entityID);
			InvokeEntityDestructionObservers(*entityData.m_EntityPtr);

			Archetype* currentArchetype = entityData.m_CurrentArchetype;
			if (currentArchetype != nullptr)
			{
				// Invoke On destroy metyhods
				uint64_t firstComponentDataIndexInArch = currentArchetype->GetComponentsCount() * entityData.m_IndexInArchetype;

				for (uint64_t i = 0; i < currentArchetype->GetComponentsCount(); i++)
				{
					auto& compRef = currentArchetype->m_ComponentsRefs[firstComponentDataIndexInArch + i];
					auto compContext = currentArchetype->m_ComponentContexts[i];
					compContext->InvokeOnDestroyComponent_S(compRef.ComponentPointer, *entityData.m_EntityPtr);
				}

				// Remove entity from component bucket:
				for (uint64_t i = 0; i < currentArchetype->GetComponentsCount(); i++)
				{
					auto& compRef = currentArchetype->m_ComponentsRefs[firstComponentDataIndexInArch + i];
					auto allocator = currentArchetype->m_ComponentContexts[i]->GetAllocator();
					auto result = allocator->RemoveSwapBack(compRef.ChunkIndex, compRef.ElementIndex);

					if (result.IsValid())
					{
						uint64_t lastEntityIDInContainer = result.eID;
						UpdateEntityComponentAccesDataInArchetype(
							lastEntityIDInContainer,
							result.ChunkIndex,
							result.ElementIndex,
							allocator->GetComponentAsVoid(result.ChunkIndex, result.ElementIndex),
							i
						);
					}
				}

				RemoveEntityFromArchetype(*entityData.m_CurrentArchetype, entityData);
			}

			m_EntityManager->DestroyEntity(entityID);
			return true;
		}

		return false;
	}

	bool Container::DestroyEntity(Entity& e)
	{
		if (e.m_Container != this) return false;
		return DestroyEntity(e.m_ID);
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
			for (uint64_t entityDataIdx = 0; entityDataIdx < entityDataCount; entityDataIdx++)
			{
				ArchetypeEntityData& archetypeEntityData = entitesData[entityDataIdx];
				InvokeEntityDestructionObservers(*archetypeEntityData.EntityPtr());

				for (uint64_t i = 0; i < archetypeComponentsCount; i++)
				{
					auto& compRef = archetype.m_ComponentsRefs[componentDataIndex];
					auto compContext = archetype.m_ComponentContexts[i];
					compContext->InvokeOnDestroyComponent_S(compRef.ComponentPointer, *archetypeEntityData.EntityPtr());
					componentDataIndex += 1;
				}

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

	Entity* Container::Spawn(const Entity& prefab, const bool& isActive)
	{
		if (prefab.IsNull()) return nullptr;

		Container* prefabContainer = prefab.GetContainer();
		bool isTheSameContainer = prefabContainer == this;
		EntityData prefabEntityData = prefabContainer->m_EntityManager->GetEntityData(prefab.ID());

		Archetype* prefabArchetype = prefabEntityData.m_CurrentArchetype;
		if (prefabArchetype == nullptr)
		{
			return &CreateEntity(isActive);
		}

		uint64_t componentsCount = prefabArchetype->GetComponentsCount();

		PreapareSpawnData(componentsCount, prefabArchetype);

		Entity* spawnedEntity = CreateEntityFromSpawnData(
			isActive,
			componentsCount,
			prefabEntityData,
			prefabArchetype,
			prefabContainer
		);

		InvokeEntityCreationObservers(*spawnedEntity);
		for (uint64_t i = 0; i < componentsCount; i++)
		{
			auto& componentContext = m_SpawnData.ComponentContexts[i];
			m_SpawnData.ComponentContexts[i].ComponentContext->InvokeOnCreateComponent_S(
				componentContext.ComponentData.Component,
				*spawnedEntity
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
		if (prefabArchetype == nullptr)
		{
			for (uint64_t i = 0; i < spawnCount; i++)
			{
				CreateEntity(areActive);
			}
			return true;
		}

		uint64_t componentsCount = prefabArchetype->GetComponentsCount();
		PreapareSpawnData(componentsCount, prefabArchetype);

		for (uint64_t i = 0; i < spawnCount; i++)
		{
			Entity* spawnedEntity = CreateEntityFromSpawnData(
				areActive,
				componentsCount,
				prefabEntityData,
				prefabArchetype,
				prefabContainer
			);

			InvokeEntityCreationObservers(*spawnedEntity);
			for (uint64_t i = 0; i < componentsCount; i++)
			{
				auto& componentContext = m_SpawnData.ComponentContexts[i];
				m_SpawnData.ComponentContexts[i].ComponentContext->InvokeOnCreateComponent_S(
					componentContext.ComponentData.Component,
					*spawnedEntity
				);
			}
		}

		return true;
	}

	bool Container::Spawn(const Entity& prefab, std::vector<Entity*>& spawnedEntities, const uint64_t& spawnCount, const bool& areActive)
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
				spawnedEntities.emplace_back(&CreateEntity(areActive));
			}
			return true;
		}

		uint64_t componentsCount = prefabArchetype->GetComponentsCount();
		PreapareSpawnData(componentsCount, prefabArchetype);

		for (uint64_t i = 0; i < spawnCount; i++)
		{
			Entity* spawnedEntity = CreateEntityFromSpawnData(
				areActive,
				componentsCount,
				prefabEntityData,
				prefabArchetype,
				prefabContainer
			);

			InvokeEntityCreationObservers(*spawnedEntity);
			for (uint64_t i = 0; i < componentsCount; i++)
			{
				auto& componentContext = m_SpawnData.ComponentContexts[i];
				m_SpawnData.ComponentContexts[i].ComponentContext->InvokeOnCreateComponent_S(
					componentContext.ComponentData.Component,
					*spawnedEntity
				);
			}

			spawnedEntities.emplace_back(spawnedEntity);
		}

		return true;
	}

	void Container::PreapareSpawnData(const uint64_t& componentsCount, Archetype* prefabArchetype)
	{
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
		}
	}

	bool Container::RemoveComponent(const EntityID& e, const TypeID& componentTypeID)
	{
		if (!IsEntityAlive(e)) return false;

		EntityData& entityData = m_EntityManager->GetEntityData(e);
		if (entityData.m_CurrentArchetype == nullptr) return false;

		auto compIdxInArch = entityData.m_CurrentArchetype->m_TypeIDsIndexes.find(componentTypeID);
		if (compIdxInArch == entityData.m_CurrentArchetype->m_TypeIDsIndexes.end())return false;

		auto compContextIt = m_ComponentContexts.find(componentTypeID);
		if (compContextIt == m_ComponentContexts.end()) return false;
		ComponentContextBase* componentContext = compContextIt->second;


		auto& removedCompData = GetEntityComponentChunkElementIndex(entityData, compIdxInArch->second);

		componentContext->InvokeOnDestroyComponent_S(
			removedCompData.ComponentPointer,
			*entityData.m_EntityPtr
		);

		auto allocator = componentContext->GetAllocator();
		auto result = allocator->RemoveSwapBack(removedCompData.ChunkIndex, removedCompData.ElementIndex);

		if (result.IsValid())
		{
			uint64_t lastEntityIDInContainer = result.eID;
			UpdateEntityComponentAccesDataInArchetype(
				lastEntityIDInContainer,
				result.ChunkIndex,
				result.ElementIndex,
				allocator->GetComponentAsVoid(result.ChunkIndex, result.ElementIndex),
				compIdxInArch->second
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
				uint64_t typeIndexInOldArchetype = oldArchetype.m_TypeIDsIndexes[newArchetype.ComponentsTypes()[compIdx]];
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

	void Container::AddEntityToSingleComponentArchetype(
		Archetype& newArchetype,
		EntityData& entityData,
		const uint64_t& newCompChunkIndex,
		const uint64_t& newCompElementIndex,
		void* componentPointer
	)
	{
		newArchetype.m_ComponentsRefs.emplace_back(newCompChunkIndex, newCompElementIndex, componentPointer);
		newArchetype.m_EntitiesData.emplace_back(entityData.m_ID, entityData.m_IsActive, entityData.m_EntityPtr);

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
		const EntityID& entityID,
		const uint64_t& compChunkIndex,
		const uint64_t& compElementIndex,
		void* compPtr,
		const uint64_t& typeIndex
	)
	{
		EntityData& data = m_EntityManager->GetEntityData(entityID);

		uint64_t compDataIndex = data.m_IndexInArchetype * data.m_CurrentArchetype->GetComponentsCount() + typeIndex;

		auto& compData = data.m_CurrentArchetype->m_ComponentsRefs[compDataIndex];

		compData.ChunkIndex = compChunkIndex;
		compData.ElementIndex = compElementIndex;
		compData.ComponentPointer = compPtr;
	}

	void Container::AddSpawnedEntityToArchetype(
		const EntityID& entityID,
		Archetype* toArchetype,
		Archetype* prefabArchetype
	)
	{
		EntityData& data = m_EntityManager->GetEntityData(entityID);
		data.m_CurrentArchetype = toArchetype;
		data.m_IndexInArchetype = toArchetype->EntitiesCount();

		toArchetype->m_EntitiesData.emplace_back(entityID, data.m_IsActive, data.m_EntityPtr);
		toArchetype->m_EntitiesCount += 1;

		// adding component:
		uint64_t componentsCount = m_SpawnData.ComponentContexts.size();

		if (toArchetype == prefabArchetype)
		{
			for (int compIdx = 0; compIdx < componentsCount; compIdx++)
			{
				auto& contextData = m_SpawnData.ComponentContexts[compIdx];

				toArchetype->m_ComponentsRefs.emplace_back(
					contextData.ComponentData.ChunkIndex,
					contextData.ComponentData.ElementIndex,
					contextData.ComponentData.Component
				);
			}
		}
		else
		{
			for (int compIdx = 0; compIdx < componentsCount; compIdx++)
			{
				TypeID componentTypeIdInArchetype = toArchetype->ComponentsTypes()[compIdx];
				uint64_t componentIndexInPrefabArchetype = prefabArchetype->m_TypeIDsIndexes[componentTypeIdInArchetype];

				auto& contextData = m_SpawnData.ComponentContexts[componentIndexInPrefabArchetype];

				toArchetype->m_ComponentsRefs.emplace_back(
					contextData.ComponentData.ChunkIndex,
					contextData.ComponentData.ElementIndex,
					contextData.ComponentData.Component
				);
			}
		}
	}

	Entity* Container::CreateEntityFromSpawnData(
		const bool& isActive,
		const uint64_t componentsCount,
		const EntityData& prefabEntityData,
		Archetype* prefabArchetype,
		Container* prefabContainer
	)
	{
		Entity* spawnedEntity = m_EntityManager->CreateEntity(this, isActive);
		Archetype* newEntityArchetype = nullptr;

		for (uint64_t i = 0; i < componentsCount; i++)
		{
			TypeID componentTypeID = prefabArchetype->ComponentsTypes()[i];
			auto& contextData = m_SpawnData.ComponentContexts[i];
			auto& prefabComponentData = prefabContainer->GetEntityComponentChunkElementIndex(prefabEntityData, i);

			m_SpawnData.ComponentContexts[i].ComponentData = contextData.ComponentContext->GetAllocator()->CreateCopy(
				contextData.PrefabComponentContext->GetAllocator(),
				spawnedEntity->ID(),
				prefabComponentData.ChunkIndex,
				prefabComponentData.ElementIndex
			);

			if (i == 0)
			{
				newEntityArchetype = m_ArchetypesMap.GetSingleComponentArchetype(contextData.ComponentContext, componentTypeID);
			}
			else
			{
				newEntityArchetype = m_ArchetypesMap.GetArchetypeAfterAddComponent(
					*newEntityArchetype,
					componentTypeID,
					contextData.ComponentContext
				);
			}
		}

		if (newEntityArchetype != nullptr)
		{
			AddSpawnedEntityToArchetype(spawnedEntity->ID(), newEntityArchetype, prefabArchetype);
		}

		return spawnedEntity;
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

		for (uint64_t entityDataIdx = 0; entityDataIdx < entityDataCount; entityDataIdx++)
		{
			ArchetypeEntityData& archetypeEntityData = entitesData[entityDataIdx];
			InvokeEntityCreationObservers(*archetypeEntityData.EntityPtr());

			for (uint64_t i = 0; i < archetypeComponentsCount; i++)
			{
				auto& compRef = archetype.m_ComponentsRefs[componentDataIndex];
				auto compContext = archetype.m_ComponentContexts[i];
				compContext->InvokeOnCreateComponent_S(compRef.ComponentPointer, *archetypeEntityData.EntityPtr());
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

		for (uint64_t entityDataIdx = 0; entityDataIdx < entityDataCount; entityDataIdx++)
		{
			ArchetypeEntityData& archetypeEntityData = entitesData[entityDataIdx];
			InvokeEntityDestructionObservers(*archetypeEntityData.EntityPtr());

			for (uint64_t i = 0; i < archetypeComponentsCount; i++)
			{
				auto& compRef = archetype.m_ComponentsRefs[componentDataIndex];
				auto compContext = archetype.m_ComponentContexts[i];
				compContext->InvokeOnDestroyComponent_S(compRef.ComponentPointer, *archetypeEntityData.EntityPtr());
				componentDataIndex += 1;
			}
		}
	}

}
