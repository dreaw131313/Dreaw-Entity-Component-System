#pragma once
#include "Core.h"

namespace decs
{
	class Archetype;

	class EntityData final
	{
		friend class Archetype;
		friend class EntityManager;
		friend class Container;
		friend class Entity;
		friend class EntityManager;

	private:
		Archetype* m_Archetype = nullptr;
		Container* m_Container = nullptr;

		EntityID m_ID = std::numeric_limits<EntityID>::max();
		uint32_t m_IndexInArchetype = std::numeric_limits<uint32_t>::max();
		EntityVersion m_Version = 1;
		bool m_bIsAlive = true;

	public:
		EntityData() = delete;
		EntityData(const EntityData&) = delete;
		EntityData(EntityData&&) = delete;
		EntityData& operator=(const EntityData&) = delete;
		EntityData& operator=(EntityData&&) = delete;

		EntityData(EntityID id):
			m_ID(id)
		{
		}

		~EntityData()
		{
		}


		inline EntityVersion GetVersion() const
		{
			return m_Version;
		}

		inline EntityID GetID() const noexcept
		{
			return m_ID;
		}

		inline bool IsAlive() const noexcept
		{
			return m_bIsAlive;
		}

		inline bool IsAliveWithVersion(uint32_t desiredVersion) const noexcept
		{
			return desiredVersion == m_Version && m_bIsAlive;
		}

		uint32_t GetComponentAndTagCount() const;

		inline bool IsInManager() const
		{
			return m_Container == nullptr;
		}

	private:
		inline void OnDestroyByEntityManager()
		{
			m_Container = nullptr;
			m_Archetype = nullptr;
			m_Version += 1;
			m_bIsAlive = false;
		}
	};

}