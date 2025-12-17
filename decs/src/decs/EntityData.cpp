#pragma once
#include "EntityData.h"
#include "Archetypes\Archetype.h"

namespace decs
{
	uint32_t EntityData::GetTypeCount() const
	{
		if (m_Archetype == nullptr) return 0;
		return m_Archetype->GetTypeCount();
	}

}