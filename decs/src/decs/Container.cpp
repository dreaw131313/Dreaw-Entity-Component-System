#pragma once
#include "decspch.h"

#include "Container.h"
#include "Entity.h"

namespace decs
{
	Container::Container()
	{
	}

	Container::Container(
		const uint64_t& initialEntitiesCapacity,
		const ContainerBucketSizeType& componentContainerBucketSizeType,
		const uint64_t& componentContainerBucketSize
	) :
		m_ComponentContainerBucketSize(componentContainerBucketSize),
		m_ContainerSizeType(componentContainerBucketSizeType),
		m_EntityManager(initialEntitiesCapacity)
	{
	}

	Container::~Container()
	{
		DestroyComponentsContexts();
	}

	Entity Container::CreateEntity(const bool& isActive)
	{
		EntityID id = m_EntityManager.CreateEntity(isActive);
		InvokeEntityCreationObservers(id);

		return Entity(id, this);
	}

	bool Container::DestroyEntity(const EntityID& entity)
	{
		if (IsEntityAlive(entity))
		{
			EntityData& entityData = m_EntityManager.GetEntityData(entity);

			InvokeEntityDestructionObservers(entity);

			Archetype* currentArchetype = entityData.CurrentArchetype;
			if (currentArchetype != nullptr)
			{
				// Invoke On destroy metyhods
				uint64_t firstComponentDataIndexInArch = currentArchetype->GetComponentsCount() * entityData.IndexInArchetype;

				for (uint64_t i = 0; i < currentArchetype->GetComponentsCount(); i++)
				{
					auto& compRef = currentArchetype->m_ComponentsRefs[firstComponentDataIndexInArch + i];
					auto compContext = currentArchetype->m_ComponentContexts[i];

					compContext->InvokeOnDestroyComponent_S(compRef.ComponentPointer, entity, *this);
				}

				// Remove entity from component bucket:

				for (uint64_t i = 0; i < currentArchetype->GetComponentsCount(); i++)
				{
					auto& compRef = currentArchetype->m_ComponentsRefs[firstComponentDataIndexInArch + i];

					auto allocator = currentArchetype->m_ComponentContexts[i]->GetAllocator();

					auto result = allocator->RemoveSwapBack(compRef.BucketIndex, compRef.ElementIndex);

					if (result.IsValid())
					{
						uint64_t lastEntityIDInContainer = result.eID;
						UpdateEntityComponentAccesDataInArchetype(
							lastEntityIDInContainer,
							result.BucketIndex,
							result.ElementIndex,
							allocator->GetComponentAsVoid(result.BucketIndex, result.ElementIndex),
							i
						);
					}
				}

				RemoveEntityFromArchetype(*entityData.CurrentArchetype, entityData);
			}

			m_EntityManager.DestroyEntity(entity);
			return true;
		}

		return false;
	}

	Entity Container::Spawn(const Entity& prefab, const bool& isActive)
	{
		if (!prefab.IsValid()) return Entity();

		Container* prefabContainer = prefab.GetContainer();
		bool isTheSameContainer = prefabContainer == this;
		EntityData prefabEntityData = prefabContainer->m_EntityManager.GetEntityData(prefab.ID());

		Archetype* prefabArchetype = prefabEntityData.CurrentArchetype;
		if (prefabArchetype == nullptr)
		{
			return CreateEntity(isActive);
		}

		uint64_t componentsCount = prefabArchetype->GetComponentsCount();

		if (!PreapareSpawnData(componentsCount, prefabArchetype))
		{
			return Entity();
		}

		EntityID spawnedEntityID = CreateEntityFromSpawnData(
			isActive,
			componentsCount,
			prefabEntityData,
			prefabArchetype,
			prefabContainer
		);

		InvokeEntityCreationObservers(spawnedEntityID);
		for (uint64_t i = 0; i < componentsCount; i++)
		{
			auto& componentContext = m_SpawnData.ComponentContexts[i];
			m_SpawnData.ComponentContexts[i].ComponentContext->InvokeOnCreateComponent_S(
				componentContext.ComponentData.Component,
				spawnedEntityID,
				*this
			);
		}

		return Entity(spawnedEntityID, this);
	}

