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
			const uint32_t& componentIndex
		) :
			m_EntityData(&entityData),
			m_ComponentIndex(componentIndex)
		{
			FetchWithoutGettingComponentIndex();
		}

		ComponentRef(
			Entity& entity
		) :
			m_EntityData(entity.m_EntityData)
		{
			if (m_EntityData != nullptr)
			{
				FetchWhenIsInvalid();
			}
		}

		inline ComponentType* Get()
		{
			if (!IsValid())
			{
				FetchWhenIsInvalid();
			}
			if (m_ComponentsVector != nullptr)
			{
				return &m_ComponentsVector->operator[](m_IndexInArchetype);
			}
			return nullptr;
		}

		operator bool() { return Get() != nullptr; }

	private:
		EntityData* m_EntityData = nullptr;
		Archetype* m_Archetype = nullptr;
		uint32_t m_IndexInArchetype = std::numeric_limits<uint32_t>::max();
		uint32_t m_ComponentIndex = Limits::MaxComponentCount;
		std::vector<ComponentType>* m_ComponentsVector = nullptr;

	private:
		inline bool IsValid() const
		{
			return m_EntityData->m_Archetype == m_Archetype && m_EntityData->m_IndexInArchetype == m_IndexInArchetype;
		}

		inline void SetComponentFromValidData()
		{
			m_ComponentsVector = &((PackedContainer<ComponentType>*)m_Archetype->m_PackedContainers[m_ComponentIndex])->m_Data;
		}

		inline void FetchWhenIsInvalid()
		{
			if (m_Archetype != m_EntityData->m_Archetype)
			{
				m_Archetype = m_EntityData->m_Archetype;
				m_ComponentIndex = m_Archetype->FindTypeIndex<ComponentType>();
			}
			m_IndexInArchetype = m_EntityData->m_IndexInArchetype;

			if (m_Archetype == nullptr || m_ComponentIndex == std::numeric_limits<uint32_t>::max())
			{
				m_ComponentsVector = nullptr;
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

			if (m_Archetype == nullptr || m_ComponentIndex == std::numeric_limits<uint32_t>::max())
			{
				m_ComponentsVector = nullptr;
			}
			else
			{
				SetComponentFromValidData();
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
					if (m_ComponentIndex != std::numeric_limits<uint32_t>::max())
					{
						SetComponentFromValidData();
					}
				}
			}
		}
	};

}