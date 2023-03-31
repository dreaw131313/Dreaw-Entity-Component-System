#pragma once
#include "decs/Core.h"
#include "decs/EntityData.h"
#include "decs/Entity.h"

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
			Entity& entity
		) :
			m_EntityData(entity.m_EntityData)
		{
			if (m_EntityData != nullptr)
			{
				FetchWhenIsInvalid();
			}
		}

		inline bool IsNull()
		{
			return  Get() == nullptr;
		}

		inline typename component_type<ComponentType>::Type* Get()
		{
			if (!IsValid())
			{
				FetchWhenIsInvalid();
			}
			if (m_PackedContainer != nullptr)
			{
				return m_PackedContainer->GetAsPtr(m_IndexInArchetype);
			}
			return nullptr;
		}

		inline operator bool() { return Get() != nullptr; }

		inline component_type<ComponentType>::Type* operator->()
		{
			return Get();
		}

	private:
		EntityData* m_EntityData = nullptr;
		Archetype* m_Archetype = nullptr;
		PackedContainer<ComponentType>* m_PackedContainer = nullptr;
		uint32_t m_IndexInArchetype = std::numeric_limits<uint32_t>::max();

	private:
		inline bool IsValid() const
		{
			return m_EntityData->m_Archetype == m_Archetype && m_EntityData->m_IndexInArchetype == m_IndexInArchetype;
		}

		inline void FetchWhenIsInvalid()
		{
			uint32_t compIndex = Limits::MaxComponentCount;
			if (m_Archetype != m_EntityData->m_Archetype)
			{
				m_Archetype = m_EntityData->m_Archetype;
				compIndex = m_Archetype->FindTypeIndex<ComponentType>();
			}
			m_IndexInArchetype = m_EntityData->m_IndexInArchetype;

			if (m_Archetype == nullptr || compIndex == Limits::MaxComponentCount)
			{
				m_PackedContainer = nullptr;
			}
			else
			{
				m_PackedContainer = dynamic_cast<PackedContainer<ComponentType>*>(m_Archetype->m_TypeData[compIndex].m_PackedContainer);
			}
		}
	};

}