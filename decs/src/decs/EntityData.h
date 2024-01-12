#pragma once
#include "Core.h"
#include "ComponentContainers\StableContainer.h"

namespace decs
{
	class Archetype;

	enum class EntityState : uint8_t
	{
		Dead = 0,
		Alive = 1,
		InDestruction = 2,
		DelayedToDestruction = 3,
	};
	
	class EntityData
	{
		friend class EntityManager;
		friend class Container;

	public:
		Archetype* m_Archetype = nullptr;

	private:
		EntityID m_ID = std::numeric_limits<EntityID>::max();

	public:
		uint32_t m_IndexInArchetype = std::numeric_limits<uint32_t>::max();

	private:
		EntityVersion m_Version = 1;
		EntityState m_State = EntityState::Alive;
		bool m_bIsActive = false;
		bool m_bIsUsedAsPrefab = false;
		bool m_bCanBeDestroyed = true;

	public:
		EntityData()
		{

		}

		EntityData(EntityID id, bool bIsActive, bool bCanBeDestroyed = true) :
			m_ID(id),
			m_bIsActive(bIsActive),
			m_bCanBeDestroyed(bCanBeDestroyed)
		{

		}

		inline EntityVersion GetVersion() const
		{
			return m_Version;
		}

		inline EntityID GetID() const noexcept { return m_ID; }

		inline bool IsActive() const
		{
			return m_bIsActive;
		}

		inline void SetIsActive(bool bIsActive)
		{
			m_bIsActive = bIsActive;
		}

		inline bool IsAlive() const noexcept
		{
			return m_State == EntityState::Alive;
		}

		inline bool IsDead() const
		{
			return m_State == EntityState::Dead;
		}

		inline bool IsInDestruction() const
		{
			return m_State == EntityState::InDestruction;
		}

		inline bool IsValidToPerformComponentOperation() const
		{
			return m_State == EntityState::Alive && !m_bIsUsedAsPrefab;
		}

		inline bool CanBeDestructed() const
		{
			return m_State != EntityState::InDestruction && m_State != EntityState::Dead && !m_bIsUsedAsPrefab;
		}

		inline bool IsDelayedToDestruction() const
		{
			return m_State == EntityState::DelayedToDestruction;
		}

		inline bool IsUsedAsPrefab() const
		{
			return m_bIsUsedAsPrefab;
		}

		void SetState(EntityState state);

		uint32_t ComponentCount() const;

		void SetActiveState(bool state);

	private:
		inline void OnDestroyByEntityManager()
		{
			m_Archetype = nullptr;
			m_Version += 1;
			m_State = EntityState::Dead;
		}
	};

}