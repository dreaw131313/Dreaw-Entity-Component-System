#pragma once
#include "LEntityData.h"
#include "Archetypes\LArchetype.h"

namespace decs::light
{
	uint32_t EntityData::GetComponentTagCount() const
	{
		if (m_Archetype == nullptr) return 0;
		return m_Archetype->GetComponentTagCount();
	}

}