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
					if (m_Archetype != nullptr)
					{
						m_Archetype->SetEntityActiveState(m_IndexInArchetype, false);
					}
					break;
				}
			}
		}
	}

	void EntityData::SetStateRaw(EEntityState state)
	{
		m_State = state;
	}

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

	void EntityData::SetActiveState(bool state)
	{
		bool bOldActiveState = IsActive();
		{
			m_bIsActive = state;
		}
		bool bNewActiveState = IsActive();

		if (m_Archetype != nullptr && bOldActiveState != bNewActiveState)
		{
			m_Archetype->SetEntityActiveState(m_IndexInArchetype, bNewActiveState);
		}
	}

	void EntityData::SetDisableOverride(bool bDisableOverride)
	{
		bool bOldActiveState = IsActive();
		if (bDisableOverride)
		{
			m_DisabledOverrideCount = m_DisabledOverrideCount > 0 ? m_DisabledOverrideCount - 1 : 0;
		}
		else
		{
			m_DisabledOverrideCount++;
		}
		bool bNewActiveState = IsActive();

		if (m_Archetype != nullptr && bOldActiveState != bNewActiveState)
		{
			m_Archetype->SetEntityActiveState(m_IndexInArchetype, bNewActiveState);
		}
	}

	void EntityData::SetDisabledOverrideCount(uint32_t disableOverrideCount)
	{
		bool bOldActiveState = IsActive();
		m_DisabledOverrideCount = disableOverrideCount;
		bool bNewActiveState = IsActive();

		if (m_Archetype != nullptr && bOldActiveState != bNewActiveState)
		{
			m_Archetype->SetEntityActiveState(m_IndexInArchetype, bNewActiveState);
		}
	}

	void EntityData::ResetDisableOverrideCount()
	{
		SetDisabledOverrideCount(0);
	}

	void EntityData::OnDestroyByEntityManager()
	{
		m_Archetype = nullptr;
		m_Version += 1;
		m_State = EEntityState::Dead;
	}

}