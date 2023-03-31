#pragma once
#include "Core.h"
#include "Archetypes\Archetype.h"
#include "ComponentContainers\StableContainer.h"

namespace decs
{
	enum class EntityState : uint8_t
	{
		Dead = 0,
		Alive = 1,
		InDestruction = 2,
		DelayedToDestruction = 3,
	};

	class EntityData
	{
	public:
		Archetype* m_Archetype = nullptr;

		EntityID m_ID = std::numeric_limits<EntityID>::max();
		uint32_t m_IndexInArchetype = std::numeric_limits<uint32_t>::max();
		EntityVersion m_Version = 1;

		EntityState m_State = EntityState::Alive;
		bool m_IsActive = false;
		bool m_IsUsedAsPrefab = false;

	public:
		EntityData()
		{

		}

		EntityData(EntityID id, bool isActive) : m_ID(id), m_IsActive(isActive)
		{

		}

		inline EntityID ID() const noexcept { return m_ID; }

		inline bool IsAlive() const noexcept
		{
			return m_State == EntityState::Alive;
		}

		inline bool IsDead() const
		{
			return m_State != EntityState::Dead;
		}

		inline bool IsInDestruction() const
		{
			return m_State == EntityState::InDestruction;
		}

		inline bool IsValidToPerformComponentOperation() const
		{
			return m_State == decs::EntityState::Alive && !m_IsUsedAsPrefab;
		}

		inline bool CanBeDestructed() const
		{
			return m_State != EntityState::InDestruction && m_State != EntityState::Dead && !m_IsUsedAsPrefab;
		}

		inline bool IsDelayedToDestruction() const
		{
			return m_State == EntityState::DelayedToDestruction;
		}

		inline bool IsUsedAsPrefab() const
		{
			return m_IsUsedAsPrefab;
		}
		
		inline void SetState(EntityState state)
		{
			m_State = state;

			switch (state)
			{
				case EntityState::Alive:
				{
					break;
				}
				case EntityState::Dead:
				case EntityState::InDestruction:
				case EntityState::DelayedToDestruction:
				{
					m_IsActive = false;
					break;
				}
			}
		}

		inline uint32_t ComponentsCount()
		{
			if (m_Archetype == nullptr) return 0;
			return m_Archetype->ComponentsCount();
		}
	};

}