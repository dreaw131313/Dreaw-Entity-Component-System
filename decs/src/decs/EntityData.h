#pragma once
#include "Core.h"
#include "Archetype.h"

namespace decs
{
	class EntityData
	{
	public:
		EntityID m_ID = std::numeric_limits<EntityID>::max();
		uint32_t m_Version = std::numeric_limits<uint32_t>::max();
		bool m_IsAlive = false;
		bool m_IsActive = false;
		bool m_IsEntityInDestruction = false;

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