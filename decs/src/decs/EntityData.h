#pragma once
#include "decspch.h"

#include "Core.h"


namespace decs
{
	class Entity;
	// Entities
	class EntityData
	{
	public:
		EntityID ID = std::numeric_limits<EntityID>::max();
		uint32_t Version = std::numeric_limits<uint32_t>::max();
		bool IsAlive = false;
		bool IsActive = false;

		class Archetype* CurrentArchetype = nullptr;
		uint32_t IndexInArchetype = std::numeric_limits<uint32_t>::max();
		
		Entity* m_EntityPtr = nullptr;
	public:
		EntityData()
		{

		}

		EntityData(const EntityID& id, const bool& isActive) : ID(id), Version(1), IsAlive(true), IsActive(isActive)
		{

		}

		~EntityData()
		{

		}
	};
}