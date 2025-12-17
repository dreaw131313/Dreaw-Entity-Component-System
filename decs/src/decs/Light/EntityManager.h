#pragma once
#include "decs/Core.h"
#include "decs/Containers/TChunkedVector.h"

#include "EntityData.h"
#include "Archetypes/Archetype.h"

namespace decs
{
	class EntityManager
	{
	public:
		EntityManager();

		EntityManager(uint64_t entityDataHandleChunkSize);

		~EntityManager();

		inline uint64_t GetEntitiesDataCount() const
		{
			return m_EntityDatas.Size();
		}

		inline uint64_t GetFreeEntitiesCount() const
		{
			return m_FreeEntities.size();
		}

		inline uint32_t GetCreatedEntityCount() const
		{
			return m_CreatedEntityCount;
		}

		EntityData* CreateEntity(Container& container);

		bool DestroyEntity(EntityData* entityData);

		void ForceDestroyEntity(EntityData* entityData);

	private:
		TChunkedVector<EntityData> m_EntityDatas{};
		std::vector<EntityData*> m_FreeEntities{};

		uint32_t m_CreatedEntityCount = 0;

	private:
	};
}