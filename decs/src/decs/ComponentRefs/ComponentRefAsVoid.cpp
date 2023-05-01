#pragma once
#include "ComponentRefAsVoid.h"
#include "decs\Entity.h"

namespace decs
{
	ComponentRefAsVoid::ComponentRefAsVoid(TypeID typeID, EntityData& entityData) :
		m_TypeID(typeID),
		m_EntityData(&entityData)
	{
		FetchWhenIsInvalid();
	}

	ComponentRefAsVoid::ComponentRefAsVoid(TypeID typeID, EntityData& entityData, uint32_t componentIndex) :
		m_TypeID(typeID),
		m_EntityData(&entityData),
		m_ComponentIndex(componentIndex)
	{
		FetchWithoutGettingComponentIndex();
	}

	ComponentRefAsVoid::ComponentRefAsVoid(TypeID typeID, Entity& entity) :
		m_TypeID(typeID),
		m_EntityData(entity.m_EntityData)
	{
		if (m_EntityData != nullptr)
		{
			FetchWhenIsInvalid();
		}
	}

	void ComponentRefAsVoid::Set(TypeID typeID, EntityData& entityData, uint32_t componentIndex)
	{
		m_TypeID = typeID;
		m_EntityData = &entityData;
		m_ComponentIndex = componentIndex;
		FetchWithoutGettingComponentIndex();
	}

}