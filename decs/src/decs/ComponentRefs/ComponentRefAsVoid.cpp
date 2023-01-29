#pragma once
#include "ComponentRefAsVoid.h"
#include "decs\Entity.h"

namespace decs
{
	ComponentRefAsVoid::ComponentRefAsVoid(const TypeID& typeID, EntityData& entityData) :
		m_TypeID(typeID),
		m_EntityData(&entityData)
	{
		FetchWhenIsInvalid();
	}

	ComponentRefAsVoid::ComponentRefAsVoid(const TypeID& typeID, EntityData& entityData, const uint32_t& componentIndex) :
		m_TypeID(typeID),
		m_EntityData(&entityData),
		m_ComponentIndex(componentIndex)
	{
		FetchWithoutGettingComponentIndex();
	}

	ComponentRefAsVoid::ComponentRefAsVoid(const TypeID& typeID, Entity& entity) :
		m_TypeID(typeID),
		m_EntityData(entity.m_EntityData)
	{
		if (m_EntityData != nullptr)
		{
			FetchWhenIsInvalid();
		}
	}

}