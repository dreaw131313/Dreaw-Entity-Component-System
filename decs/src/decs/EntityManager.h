#pragma once
#include "Core.h"
#include "EntityData.h"
#include "Archetypes\Archetype.h"
#include "Containers\ChunkedVector.h"

namespace decs
{
	class EntityManager
	{
		template<typename ...Types>
		friend class Query;
	public:
		EntityManager();

		EntityManager(const uint64_t& initialEntitiesCapacity);

		uint64_t GetCreatedEntitiesCount() const { return m_CreatedEntitiesCount; }
		uint64_t GetEntitiesDataCount() const { return m_EntityData.Size(); }
		uint64_t GetFreeEntitiesCount() const { return m_FreeEntitiesCount; }

		EntityID CreateEntity(const bool& isActive = true);

		bool DestroyEntity(const EntityID& entity);

		bool DestroyEntity(EntityData& entityData);

		inline bool IsEntityAlive(const EntityID& entity) const
		{
			if (entity >= m_EntityData.Size()) return false;
			return m_EntityData[entity].IsAlive();
		}

		/// <summary>
		/// 
		/// </summary>
		/// <param name="entity"></param>
		/// <param name="isActive"></param>
		/// <returns>true if isActive state was changed, else false.</returns>
		inline bool SetEntityActive(const EntityID& entity, const bool& isActive)
		{
			if (entity >= m_EntityData.Size()) return false;

			auto& data = m_EntityData[entity];
			if (!data.m_IsAlive) return false;

			if (data.m_IsActive != isActive)
			{
				data.m_IsActive = isActive;
				auto& archEntityData = data.m_Archetype->m_EntitiesData[data.m_IndexInArchetype];
				archEntityData.m_IsActive = isActive;
				return true;
			}
			return false;
		}

		inline bool IsEntityActive(const EntityID& entity) const
		{
			auto& data = m_EntityData[entity];
			if (entity >= m_EntityData.Size()) return false;
			if (!data.m_IsAlive) return false;
			return m_EntityData[entity].m_IsActive;
		}

		inline EntityVersion GetEntityVersion(const EntityID& entity) const
		{
			if (IsEntityAlive(entity))
				return m_EntityData[entity].m_Version;

			return std::numeric_limits<EntityVersion>::max();
		}

		inline uint32_t ComponentsCount(const EntityID& entity) const
		{
			auto& data = m_EntityData[entity];
			if (data.m_IsAlive && data.m_Archetype != nullptr)
			{
				return data.m_Archetype->ComponentsCount();
			}
			return 0;
		}

		inline EntityData& GetEntityData(const EntityID& entity)
		{
			return m_EntityData[entity];
		}

		inline const EntityData& GetConstEntityData(const EntityID& entity) const { return m_EntityData[entity]; }

	private:
		ChunkedVector<EntityData> m_EntityData = { 1000 };
		std::vector<uint64_t> m_FreeEntities;

		EntityID m_CreatedEntitiesCount = 0;
		uint64_t m_FreeEntitiesCount = 0;
	};
}