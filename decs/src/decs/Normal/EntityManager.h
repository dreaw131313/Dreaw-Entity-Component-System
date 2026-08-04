#pragma once
#include "decs/Core/Core.h"
#include "decs/Core/TChunkedVector.h"

#include "Archetypes/Archetype.h"
#include "EntityData.h"

namespace decs
{
	struct EntityManager
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

		EntityData* CreateEntity(bool isActive, Container& container);

		bool DestroyEntity(EntityData* entityData);

		void ForceDestroyEntity(EntityData* entityData);

		inline const EntityData* GetEntityData(EntityID id) const
		{
			if (id < m_EntityDatas.Size())
			{
				return &m_EntityDatas[id];
			}
			return nullptr;
		}

		inline EntityData* GetEntityData(EntityID id)
		{
			if (id < m_EntityDatas.Size())
			{
				return &m_EntityDatas[id];
			}
			return nullptr;
		}

	private:
		TChunkedVector<EntityData> m_EntityDatas{};
		ecsVector<EntityData*> m_FreeEntities{};

		uint32_t m_CreatedEntityCount = 0;

	private:
	};
}