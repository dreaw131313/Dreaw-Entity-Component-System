#pragma once
#include "Core.h"
#include "Archetype.h"

namespace decs
{
	enum class EntityDestructionState : uint8_t
	{
		Alive = 1,
		InDestruction = 2,
		DelayedToDestruction = 3
	};

	class EntityData
	{
	public:
		EntityID m_ID = std::numeric_limits<EntityID>::max();
		uint32_t m_Version = std::numeric_limits<uint32_t>::max();
		bool m_IsAlive = false;
		bool m_IsActive = false;
		EntityDestructionState m_DestructionState = EntityDestructionState::Alive;

		Archetype* m_CurrentArchetype = nullptr;
		uint32_t m_IndexInArchetype = std::numeric_limits<uint32_t>::max();

	public:
		EntityData()
		{

		}

		EntityData(const EntityID& id, const bool& isActive) : m_ID(id), m_Version(1), m_IsAlive(true), m_IsActive(isActive)
		{

		}

		inline ComponentRef& GetComponentRef(const uint64_t& componentTypeIndex)
		{
			uint64_t dataIndex = (uint64_t)m_CurrentArchetype->GetComponentsCount() * (uint64_t)m_IndexInArchetype + componentTypeIndex;
			return m_CurrentArchetype->m_ComponentsRefs[dataIndex];
		}

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
			return m_IsAlive && m_DestructionState == decs::EntityDestructionState::Alive;
		}
	};
}