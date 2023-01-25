#pragma once
#include "Core.h"
#include "EntityData.h"
#include "Entity.h"

namespace decs
{
	template<typename ComponentType>
	class ComponentRef final
	{
	public:
		ComponentRef(EntityData& entityData) :
			m_EntityData(&entityData)
		{
			FetchWhenIsInvalid();
		}

		ComponentRef(
			EntityData& entityData,
			const uint64_t& componentIndex,
			ComponentType* componentPtr
		) :
			m_EntityData(&entityData),
			m_Archetype(entityData.m_Archetype),
			m_IndexInArchetype(entityData.m_IndexInArchetype),
			m_ComponentIndex(componentIndex),
			m_CachedComponent(componentPtr)
		{

		}

		ComponentRef(
			EntityData& entityData,
			const uint64_t& componentIndex
		) :
			m_EntityData(&entityData)
		{
			FetchWithoutGettingComponentIndex();
		}

		ComponentRef(
			Entity& entity,
			ComponentType* componentPtr
		) :
			m_EntityData(entity.m_EntityData),
			m_CachedComponent(componentPtr)
		{
			if (m_EntityData != nullptr)
			{
				FetchDataFromEntity();
			}
		}

		ComponentRef(
			Entity& entity
		) :
			m_EntityData(entity.m_EntityData)
		{
			if (m_EntityData != nullptr)
			{
				FetchDataFromEntity();
			}
		}

		inline ComponentType* Get()
		{
			if (!IsValid())
			{
				FetchWhenIsInvalid();
			}
			return m_CachedComponent;
		}

		operator bool() { return Get() != nullptr; }

	private:
		EntityData* m_EntityData = nullptr;
		Archetype* m_Archetype = nullptr;
		uint32_t m_IndexInArchetype = std::numeric_limits<uint32_t>::max();
		uint64_t m_ComponentIndex = std::numeric_limits<uint64_t>::max();
		ComponentType* m_CachedComponent = nullptr;

	private:
		inline bool IsValid() const
		{
			return m_EntityData->m_Archetype == m_Archetype && m_EntityData->m_IndexInArchetype == m_IndexInArchetype;
		}

		inline void SetComponentFromValidData()
		{
			m_CachedComponent = &((PackedContainer<ComponentType>*)m_Archetype->m_PackedContainers[m_ComponentIndex])->m_Data[m_IndexInArchetype];
		}

		inline void FetchWhenIsInvalid()
		{
			if (m_Archetype != m_EntityData->m_Archetype)
			{
				m_Archetype = m_EntityData->m_Archetype;
				m_ComponentIndex = m_Archetype->FindTypeIndex<ComponentType>();
			}
			m_IndexInArchetype = m_EntityData->m_IndexInArchetype;

			if (m_Archetype == nullptr || m_ComponentIndex == std::numeric_limits<uint64_t>::max())
			{
				m_CachedComponent = nullptr;
			}
			else
			{
				SetComponentFromValidData();
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
				SetComponentFromValidData();
			}
		}

		inline void FetchDataFromEntity()
		{
			if (m_EntityData != nullptr)
			{
				m_Archetype = m_EntityData->m_Archetype;
				m_IndexInArchetype = m_EntityData->m_IndexInArchetype;
				if (m_Archetype != nullptr)
				{
					m_ComponentIndex = m_Archetype->FindTypeIndex<ComponentType>();
					if (m_CachedComponent == nullptr && m_ComponentIndex != std::numeric_limits<uint64_t>::max())
					{
						SetComponentFromValidData();
					}
				}
			}
		}

		inline void FetchFullDataFromEntity()
		{
			if (m_EntityData != nullptr)
			{
				m_Archetype = m_EntityData->m_Archetype;
				m_IndexInArchetype = m_EntityData->m_IndexInArchetype;
				if (m_Archetype != nullptr)
				{
					m_ComponentIndex = m_Archetype->FindTypeIndex<ComponentType>();
					if (m_ComponentIndex != std::numeric_limits<uint64_t>::max())
					{
						SetComponentFromValidData();
					}
				}
			}
		}


	};
}