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
			return m_EntityDataHandles.Size();
		}

		uint64_t GetFreeEntitiesCount() const
		{
			return m_FreeEntities.size();
		}

		EntityDataHandle CreateEntity(bool isActive, Container& container);

		bool DestroyEntity(const EntityDataHandle& entityDataHandle);

		void ForceDestroyEntity(const EntityDataHandle& entityDataHandle);

	private:
		TChunkedVector<EntityDataHandle> m_EntityDataHandles{};
		std::vector<EntityDataHandle> m_FreeEntities{};
	};
}