	bool Container::Spawn(const Entity& prefab, const uint64_t& spawnCount, const bool& areActive)
	{
		if (spawnCount == 0) return true;

		if (!prefab.IsValid()) return Entity();

		Container* prefabContainer = prefab.GetContainer();
		bool isTheSameContainer = prefabContainer == this;
		EntityData prefabEntityData = prefabContainer->m_EntityManager.GetEntityData(prefab.ID());

		Archetype* prefabArchetype = prefabEntityData.CurrentArchetype;
		if (prefabArchetype == nullptr)
		{
			for (uint64_t i = 0; i < spawnCount; i++)
			{
				CreateEntity(areActive);
			}
			return true;
		}

		uint64_t componentsCount = prefabArchetype->GetComponentsCount();
		if (!PreapareSpawnData(componentsCount, prefabArchetype))
		{
			return false;
		}

		for (uint64_t i = 0; i < spawnCount; i++)
		{
			EntityID spawnedEntityID = CreateEntityFromSpawnData(
				areActive,
				componentsCount,
				prefabEntityData,
				prefabArchetype,
				prefabContainer
			);

			InvokeEntityCreationObservers(spawnedEntityID);
			for (uint64_t i = 0; i < componentsCount; i++)
			{
				auto& componentContext = m_SpawnData.ComponentContexts[i];
				m_SpawnData.ComponentContexts[i].ComponentContext->InvokeOnCreateComponent_S(
					componentContext.ComponentData.Component,
					spawnedEntityID,
					*this
				);
			}
		}

		return true;
	}

	bool Container::Spawn(const Entity& prefab, std::vector<Entity>& spawnedEntities, const uint64_t& spawnCount, const bool& areActive)
	{
		if (spawnCount == 0) return true;

		if (!prefab.IsValid()) return Entity();

		Container* prefabContainer = prefab.GetContainer();
		bool isTheSameContainer = prefabContainer == this;
		EntityData prefabEntityData = prefabContainer->m_EntityManager.GetEntityData(prefab.ID());

		Archetype* prefabArchetype = prefabEntityData.CurrentArchetype;
		if (prefabArchetype == nullptr)
		{
			for (uint64_t i = 0; i < spawnCount; i++)
			{
				spawnedEntities.emplace_back(CreateEntity(areActive));
			}
			return true;
		}

		uint64_t componentsCount = prefabArchetype->GetComponentsCount();
		if (!PreapareSpawnData(componentsCount, prefabArchetype))
		{
			return false;
		}

		for (uint64_t i = 0; i < spawnCount; i++)
		{
			EntityID spawnedEntityID = CreateEntityFromSpawnData(
				areActive,
				componentsCount,
				prefabEntityData,
				prefabArchetype,
				prefabContainer
			);

			InvokeEntityCreationObservers(spawnedEntityID);
			for (uint64_t i = 0; i < componentsCount; i++)
			{
				auto& componentContext = m_SpawnData.ComponentContexts[i];
				m_SpawnData.ComponentContexts[i].ComponentContext->InvokeOnCreateComponent_S(
					componentContext.ComponentData.Component,
					spawnedEntityID,
					*this
				);
			}

			spawnedEntities.emplace_back(spawnedEntityID, this);
		}

		return true;
	}

	bool Container::PreapareSpawnData(
		const uint64_t& componentsCount,
		Archetype* prefabArchetype
	)
	{
		m_SpawnData.Clear();
		m_SpawnData.Reserve(componentsCount);

		auto endIt = m_ComponentContexts.end();
		for (uint64_t i = 0; i < componentsCount; i++)
		{
			TypeID typeID = prefabArchetype->ComponentsTypes()[i];
			auto it = m_ComponentContexts.find(typeID);
			if (it == endIt)
			{
				return false;
			}
			else
			{
				m_SpawnData.ComponentContexts.emplace_back(
					typeID,
					prefabArchetype->m_ComponentContexts[i],
					it->second
				);
			}
		}

		return true;
	}

