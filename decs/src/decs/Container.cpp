#pragma once
#include "Container.h"
#include "Entity.h"

#include "decs/Iteration/ContainerIterator.h"

namespace decs
{
	Container::Container():
		m_EntityManager(m_DefaultEntitiesChunkSize)
	{
	}

	Container::Container(const ContainerConfig& config):
		m_EntityManager(config.EntityChunkSize),
		m_ArchetypesMap(config.ArchetypeChunkSize, 100)
	{
	}

	Container::~Container()
	{
		m_ArchetypesMap.ClearEntityDataAndComponents();
	}

	void Container::Clear()
	{
		ReturnOwnedEntitiesToEntityManager_Internal();
		m_EmptyEntities.clear();
		m_ArchetypesMap.ClearEntityDataAndComponents();
	}

	void Container::ReturnOwnedEntitiesToEntityManager_Internal()
	{
		ContainerIterator iterator = {};
		iterator.Foreach(*this, [this](const decs::Entity& entity)
		{
			m_EntityManager.ForceDestroyEntity(entity.m_EntityData);
		});
	}

	Entity Container::CreateEntity()
	{
		EntityData* entityData = m_EntityManager.CreateEntity(*this);
		AddToEmptyEntitiesRightAfterNewEntityCreation(*entityData);

		return Entity(*entityData);
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
		if (entity.GetContainer() == this)
		{
			EntityID entityID = entity.GetID();
			EntityData& entityData = *entity.GetEntityData();

			Archetype* currentArchetype = entityData.m_Archetype;

			if (currentArchetype != nullptr)
			{
				const uint32_t indexInArchetype = entityData.m_IndexInArchetype;
				currentArchetype->RemoveSwapBackEntity(indexInArchetype);
			}
			else
			{
				RemoveFromEmptyEntities(entityData);
			}

			m_EntityManager.DestroyEntity(entity.GetEntityData());

			return true;
		}

		return false;
	}

	EntityData* Container::GetEntityData(const Entity& entity) const
	{
		return entity.m_EntityData;
	}

