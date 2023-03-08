#pragma once
#include "Core.h"
#include "Archetypes\Archetype.h"
#include "ComponentContainers\StableContainer.h"

namespace decs
{
	enum class EntityDestructionState : uint8_t
	{
		Alive = 1,
		InDestruction = 2,
		DelayedToDestruction = 3,
		Destructed
	};

	class EntityData
	{
	public:
		Archetype* m_Archetype = nullptr;
		
		EntityID m_ID = std::numeric_limits<EntityID>::max();
		uint32_t m_IndexInArchetype = std::numeric_limits<uint32_t>::max();
		EntityVersion m_Version = std::numeric_limits<EntityVersion>::max();

		bool m_IsAlive = false;
		bool m_IsActive = false;
		bool m_IsUsedAsPrefab = false;
		EntityDestructionState m_DestructionState = EntityDestructionState::Alive;

	public:
		EntityData()
		{

		}

		EntityData(EntityID id, bool isActive) : m_ID(id), m_Version(1), m_IsAlive(true), m_IsActive(isActive)
		{

		}

		inline EntityID ID() const noexcept { return m_ID; }

		inline bool IsAlive() const
		{
			return m_IsAlive && m_DestructionState == EntityDestructionState::Alive;
		}

		inline bool IsAliveAndInDestruction() const
		{
			return m_IsAlive && m_DestructionState == EntityDestructionState::InDestruction;
		}

		inline bool IsInDestruction() const
		{
			return m_DestructionState == EntityDestructionState::InDestruction;
		}

		inline void SetState(EntityDestructionState state)
		{
			m_DestructionState = state;

			switch (state)
			{
				case EntityDestructionState::Alive:
				{
					break;
				}
				case EntityDestructionState::InDestruction:
				case EntityDestructionState::DelayedToDestruction:
				{
					m_IsActive = false;
					break;
				}
			}
		}

		inline bool IsValidToPerformComponentOperation() const
		{
			return m_IsAlive && m_DestructionState == decs::EntityDestructionState::Alive && !m_IsUsedAsPrefab;
		}

		inline bool CanBeDestructed() const
		{
			return m_IsAlive && m_DestructionState != EntityDestructionState::InDestruction && !m_IsUsedAsPrefab;
		}

		inline bool IsDelayedToDestruction() const
		{
			return m_DestructionState == EntityDestructionState::DelayedToDestruction;
		}
		
		inline bool IsUsedAsPrefab() const
		{
			return m_IsUsedAsPrefab;
		}

		inline uint32_t ComponentsCount()
		{
			if (m_Archetype == nullptr) return 0;
			return m_Archetype->ComponentsCount();
		}
	};

}