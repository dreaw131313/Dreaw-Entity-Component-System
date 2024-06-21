#pragma once
#include "EntityData.h"
#include "Archetypes\Archetype.h"

namespace decs
{
	void EntityData::SetState(EEntityState state)
	{
		if (state != m_State)
		{
			m_State = state;

			switch (state)
			{
				case EEntityState::Alive:
				{
					break;
				}
				case EEntityState::Dead:
				case EEntityState::InDestruction:
				case EEntityState::DelayedToDestruction:
				{
					SetActiveState(false);
					break;
				}
			}
		}
	}

	void EntityData::SetStateRaw(EEntityState state)
	{
		m_State = state;
	}

	uint32_t EntityData::ComponentCount() const
	{
		if (m_Archetype == nullptr) return 0;
		return m_Archetype->ComponentCount();
	}

	void EntityData::SetActiveState(bool state)
	{
		m_bIsActive = state;

		if (m_Archetype != nullptr)
		{
			m_Archetype->SetEntityActiveState(m_IndexInArchetype, state);
		}
	}
}