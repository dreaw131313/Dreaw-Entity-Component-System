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

		EntityManager(uint64_t entityDataHandleChunkSize);

		~EntityManager();

		inline uint64_t GetEntitiesDataCount() const
		{
			return m_EntityDataHandles.Size();
		}

		inline uint64_t GetFreeEntitiesCount() const
		{
			return m_FreeEntities.size();
		}

		inline uint32_t GetCreatedEntityCount() const
		{
			return m_CreatedEntityCount;
		}

		EntityDataHandle CreateEntity(bool isActive, Container& container);

		bool DestroyEntity(const EntityDataHandle& entityDataHandle);

		void ForceDestroyEntity(const EntityDataHandle& entityDataHandle);

		void MarkEntitiesDead()
		{
			m_LifeTimeData->m_bIsContainerAlive = false;
		}
	private:
		TChunkedVector<EntityDataHandle> m_EntityDataHandles{};
		std::vector<EntityDataHandle> m_FreeEntities{};

		EnityLifeTimeData* m_LifeTimeData = nullptr;

		uint32_t m_CreatedEntityCount = 0;

	private:
		void InitializeLifeTimeData();

		void DestroyLifeTimeData();
	};
}