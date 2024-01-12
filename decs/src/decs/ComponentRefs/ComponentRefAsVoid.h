#pragma once
#include "decs/Core.h"
#include "decs/Type.h"
#include "decs/EntityData.h"
#include "Archetypes/Archetype.h"

namespace decs
{
	class Entity;

	class ComponentRefAsVoid final
	{
		friend class Container;
	public:
		ComponentRefAsVoid() {}

		ComponentRefAsVoid(TypeID typeID, EntityData& entityData);

		ComponentRefAsVoid(TypeID typeID, EntityData& entityData, uint32_t componentIndex);

		ComponentRefAsVoid(TypeID typeID, Entity& entity);

		inline void* Get()
		{
			if (m_EntityData != nullptr && IsEntityVersionValid())
			{
				if (!IsArchetypeValid())
				{
					FetchWhenArchetypeIsInvalid();
				}
				if (m_PackedContainer != nullptr)
				{
					return m_PackedContainer->GetComponentPtrAsVoid(m_EntityData->m_IndexInArchetype);
				}
			}
			return nullptr;
		}

		operator bool() { return Get() != nullptr; }

	private:
		EntityData* m_EntityData = nullptr;
		Archetype* m_Archetype = nullptr;
		PackedContainerBase* m_PackedContainer = nullptr;
		TypeID m_TypeID = std::numeric_limits<TypeID>::min();
		EntityVersion m_EntityVersion = Limits::MaxVersion;

	private:
		void Set(TypeID typeID, EntityData& entityData, uint32_t componentIndex);

		inline bool IsArchetypeValid() const
		{
			return m_EntityData->m_Archetype == m_Archetype;
		}

		inline bool IsEntityVersionValid() const
		{
			return m_EntityData->GetVersion() == m_EntityVersion;
		}

		inline void FetchWhenArchetypeIsInvalid()
		{
			m_Archetype = m_EntityData->m_Archetype;
			if (m_Archetype != nullptr)
			{
				uint32_t compIndex = m_Archetype->FindTypeIndex(m_TypeID);
				if (compIndex != Limits::MaxComponentCount)
				{
					m_PackedContainer = m_Archetype->m_TypeData[compIndex].m_PackedContainer;
					return;
				}
			}

			m_PackedContainer = nullptr;
		}

		inline void FetchWithoutGettingComponentIndex(uint32_t componentIndex)
		{
			m_Archetype = m_EntityData->m_Archetype;
			if (m_Archetype != nullptr && componentIndex != Limits::MaxComponentCount)
			{
				m_PackedContainer = m_Archetype->m_TypeData[componentIndex].m_PackedContainer;
			}
			else
			{
				m_PackedContainer = nullptr;
			}
		}
	};
}
