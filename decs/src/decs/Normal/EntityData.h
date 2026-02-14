#pragma once

#include "decs/Core/RefCounterHandle.h"

namespace decs
{
	class Archetype;

	enum class EEntityState : uint8_t
	{
		Dead = 0,
		Alive = 1,
		InDestruction = 2,
		DelayedToDestruction = 3,
	};

	/// <summary>
	/// Helper class which allows simple flag change to set all entities as dead
	/// </summary>
	struct EnityLifeTimeData : 
		public RefCountedObject,
		public NonCopyableNonMoveable
	{
		friend class Container;

	public:
		EnityLifeTimeData() = default;

		~EnityLifeTimeData() = default;

		inline bool IsAlive() const noexcept
		{
			return m_bIsContainerAlive.load();
		}

	private:
		std::atomic<bool> m_bIsContainerAlive = true;
	};

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

		EntityVersion m_Version = 1; // allways startes with 1 that entity with id 0 and version 1 not make combined id zero
		/// <summary>
		/// how many times disabled override was performed. Every disable override increments disable counter, and enable decrements. If counter is equal 0 means there is no disable overrides, and enabling override do not change counter value.
		/// </summary>
		uint32_t m_DisabledOverrideCount = 0;

		EEntityState m_State = EEntityState::Alive;

		bool m_bIsActive = false;
		bool m_bIsCreatedByContainer = false;
		bool m_bIsEnabledByContainer = false;

	public:
		EntityData() = delete;
		EntityData(const EntityData&) = delete;
		EntityData(EntityData&&) = delete;
		EntityData& operator=(const EntityData&) = delete;
		EntityData& operator=(EntityData&&) = delete;

		EntityData(EntityID id, bool bIsActive):
			m_ID(id),
			m_bIsActive(bIsActive)
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

		inline bool IsActive() const noexcept
		{
			return m_bIsActive && m_DisabledOverrideCount == 0;
		}

		inline bool IsActiveFlag() const noexcept
		{
			return m_bIsActive;
		}

		inline uint32_t GetDisabledOverrideCount() const noexcept
		{
			return m_DisabledOverrideCount;
		}

		inline bool IsAlive() const noexcept
		{
			return m_State != EEntityState::Dead;
		}

		inline bool IsAliveWithVersion(uint32_t desiredVersion) const noexcept
		{
			return desiredVersion == m_Version && IsAlive();
		}

		inline bool IsDead() const
		{
			return m_State == EEntityState::Dead;
		}

		inline bool IsInDestruction() const
		{
			return m_State == EEntityState::InDestruction;
		}

		inline bool IsInDestructionOrDelayedToDestruction() const
		{
			return m_State == EEntityState::InDestruction
				|| m_State == EEntityState::DelayedToDestruction;
		}

		inline bool IsValidToPerformComponentOperation() const
		{
			return m_State == EEntityState::Alive;
		}

		inline bool IsValidToChangeActiveState()
		{
			return m_State == EEntityState::Alive;
		}

		inline bool CanBeDestructed() const
		{
			return m_State == EEntityState::Alive;
		}

		inline bool IsDelayedToDestruction() const
		{
			return m_State == EEntityState::DelayedToDestruction;
		}

		void SetState(EEntityState state);

		void SetStateRaw(EEntityState state);

		uint32_t GetComponentAndTagCount() const;

		uint32_t GetComponentOnlyCount() const;

		void SetActiveState(bool state);

		void SetDisableOverride(bool bDisableOverride);

		void SetDisabledOverrideCount(uint32_t disableOverrideCount);

		void ResetDisableOverrideCount();

		inline bool IsInManager() const
		{
			return m_Container == nullptr;
		}

		void SetValidStateOnCreateFromReservedEntityData(bool bIsActive)
		{
			SetActiveState(bIsActive);
			SetState(EEntityState::Alive);
			m_bIsCreatedByContainer = false;
			m_bIsEnabledByContainer = false;
		}

	private:
		inline void OnDestroyByEntityManager()
		{
			m_Container = nullptr;
			m_Archetype = nullptr;
			m_Version += 1;
			m_State = EEntityState::Dead;
		}
	};

}