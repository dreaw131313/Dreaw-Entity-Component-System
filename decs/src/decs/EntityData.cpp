#pragma once
#include "EntityData.h"
#include "Archetypes\Archetype.h"

namespace decs
{
	uint32_t EntityData::GetComponentAndTagCount() const
	{
		if (m_Archetype == nullptr) return 0;
		return m_Archetype->GetComponentAndTagCount();
	}

	uint32_t EntityData::GetComponentOnlyCount() const
	{
		if (m_Archetype == nullptr) return 0;
		return m_Archetype->GetComponentOnlyCount();
	}
}