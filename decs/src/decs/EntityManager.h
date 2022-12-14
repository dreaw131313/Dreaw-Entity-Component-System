#pragma once
#include "decspch.h"

#include "Core.h"
#include "EntityData.h"
#include "Archetype.h"
#include "Containers\ChunkedVector.h"

namespace decs
{
	class Entity;
	class Container;

	class EntityManager
	{
		template<typename ...Types>
		friend class View;
	public:
		EntityManager();

		EntityManager(const uint64_t& initialEntitiesCapacity);

		~EntityManager();

		uint64_t GetCreatedEntitiesCount() const { return m_CreatedEntitiesCount; }
		uint64_t GetEntitiesDataCount() const { return m_enitiesDataCount; }
		uint64_t GetFreeEntitiesCount() const { return m_FreeEntitiesCount; }

		EntityID CreateEntity(const bool& isActive = true);

		/// <summary>
		/// 
		/// </summary>
		/// <param name="isActive"></param>
		/// <returns>True if it is new entity, false if it is recycled entity</returns>
		bool CreateEntity(EntityID& id, const bool& isActive = true);

		Entity* CreateEntity(Container*& forContainer, const bool& isActive = true);

		bool DestroyEntity(const EntityID& entity);

		inline bool IsEntityAlive(const EntityID& entity) const
		{
			if (entity >= m_enitiesDataCount) return false;
			return m_EntityData[entity].IsAlive;
		}

		/// <summary>
		/// 
		/// </summary>
		/// <param name="entity"></param>
		/// <param name="isActive"></param>
		/// <returns>true if isActive state was changed, else false.</returns>
		inline bool SetEntityActive(const EntityID& entity, const bool& isActive)
		{
			if (entity >= m_enitiesDataCount) return false;

			auto& data = m_EntityData[entity];
			if (!data.IsAlive) return false;

			if (data.IsActive != isActive)
			{
				data.IsActive = isActive;
				auto& archEntityData = data.CurrentArchetype->m_EntitiesData[data.IndexInArchetype];
				archEntityData.IsActive = isActive;
				return true;
			}
			return false;
		}

		inline bool IsEntityActive(const EntityID& entity) const
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

		inline EntityData& GetEntityData(const EntityID& entity)
		{
			return m_EntityData[entity];
		}

		inline const EntityData& GetConstEntityData(const EntityID& entity) const { return m_EntityData[entity]; }
	private:
		std::vector<EntityData> m_EntityData;

		EntityID m_CreatedEntitiesCount = 0;
		uint64_t m_enitiesDataCount = 0;

		std::vector<uint64_t> m_FreeEntities;
		ChunkedVector<Entity> m_Entities;
		uint64_t m_FreeEntitiesCount = 0;
	};
}