	bool Container::RemoveComponent(const EntityID& e, const TypeID& componentTypeID)
	{
		if (!IsEntityAlive(e)) return false;

		EntityData& entityData = m_EntityManager.GetEntityData(e);
		if (entityData.CurrentArchetype == nullptr) return false;

		auto compIdxInArch = entityData.CurrentArchetype->m_TypeIDsIndexes.find(componentTypeID);
		if (compIdxInArch == entityData.CurrentArchetype->m_TypeIDsIndexes.end())return false;

		auto compContextIt = m_ComponentContexts.find(componentTypeID);
		if (compContextIt == m_ComponentContexts.end()) return false;
		ComponentContextBase* componentContext = compContextIt->second;


		auto& removedCompData = GetEntityComponentBucketElementIndex(entityData, compIdxInArch->second);

		componentContext->InvokeOnDestroyComponent_S(
			removedCompData.ComponentPointer,
			e,
			*this
		);

		auto allocator = componentContext->GetAllocator();
		auto result = allocator->RemoveSwapBack(removedCompData.BucketIndex, removedCompData.ElementIndex);

		if (result.IsValid())
		{
			uint64_t lastEntityIDInContainer = result.eID;
			UpdateEntityComponentAccesDataInArchetype(
				lastEntityIDInContainer,
				result.BucketIndex,
				result.ElementIndex,
				allocator->GetComponentAsVoid(result.BucketIndex, result.ElementIndex),
				compIdxInArch->second
			);
		}

		Archetype* newEntityArchetype = m_ArchetypesMap.GetArchetypeAfterRemoveComponent(*entityData.CurrentArchetype, componentTypeID);

		if (newEntityArchetype == nullptr)
		{
			RemoveEntityFromArchetype(*entityData.CurrentArchetype, entityData);
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
		const uint64_t& newCompBucketIndex,
		const uint64_t& newCompElementIndex,
		const TypeID& compTypeID,
		void* newCompPtr,
		const bool& bIsNewComponentAdded
	)
	{
		Archetype& oldArchetype = *entityData.CurrentArchetype;
		uint64_t firstCompIndexInOldArchetype = entityData.IndexInArchetype * oldArchetype.GetComponentsCount();
		newArchetype.m_EntitiesData.emplace_back(oldArchetype.m_EntitiesData[entityData.IndexInArchetype]);

		for (uint64_t compIdx = 0; compIdx < newArchetype.GetComponentsCount(); compIdx++)
		{
			TypeID currentCompID = newArchetype.ComponentsTypes()[compIdx];

			if (bIsNewComponentAdded && currentCompID == compTypeID)
			{
				newArchetype.m_ComponentsRefs.emplace_back(newCompBucketIndex, newCompElementIndex, newCompPtr);
			}
			else
			{
				uint64_t typeIndexInOldArchetype = oldArchetype.m_TypeIDsIndexes[newArchetype.ComponentsTypes()[compIdx]];
				uint64_t compDataIndexOldArchetype = firstCompIndexInOldArchetype + typeIndexInOldArchetype;
				newArchetype.m_ComponentsRefs.emplace_back(oldArchetype.m_ComponentsRefs[compDataIndexOldArchetype]);
			}
		}

		RemoveEntityFromArchetype(oldArchetype, entityData);

		entityData.CurrentArchetype = &newArchetype;
		entityData.IndexInArchetype = newArchetype.m_EntitiesCount;
		newArchetype.m_EntitiesCount += 1;
	}

	void Container::AddEntityToSingleComponentArchetype(
		Archetype& newArchetype,
		EntityData& entityData,
		const uint64_t& newCompBucketIndex,
		const uint64_t& newCompElementIndex,
		void* componentPointer
	)
	{
		newArchetype.m_ComponentsRefs.emplace_back(newCompBucketIndex, newCompElementIndex, componentPointer);
		newArchetype.m_EntitiesData.emplace_back(entityData.ID, entityData.IsActive);

		entityData.CurrentArchetype = &newArchetype;
		entityData.IndexInArchetype = newArchetype.EntitiesCount();
		newArchetype.m_EntitiesCount += 1;
	}


	void Container::RemoveEntityFromArchetype(Archetype& archetype, EntityData& entityData)
	{
		if (&archetype != entityData.CurrentArchetype) return;

		uint64_t lastEntityIndex = archetype.m_EntitiesCount - 1;
		const uint64_t archetypeComponentCount = archetype.GetComponentsCount();

		uint64_t firstComponentPointer = entityData.IndexInArchetype * archetypeComponentCount;
		if (entityData.IndexInArchetype == lastEntityIndex)
		{
			// pop back last entity:

			archetype.m_EntitiesData.pop_back();

			archetype.m_ComponentsRefs.erase(archetype.m_ComponentsRefs.begin() + firstComponentPointer, archetype.m_ComponentsRefs.end());
		}
		else
		{
			auto archEntityData = archetype.m_EntitiesData.back();
			EntityData& lastEntityInArchetypeData = m_EntityManager.GetEntityData(archEntityData.ID);
			uint64_t lastEntityFirstComponentIndex = lastEntityInArchetypeData.IndexInArchetype * archetypeComponentCount;

			lastEntityInArchetypeData.IndexInArchetype = entityData.IndexInArchetype;


			for (uint64_t i = 0; i < archetypeComponentCount; i++)
			{
				archetype.m_ComponentsRefs[firstComponentPointer + i] = archetype.m_ComponentsRefs[lastEntityFirstComponentIndex + i];
			}

			archetype.m_EntitiesData[entityData.IndexInArchetype] = archEntityData;
			archetype.m_EntitiesData.pop_back();

			archetype.m_ComponentsRefs.erase(archetype.m_ComponentsRefs.begin() + lastEntityFirstComponentIndex, archetype.m_ComponentsRefs.end());
		}

		entityData.CurrentArchetype = nullptr;
		archetype.m_EntitiesCount -= 1;
	}

	void Container::UpdateEntityComponentAccesDataInArchetype(
		const EntityID& entityID,
		const uint64_t& compBucketIndex,
		const uint64_t& compElementIndex,
		void* compPtr,
		const uint64_t& typeIndex
	)
	{
		EntityData& data = m_EntityManager.GetEntityData(entityID);

		uint64_t compDataIndex = data.IndexInArchetype * data.CurrentArchetype->GetComponentsCount() + typeIndex;

		auto& compData = data.CurrentArchetype->m_ComponentsRefs[compDataIndex];

		compData.BucketIndex = compBucketIndex;
		compData.ElementIndex = compElementIndex;
		compData.ComponentPointer = compPtr;
	}

	void Container::AddSpawnedEntityToArchetype(
		const EntityID& entityID,
		Archetype* toArchetype,
		Archetype* prefabArchetype
	)
	{
		EntityData& data = m_EntityManager.GetEntityData(entityID);
		data.CurrentArchetype = toArchetype;
		data.IndexInArchetype = toArchetype->EntitiesCount();

		toArchetype->m_EntitiesData.emplace_back(entityID, data.IsActive);
		toArchetype->m_EntitiesCount += 1;

		// adding component:
		uint64_t componentsCount = m_SpawnData.ComponentContexts.size();

		if (toArchetype == prefabArchetype)
		{
			for (int compIdx = 0; compIdx < componentsCount; compIdx++)
			{
				auto& contextData = m_SpawnData.ComponentContexts[compIdx];

				toArchetype->m_ComponentsRefs.emplace_back(
					contextData.ComponentData.BucketIndex,
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
					contextData.ComponentData.BucketIndex,
					contextData.ComponentData.ElementIndex,
					contextData.ComponentData.Component
				);
			}
		}
	}

	EntityID Container::CreateEntityFromSpawnData(
		const bool& isActive,
		const uint64_t componentsCount,
		const EntityData& prefabEntityData,
		Archetype* prefabArchetype,
		Container* prefabContainer
	)
	{
		EntityID spawnedEntityID = m_EntityManager.CreateEntity(isActive);
		Archetype* newEntityArchetype = nullptr;

		for (uint64_t i = 0; i < componentsCount; i++)
		{
			TypeID componentTypeID = prefabArchetype->ComponentsTypes()[i];
			auto contextData = m_SpawnData.ComponentContexts[i];
			auto prefabComponentData = prefabContainer->GetEntityComponentBucketElementIndex(prefabEntityData, i);

			m_SpawnData.ComponentContexts[i].ComponentData = contextData.ComponentContext->GetAllocator()->CreateCopy(
				contextData.PrefabComponentContext->GetAllocator(),
				spawnedEntityID,
				prefabComponentData.BucketIndex,
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
			AddSpawnedEntityToArchetype(spawnedEntityID, newEntityArchetype, prefabArchetype);
		}

		return spawnedEntityID;
	}

	bool Container::AddEntityCreationObserver(CreateEntityObserver* observer)
	{
		if (observer == nullptr) return false;
		m_EntityCreationObservers.push_back(observer);
		return true;
	}

	bool Container::RemoveEntityCreationObserver(CreateEntityObserver* observer)
	{
		for (uint32_t i = 0; i < m_EntityCreationObservers.size(); i++)
		{
			if (m_EntityCreationObservers[i] == observer)
			{
				auto it = m_EntityCreationObservers.begin();
				std::advance(it, i);
				m_EntityCreationObservers.erase(it);
				return true;
			}
		}
		return false;
	}

	bool Container::AddEntityDestructionObserver(DestroyEntityObserver* observer)
	{
		if (observer == nullptr) return false;
		m_EntittyDestructionObservers.push_back(observer);
		return true;
	}

	bool Container::RemoveEntityDestructionObserver(DestroyEntityObserver* observer)
	{
		for (uint32_t i = 0; i < m_EntittyDestructionObservers.size(); i++)
		{
			if (m_EntittyDestructionObservers[i] == observer)
			{
				auto it = m_EntittyDestructionObservers.begin();
				std::advance(it, i);
				m_EntittyDestructionObservers.erase(it);
				return true;
			}
		}
		return false;
	}

}
