#pragma once
#include "Core.h"

namespace decs
{
	class Entity;


	// Entities
	class EntityData
	{
	public:
		EntityID m_ID = std::numeric_limits<EntityID>::max();
		uint32_t m_Version = std::numeric_limits<uint32_t>::max();
		bool m_IsAlive = false;
		bool m_IsActive = false;
		bool m_IsEntityInDestruction = false;

		class Archetype* m_CurrentArchetype = nullptr;
		uint32_t m_IndexInArchetype = std::numeric_limits<uint32_t>::max();

		Entity* m_EntityPtr = nullptr;

	public:
		EntityData()
		{

		}

		EntityData(const EntityID& id, const bool& isActive) : m_ID(id), m_Version(1), m_IsAlive(true), m_IsActive(isActive)
		{

		}

	};
}