#pragma once
#include "EntityData.h"
#include "Archetypes\Archetype.h"

namespace decs
{
	void EntityData::SetState(EntityState state)
	{
		if (state != m_State)
		{
			m_State = state;

			switch (state)
			{
				case EntityState::Alive:
				{
					break;
				}
				case EntityState::Dead:
				{
					m_bIsActive = false;
					break;
				}
				case EntityState::InDestruction:
				case EntityState::DelayedToDestruction:
				{
					m_bIsActive = false;

					break;
				}
			}
		}
	}

	uint32_t EntityData::ComponentsCount() const
	{
		if (m_Archetype == nullptr) return 0;
		return m_Archetype->ComponentsCount();
	}
}