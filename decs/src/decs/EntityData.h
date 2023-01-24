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

		Archetype* m_Archetype = nullptr;
		uint32_t m_IndexInArchetype = std::numeric_limits<uint32_t>::max();

	public:
		EntityData()
		{

		}

		EntityData(const EntityID& id, const bool& isActive) : m_ID(id), m_Version(1), m_IsAlive(true), m_IsActive(isActive)
		{

		}

		template<typename ComponentType>
		inline ComponentType* GetComponent()
		{
			if (m_Archetype == nullptr) return nullptr;
			uint64_t componentIndex = m_Archetype->FindTypeIndex<ComponentType>();
			if (componentIndex == std::numeric_limits<uint64_t>::max()) return nullptr;

			PackedContainer<ComponentType>* packedComponent = reinterpret_cast<PackedContainer<ComponentType>*>(m_Archetype->m_PackedContainers[componentIndex]);

			return &packedComponent->m_Data[m_IndexInArchetype];
		}

		template<typename ComponentType>
		inline ComponentType* GetComponent(const uint64_t& componentIndex)
		{
			PackedContainer<ComponentType>* packedComponent = reinterpret_cast<PackedContainer<ComponentType>*>(m_Archetype->m_PackedContainers[componentIndex]);

			return &packedComponent->m_Data[m_IndexInArchetype];
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

		inline bool CanBeDestructed() const
		{
			return m_IsAlive && m_DestructionState != EntityDestructionState::InDestruction;
		}

		inline bool IsDelayedToDestruction()
		{
			return m_DestructionState == EntityDestructionState::DelayedToDestruction;
		}
	};

	template<typename ComponentType>
	class EntityCompnentRef final
	{
	public:
		EntityCompnentRef(EntityData& entityData) :
			m_EntityData(&entityData)
		{
			FetchWhenIsInvalid();
		}

		EntityCompnentRef(
			EntityData& entityData,
			const uint32_t& componentIndex
		) :
			m_EntityData(&entityData)
		{
			FetchWithoutGettingComponentIndex();
		}

		inline ComponentType* Get()
		{
			if (!IsValid())
			{
				FetchWhenIsInvalid();
			}
			return m_CachedComponent;
		}

	private:
		EntityData* m_EntityData;
		Archetype* m_Archetype = nullptr;
		uint32_t m_IndexInArchetype = std::numeric_limits<uint32_t>::max();
		uint64_t m_ComponentIndex = std::numeric_limits<uint32_t>::max();
		ComponentType* m_CachedComponent = nullptr;

	private:
		inline bool IsValid() const noexcept
		{
			return m_EntityData->m_Archetype == m_Archetype && m_EntityData->m_IndexInArchetype == m_IndexInArchetype;
		}

		inline void FetchWhenIsInvalid()
		{
			if (m_Archetype != m_EntityData->m_Archetype)
			{
				m_ComponentIndex = m_Archetype->FindTypeIndex<ComponentType>();
			}
			m_Archetype = m_EntityData->m_Archetype;
			m_IndexInArchetype = m_EntityData->m_IndexInArchetype;

			if (m_Archetype == nullptr || m_ComponentIndex == std::numeric_limits<uint64_t>)
			{
				m_CachedComponent = nullptr;
			}
			else
			{
				m_CachedComponent = &((PackedContainer<ComponentType>*)m_EntityData.m_Archetype->m_PackedContainers[m_ComponentIndex])->m_Data[m_IndexInArchetype];
			}
		}

		inline void FetchWithoutGettingComponentIndex()
		{
			m_Archetype = m_EntityData->m_Archetype;
			m_IndexInArchetype = m_EntityData->m_IndexInArchetype;

			if (m_Archetype == nullptr || m_ComponentIndex == std::numeric_limits<uint64_t>)
			{
				m_CachedComponent = nullptr;
			}
			else
			{
				m_CachedComponent = &((PackedContainer<ComponentType>*)m_EntityData.m_Archetype->m_PackedContainers[m_ComponentIndex])->m_Data[m_IndexInArchetype];
			}
		}
	};
}