	Entity Container::CreateEntityRaw()
	{
		return Entity(m_EntityManager.CreateEntity(*this));
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

	Entity Container::Spawn(const Entity& prefab)
	{
		if (prefab.IsNull()) return Entity();

		Container* prefabContainer = prefab.GetContainer();
		EntityData& prefabEntityData = *prefab.GetEntityData();
		Archetype* prefabArchetype = prefabEntityData.m_Archetype;

		Entity spawnedEntity(*m_EntityManager.CreateEntity(*this));
		EntityData* spawnedEntityData = spawnedEntity.GetEntityData();

		if (prefabArchetype == nullptr)
		{
			AddToEmptyEntitiesRightAfterNewEntityCreation(*spawnedEntityData);
			return spawnedEntity;
		}

		Archetype* spawnArchetype = GetArchetypeForSpawn(prefabEntityData);
		DECS_ASSERT(spawnArchetype != nullptr, "Archetype must not be nullptr!");
		CreateEntityFromSpawnData(prefabEntityData, *prefabArchetype, spawnedEntity, *spawnArchetype);

		return spawnedEntity;
	}

	bool Container::Spawn(const Entity& prefab, uint64_t spawnCount)
	{
		if (spawnCount == 0 || prefab.IsNull()) return false;

		Container* prefabContainer = prefab.GetContainer();
		EntityData& prefabEntityData = *prefab.GetEntityData();
		Archetype* prefabArchetype = prefabEntityData.m_Archetype;

		if (prefabArchetype == nullptr)
		{
			for (uint64_t i = 0; i < spawnCount; i++)
			{
				Entity spawnedEntity(*m_EntityManager.CreateEntity(*this));

				AddToEmptyEntitiesRightAfterNewEntityCreation(*spawnedEntity.GetEntityData());
			}
			return true;
		}

		Archetype* spawnArchetype = GetArchetypeForSpawn(prefabEntityData);
		DECS_ASSERT(spawnArchetype != nullptr, "Archetype must not be nullptr!");

		spawnArchetype->ReserveSpaceInArchetype(spawnArchetype->EntityCount() + spawnCount);

		for (uint64_t entityIdx = 0; entityIdx < spawnCount; entityIdx++)
		{
			Entity spawnedEntity(*m_EntityManager.CreateEntity(*this));
			CreateEntityFromSpawnData(prefabEntityData, *prefabArchetype, spawnedEntity, *spawnArchetype);
		}

		return true;
	}

	bool Container::Spawn(const Entity& prefab, std::vector<Entity>& spawnedEntities, uint64_t spawnCount)
	{
		if (spawnCount == 0 || prefab.IsNull()) return false;

		Container* prefabContainer = prefab.GetContainer();
		EntityData& prefabEntityData = *prefab.GetEntityData();
		Archetype* prefabArchetype = prefabEntityData.m_Archetype;

		spawnedEntities.reserve(spawnedEntities.size() + spawnCount);

		if (prefabArchetype == nullptr)
		{
			for (uint64_t i = 0; i < spawnCount; i++)
			{
				Entity& spawnedEntity = spawnedEntities.emplace_back(*m_EntityManager.CreateEntity(*this));
				AddToEmptyEntitiesRightAfterNewEntityCreation(*spawnedEntity.GetEntityData());
			}
			return true;
		}

		Archetype* spawnArchetype = GetArchetypeForSpawn(prefabEntityData);
		DECS_ASSERT(spawnArchetype != nullptr, "Archetype must not be nullptr!");

		spawnArchetype->ReserveSpaceInArchetype(spawnArchetype->EntityCount() + spawnCount);

		for (uint64_t entityIdx = 0; entityIdx < spawnCount; entityIdx++)
		{
			Entity& spawnedEntity = spawnedEntities.emplace_back(*m_EntityManager.CreateEntity(*this));
			CreateEntityFromSpawnData(prefabEntityData, *prefabArchetype, spawnedEntity, *spawnArchetype);
		}

		return true;
	}

	Archetype* Container::GetArchetypeForSpawn(const EntityData& prefabEntityData)
	{
		Container* prefabContainer = prefabEntityData.m_Container;
		Archetype* spawnArchetype = nullptr;
		if (prefabContainer == this)
		{
			spawnArchetype = prefabEntityData.m_Archetype;
		}
		else
		{
			spawnArchetype = m_ArchetypesMap.GetOrCreateMatchedArchetype(*prefabEntityData.m_Archetype);
		}
		return spawnArchetype;
	}

	void Container::CreateEntityFromSpawnData(
		const EntityData& prefabEntityData,
		const Archetype& prefabArchetype,
		const Entity& spawnedEntity,
		Archetype& spawnArchetype

	)
	{
		const uint32_t prefabIndexInArchetype = prefabEntityData.m_IndexInArchetype;

		auto entityData = spawnedEntity.GetEntityData();
		spawnArchetype.AddEntityData(spawnedEntity.GetEntityData());

		auto& spawnArchetypeTypeData = spawnArchetype.m_TypeData;
		auto& prefabArchetypeTypeData = prefabArchetype.m_TypeData;

		const uint64_t typeCount = prefabArchetype.GetTypeCount();

		for (uint32_t i = 0; i < typeCount; i++)
		{
			ArchetypeTypeData& currentSpawnTypeData = spawnArchetypeTypeData[i];
			const ArchetypeTypeData& currentPrefabTypeData = prefabArchetypeTypeData[i];

			if (currentSpawnTypeData.IsTag())
			{
				continue;
			}

			currentSpawnTypeData.m_PackedContainer->PushBack(currentPrefabTypeData.m_PackedContainer->GetComponentBasePtr(prefabIndexInArchetype));
		}
	}

	bool Container::RemoveComponent(const Entity& entity, TypeID componentTypeID)
	{
		if (entity.GetContainer() != this) return false;

		EntityData& entityData = *entity.GetEntityData();
		if (entityData.m_Archetype == nullptr) return false;

		uint32_t compIdxInArch = entityData.m_Archetype->FindTypeIndex(componentTypeID);
		if (compIdxInArch == std::numeric_limits<uint32_t>::max()) return false;

		Archetype* oldArchetype = entityData.m_Archetype;
		uint64_t indexInOldArchetype = entityData.m_IndexInArchetype;

		ArchetypeTypeData& oldArchetypeTypeData = oldArchetype->m_TypeData[compIdxInArch];
		if (oldArchetypeTypeData.IsTag())
		{
			return false;
		}

		Archetype* newArchetype = m_ArchetypesMap.GetArchetypeAfterRemoveComponent(
			*entityData.m_Archetype,
			componentTypeID
		);

		if (newArchetype != nullptr)
		{
			Archetype::MoveEntityAfterRemoveComponent(*oldArchetype, *newArchetype, indexInOldArchetype, componentTypeID);
		}
		else
		{
			oldArchetype->RemoveSwapBackEntity(indexInOldArchetype);
			AddToEmptyEntities(entityData);
		}

		return true;
	}

	bool Container::RemoveTag(EntityData& entityData, TypeID tagType)
	{
		if (entityData.m_Container != this || entityData.m_Archetype == nullptr)
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
			Archetype::MoveEntityAfterRemoveComponent(*oldArchetype, *newArchetype, entityIndexInOldArchetype, tagType);
		}
		else
		{
			oldArchetype->RemoveSwapBackEntity(entityIndexInOldArchetype);
			AddToEmptyEntities(entityData);
		}

		return true;
	}

}
