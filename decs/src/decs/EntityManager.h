#pragma once
#include "Core.h"
#include "EntityData.h"
#include "Archetypes/Archetype.h"
#include "Containers/TChunkedVector.h"

namespace decs
{
	class EntityManager
	{
	public:
		EntityManager();

		EntityManager(uint64_t initialEntitiesCapacity);

		uint64_t GetEntitiesDataCount() const
		{
			return m_EntityData.Size();
		}

		uint64_t GetFreeEntitiesCount() const
		{
			return m_FreeEntities.size();
		}

		EntityData* CreateEntity(bool isActive = true);

		bool DestroyEntity(EntityData& entityData);

		void ForceDestroyEntity(EntityData& entityData);

		void CreateReservedEntityData(uint32_t entitesToReserve, std::vector<EntityData*>& reservedEntityData);

		void ReturnReservedEntityData(std::vector<EntityData*> reservedEntityData);

	private:
		TChunkedVector<EntityData> m_EntityData = { 1000 };
		std::vector<EntityData*> m_FreeEntities;
	};
}