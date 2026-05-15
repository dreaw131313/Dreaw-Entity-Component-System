#include "LArchetypeAllocator.h"

namespace decs::light
{
	ArchetypeAllocator::ArchetypeAllocator(FilterManager& filterManager, uint32_t chunkSize):
		m_FilterManager(filterManager),
		m_Archetypes(chunkSize)
	{

	}

	Archetype* ArchetypeAllocator::CreateArchetype()
	{
		Archetype* newArchetype = nullptr;

		if (!m_FreeList.empty())
		{
			newArchetype = m_FreeList.back();
			m_FreeList.pop_back();
		}
		else
		{
			newArchetype = &m_Archetypes.EmplaceBack();
		}

		newArchetype->m_CreatedIndexInAllocator = m_Created.size();
		m_Created.push_back(newArchetype);

		return newArchetype;
	}

	bool ArchetypeAllocator::Destroy(Archetype* archetype)
	{
		if (archetype == nullptr
			|| archetype->m_CreatedIndexInAllocator >= m_Created.size())
		{
			return false;
		}

		size_t createdIndex = archetype->m_CreatedIndexInAllocator;
		if (createdIndex < (m_Created.size() - 1))
		{
			auto lastCreatedArchetype = m_Created.back();
			m_Created[createdIndex] = lastCreatedArchetype;
			lastCreatedArchetype->m_CreatedIndexInAllocator = createdIndex;
		}
		m_Created.pop_back();

		OnDestroyArchetype(*archetype);
		archetype->ResetOnDestroy(m_FilterManager);

		m_FreeList.push_back(archetype);

		return true;
	}

	void ArchetypeAllocator::OnDestroyArchetype(Archetype& archetype)
	{
		archetype.m_CreatedIndexInAllocator = std::numeric_limits<size_t>::max();
		archetype.m_Version++;
	}

}
