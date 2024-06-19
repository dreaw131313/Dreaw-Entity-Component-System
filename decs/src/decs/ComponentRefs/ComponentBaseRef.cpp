#pragma once
#include "ComponentBaseRef.h"
#include "decs\Entity.h"

namespace decs
{
	ComponentBaseRef::ComponentBaseRef(TypeID typeID, EntityData& entityData) :
		m_TypeID(typeID),
		m_EntityData(&entityData),
		m_EntityVersion(entityData.GetVersion())
	{
		if (m_EntityData != nullptr)
		{
			FetchWhenArchetypeIsInvalid();
		}
	}

	ComponentBaseRef::ComponentBaseRef(TypeID typeID, EntityData& entityData, uint32_t componentIndex) :
		m_TypeID(typeID),
		m_EntityData(&entityData),
		m_EntityVersion(entityData.GetVersion())
	{
		if (m_EntityData != nullptr)
		{
			FetchWithoutGettingComponentIndex(componentIndex);
		}
	}

	ComponentBaseRef::ComponentBaseRef(TypeID typeID, Entity& entity) :
		m_TypeID(typeID),
		m_EntityData(entity.m_EntityData),
		m_EntityVersion(entity.m_EntityData->GetVersion())
	{
		if (m_EntityData != nullptr)
		{
			FetchWhenArchetypeIsInvalid();
		}
	}

	void ComponentBaseRef::Set(TypeID typeID, EntityData& entityData, uint32_t componentIndex)
	{
		m_TypeID = typeID;
		m_EntityData = &entityData;
		m_EntityVersion = entityData.GetVersion();
		if (m_EntityData != nullptr)
		{
			FetchWithoutGettingComponentIndex(componentIndex);
		}
	}

}