#pragma once
#include "Core.h"
#include "ComponentContainers\StableContainer.h"

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

	enum class EEntityCallbackState : uint8_t
	{
		None = 0, // every operation on entity can be performed
		Create, // cannot destroy entity
		Destroy, // cannont do anything
		Disable,
		Enable
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
		EEntityState m_State = EEntityState::Alive;
		EEntityCallbackState m_EntityCallbackState = EEntityCallbackState::None;

		bool m_bCanPerformOperation = false;
		bool m_bIsActive = false;
		bool m_bIsUsedAsPrefab = false;
		bool m_bIsInManager = true;
		bool m_bIsCreatedByContainer = false;
		bool m_bIsEnabledByContainer = false;

	public:
		EntityData()
		{

		}

		EntityData(EntityID id, bool bIsActive) :
			m_ID(id),
			m_bIsActive(bIsActive)
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

		inline bool IsValidToPerformComponentOperation() const
		{
			return m_State == EEntityState::Alive && !m_bIsUsedAsPrefab;
		}

		inline bool CanBeDestructed() const
		{
			return m_State != EEntityState::InDestruction && m_State != EEntityState::Dead && !m_bIsUsedAsPrefab;
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

		uint32_t ComponentCount() const;

		void SetActiveState(bool state);

		inline bool IsInManager() const 
		{
			return m_bIsInManager;
		}

		EEntityCallbackState GetEntityCallbackState() const
		{
			return m_EntityCallbackState;
		}

		void SetEntityCallbackState(EEntityCallbackState callbackState)
		{
			m_EntityCallbackState = callbackState;
		}

	private:
		inline void OnDestroyByEntityManager()
		{
			m_Archetype = nullptr;
			m_Version += 1;
			m_State = EEntityState::Dead;
		}

		void SetIsInManager(bool bIsInManager)
		{
			m_bIsInManager = bIsInManager;
		}
	};

}