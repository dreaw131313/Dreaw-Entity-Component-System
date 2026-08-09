#pragma once
#include "decs/Core/Core.h"
#include "decs/Core/TChunkedVector.h"

#include "Archetypes/Archetype.h"
#include "EntityData.h"

namespace decs
{
	struct EntityManager
	{
		friend class Container;
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

		inline size_t GetCapacity() const noexcept
		{
			return m_EntityDatas.Capacity();
		}

		inline size_t GetEntityDataChunkSize() const noexcept
		{
			return m_EntityDatas.ChunkCapacity();
		}

		EntityData* CreateEntity(bool isActive);

		bool DestroyEntity(EntityData* entityData);

		inline EntityData* GetEntityData(EntityID id) const noexcept
		{
			if (id < m_LookupTable.size())
			{
				return m_LookupTable[id];
			}
			return nullptr;
		}

	private:
		TChunkedVector<EntityData> m_EntityDatas{};
		ecsVector<EntityData*> m_LookupTable{};
		ecsVector<EntityData*> m_FreeEntities{};
		uint32_t m_CreatedEntityCount = 0;
	};
}