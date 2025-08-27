#pragma once
#include "Core.h"

#include "ComponentContainers\StableContainer.h"

#include <iostream>

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

	class EntityData
	{
		friend class Archetype;
		friend class EntityManager;
		friend class Container;
		friend class Entity;
		friend class EntityManager;
		friend struct EntityDataHandle;

	private:
		Archetype* m_Archetype = nullptr;
		Container* m_Container = nullptr;

		EntityID m_ID = std::numeric_limits<EntityID>::max();
		uint32_t m_IndexInArchetype = std::numeric_limits<uint32_t>::max();

		EntityVersion m_Version = 1;
		EEntityState m_State = EEntityState::Alive;

		bool m_bIsActive = false;
		bool m_bIsUsedAsPrefab = false;
		bool m_bIsInManager = true;
		bool m_bIsCreatedByContainer = false;
		bool m_bIsEnabledByContainer = false;

		std::atomic<uint32_t> m_RefCounter = 0;

	public:
		EntityData()
		{
			std::cout << "EntityData::Constructor" << "\n";
		}

		EntityData(EntityID id, bool bIsActive):
			m_ID(id),
			m_bIsActive(bIsActive)
		{

			std::cout << "EntityData::Constructor" << "\n";
		}

		~EntityData()
		{
			std::cout << "EntityData::Destructor" << "\n";
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

		inline bool IsAlive() const noexcept
		{
			return m_State != EEntityState::Dead;
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
			return m_State == EEntityState::Alive && !m_bIsUsedAsPrefab;
		}

		inline bool IsValidToChangeActiveState()
		{
			return m_State == EEntityState::Alive;
		}

		inline bool CanBeDestructed() const
		{
			return m_State == EEntityState::Alive && !m_bIsUsedAsPrefab;
		}

		inline bool IsDelayedToDestruction() const
		{
			return m_State == EEntityState::DelayedToDestruction;
		}

		inline bool IsUsedAsPrefab() const
		{
			return m_bIsUsedAsPrefab;
		}

		void SetState(EEntityState state);

		void SetStateRaw(EEntityState state);

		uint32_t GetComponentAndTagCount() const;

		uint32_t GetComponentOnlyCount() const;

		void SetActiveState(bool state);

		inline bool IsInManager() const
		{
			return m_bIsInManager;
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
			m_Archetype = nullptr;
			m_Version += 1;
			m_State = EEntityState::Dead;
			m_Container = nullptr;
		}

		void SetIsInManager(bool bIsInManager)
		{
			m_bIsInManager = bIsInManager;
		}
	};

	struct EntityDataHandle
	{
	public:
		EntityDataHandle() = default;

		EntityDataHandle(EntityData* entityData):
			m_EntityData(entityData)
		{
			IncrementRefCount();
		}

		EntityDataHandle(const EntityDataHandle& other)
		{
			OnCopy(other);
		}

		EntityDataHandle(EntityDataHandle&& other) noexcept
		{
			OnMove(std::move(other));
		}

		~EntityDataHandle()
		{
			DecrementRefCount();
		}

		EntityDataHandle& operator = (const EntityDataHandle& other)
		{
			if (&other != this)
			{
				OnCopy(other);
			}
			return *this;
		}

		EntityDataHandle& operator=(EntityDataHandle&& other) noexcept
		{
			if (&other != this)
			{
				OnMove(std::move(other));
			}
			return *this;
		}

		bool operator==(const EntityDataHandle& rhs)const
		{
			return this->m_EntityData == rhs.m_EntityData;
		}

		inline EntityData* GetEntityData() const
		{
			return m_EntityData;
		}

		inline bool IsValid() const
		{
			return m_EntityData != nullptr;
		}
	private:
		EntityData* m_EntityData = nullptr;

	private:
		void IncrementRefCount() 
		{
			if (m_EntityData != nullptr)
			{
				m_EntityData->m_RefCounter.fetch_add(1, std::memory_order_relaxed);
			}
		}

		void DecrementRefCount() 
		{
			if (m_EntityData != nullptr)
			{
				if (m_EntityData->m_RefCounter.fetch_sub(1, std::memory_order_acq_rel) == 1)
				{
					std::atomic_thread_fence(std::memory_order_acquire);
					delete m_EntityData;
					m_EntityData = nullptr;
				}
			}
		}

		void OnMove(EntityDataHandle&& other)
		{
			DecrementRefCount();
			m_EntityData = other.m_EntityData;
			other.m_EntityData = nullptr;
		}

		void OnCopy(const EntityDataHandle& other)
		{
			DecrementRefCount();
			m_EntityData = other.m_EntityData;
			IncrementRefCount();
		}
	};

}