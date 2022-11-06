#pragma once
#include "decspch.h"

#include "Core.h"
#include "EntityData.h"
#include "Archetype.h"



namespace decs
{
	class EntityManager
	{
		friend class Container;
	public:
		EntityManager();

		EntityManager(const uint64_t& initialEntitiesCapacity);

		~EntityManager();

		EntityID CreateEntity(const bool& isActive = true);

		bool DestroyEntity(const EntityID& entity);

		inline bool IsEntityAlive(const EntityID& entity) const
		{
			if (entity >= m_enitiesDataCount) return false;
			return m_EntityData[entity].IsAlive;
		}

		inline void SetEntityActive(const EntityID& entity, const bool& isActive)
		{
			auto& data = m_EntityData[entity];
			if (data.IsActive != isActive)
			{
				data.IsActive = isActive;
				auto& archEntityData = data.CurrentArchetype->m_EntitiesData[data.IndexInArchetype];
				archEntityData.IsActive = isActive;
			}
		}

		inline bool IsEntityActive(const EntityID& entity)
		{
			if (entity >= m_enitiesDataCount) return false;
			return m_EntityData[entity].IsActive;
		}

		inline uint32_t GetEntityVersion(const EntityID& entity) const
		{
			if (IsEntityAlive(entity))
				return m_EntityData[entity].Version;

			return std::numeric_limits<uint32_t>::max();
		}

		inline uint32_t GetComponentsCount(const EntityID& entity) const
		{
			auto& data = m_EntityData[entity];
			if (data.IsAlive && data.CurrentArchetype != nullptr)
			{
				return data.CurrentArchetype->GetComponentsCount();
			}
			return 0;
		}

	private:
		std::vector<EntityData> m_EntityData;
		EntityID m_CreatedEntitiesCount = 0;
		uint64_t m_enitiesDataCount = 0;

		std::vector<uint64_t> m_FreeEntities;
		uint64_t m_FreeEntitiesCount = 0;

	private:
		inline EntityData& GetEntityData(const EntityID& entity) { return m_EntityData[entity]; }
	};
}