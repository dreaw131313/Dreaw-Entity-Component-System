#pragma once
#include "Core.h"
#include "Archetype.h"

namespace decs
{
	enum class EntityDestructionState : uint8_t
	{
		Alive = 1,
		InDestruction = 2,
		DelayedToDestruction = 3,
		Destructed = 4
	};

	class EntityData
	{
	public:
		EntityID m_ID = std::numeric_limits<EntityID>::max();
		uint32_t m_Version = std::numeric_limits<uint32_t>::max();
		bool m_IsAlive = false;
		bool m_IsActive = false;
		bool m_IsEntityInDestruction = false;
		bool m_IsEntityDelayedToDestroy = false;

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
	};
}