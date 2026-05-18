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

	void EntityData::OnDestroyByEntityManager()
	{
		m_Container = nullptr;
		m_Archetype = nullptr;
		m_Version += 1;
		m_bIsAlive = false;
		m_bOperationLocked = true;
	}

	void EntityData::OnCreateEntityByEntityManager(Container& container)
	{ 
		m_Container = &container;
		m_bIsAlive = true;
		m_bOperationLocked = false;
	}

}