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
		ComponentRef()
		{

		}

		ComponentRef(EntityData& entityData) :
			m_EntityData(&entityData)
		{
			FetchWhenIsInvalid();
		}

		ComponentRef(
			const Entity& entity
		) :
			m_EntityData(entity.m_EntityData)
		{
			if (m_EntityData != nullptr)
			{
				m_EntityVersion = entity.m_EntityData->m_Version;
				FetchWhenIsInvalid();
			}
		}

		inline bool IsNull()
		{
			return  Get() == nullptr;
		}

		inline bool IsValid()
		{
			return Get() != nullptr;
		}

		inline typename component_type<ComponentType>::Type* Get()
		{
			if (m_EntityData != nullptr && IsEntityVersionValid())
			{
				if (!IsArchetypeValid())
				{
					FetchWhenIsInvalid();
				}
				if (m_PackedContainer != nullptr)
				{
					return m_PackedContainer->GetAsPtr(m_EntityData->m_IndexInArchetype);
				}
			}
			return nullptr;
		}

		inline operator bool() { return Get() != nullptr; }

		inline typename component_type<ComponentType>::Type* operator->()
		{
			return Get();
		}

		void Invalidate()
		{
			m_EntityData = nullptr;
			m_Archetype = nullptr;
			m_PackedContainer = nullptr;
			m_EntityVersion = std::numeric_limits<EntityVersion>::max();
		}

	private:
		EntityData* m_EntityData = nullptr;
		Archetype* m_Archetype = nullptr;
		PackedContainer<ComponentType>* m_PackedContainer = nullptr;
		EntityVersion m_EntityVersion = std::numeric_limits<EntityVersion>::max();

	private:
		inline bool IsArchetypeValid() const
		{
			return m_EntityData->m_Archetype == m_Archetype;
		}

		inline bool IsEntityVersionValid() const
		{
			return m_EntityData->m_Version == m_EntityVersion;
		}

		inline void FetchWhenIsInvalid()
		{
			m_Archetype = m_EntityData->m_Archetype;
			uint32_t compIndex = m_Archetype->FindTypeIndex<ComponentType>();

			if (m_Archetype != nullptr && compIndex != Limits::MaxComponentCount)
			{
				m_PackedContainer = dynamic_cast<PackedContainer<ComponentType>*>(m_Archetype->m_TypeData[compIndex].m_PackedContainer);
			}
			else
			{
				m_PackedContainer = nullptr;
			}
		}
	};

}