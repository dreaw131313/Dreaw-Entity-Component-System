#pragma once
#include "Core.h"
#include "EntityData.h"
#include "Archetypes/Archetype.h"
#include "Containers/TChunkedVector.h"

namespace decs
{
	class EntityManager
	{
		template<typename ...Types>
		friend class Query;
	public:
		EntityManager();

		EntityManager(uint64_t initialEntitiesCapacity);

		uint64_t GetCreatedEntitiesCount() const { return m_CreatedEntitiesCount; }
		uint64_t GetEntitiesDataCount() const { return m_EntityData.Size(); }
		uint64_t GetFreeEntitiesCount() const { return m_FreeEntitiesCount; }

		EntityData* CreateEntity(bool isActive = true);

		bool DestroyEntityInternal(EntityID entity);

		bool DestroyEntityInternal(EntityData& entityData);

		inline EntityData& GetEntityData(EntityID entity)
		{
			return m_EntityData[entity];
		}

		inline const EntityData& GetEntityData(EntityID entity) const { return m_EntityData[entity]; }

		void CreateReservedEntityData(uint32_t entitesToReserve, std::vector<EntityData*>& reservedEntityData);

		void CreateEntityFromReservedEntityData(EntityData* entityData);

		void ReturnReservedEntityData(std::vector<EntityData*> entityDatasToReturn, uint64_t entitytDataCount);

	private:
		TChunkedVector<EntityData> m_EntityData = { 1000 };
		std::vector<uint64_t> m_FreeEntities;

		EntityID m_CreatedEntitiesCount = 0;
		uint64_t m_FreeEntitiesCount = 0;
	};
}