#pragma once
#include "decs\Core.h"
#include "decs\Type.h"
#include "decs\EntityData.h"

namespace decs
{
	class Entity;

	class ComponentRefAsVoid final
	{
	public:
		ComponentRefAsVoid(const TypeID& typeID, EntityData& entityData);
		
		ComponentRefAsVoid(const TypeID& typeID, EntityData& entityData, const uint32_t& componentIndex);
		
		ComponentRefAsVoid(const TypeID& typeID, Entity& entity);

		inline void* Get()
		{
			if (!IsValid())
			{
				FetchWhenIsInvalid();
			}
			if (m_PackedContainer != nullptr)
			{
				return m_PackedContainer->GetComponentAsVoid(m_IndexInArchetype);
			}
			return nullptr;
		}

		operator bool() { return Get() != nullptr; }

	private:
		TypeID m_TypeID = std::numeric_limits<TypeID>::min();
		EntityData* m_EntityData = nullptr;
		Archetype* m_Archetype = nullptr;
		uint32_t m_IndexInArchetype = std::numeric_limits<uint32_t>::max();
		uint32_t m_ComponentIndex = Limits::MaxComponentCount;
		PackedContainerBase* m_PackedContainer = nullptr;
	private:
		inline bool IsValid() const
		{
			return m_EntityData->m_Archetype == m_Archetype && m_EntityData->m_IndexInArchetype == m_IndexInArchetype;
		}

		inline void SetComponentFromValidData()
		{
			m_PackedContainer = m_Archetype->m_PackedContainers[m_ComponentIndex];
		}

		inline void FetchWhenIsInvalid()
		{
			if (m_Archetype != m_EntityData->m_Archetype)
			{
				m_Archetype = m_EntityData->m_Archetype;
				m_ComponentIndex = m_Archetype->FindTypeIndex(m_TypeID);
			}
			m_IndexInArchetype = m_EntityData->m_IndexInArchetype;

			if (m_Archetype == nullptr || m_ComponentIndex == Limits::MaxComponentCount)
			{
				m_PackedContainer = nullptr;
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

			if (m_Archetype == nullptr || m_ComponentIndex == Limits::MaxComponentCount)
			{
				m_PackedContainer = nullptr;
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
					m_ComponentIndex = m_Archetype->FindTypeIndex(m_TypeID);
					if (m_ComponentIndex != Limits::MaxComponentCount)
					{
						SetComponentFromValidData();
					}
				}
			}
		}
	};
}
