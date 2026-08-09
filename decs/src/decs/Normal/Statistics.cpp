#include "Statistics.h"

#include "decs/Normal/Archetypes/Archetype.h"
#include "decs/Normal/Container.h"

namespace decs
{
	void decs::ArchetypeStatistics::Collect(const Archetype& archetype)
	{
		m_EntityCount = archetype.EntityCount();
		m_Capacity = archetype.GetCapacity();
		m_ComponentCount = archetype.GetComponentOnlyCount();
		m_TagCount = archetype.GetTagOnlyCount();
		m_NeighbourCount = archetype.GetEdgeCount();

		m_ComponentTypeIDs.reserve(m_ComponentCount);
		m_TagTypeIDs.reserve(m_TagCount);

		for (size_t i = 0; i < static_cast<size_t>(archetype.GetComponentAndTagCount()); i++)
		{
			TypeID typeID = archetype.GetTypeID(i);
			if (archetype.IsTypeTag(static_cast<size_t>(i)))
			{
				m_TagTypeIDs.push_back(typeID);
			}
			else
			{
				m_ComponentTypeIDs.push_back(typeID);
			}
		}
	}

	void ContainerStatistics::Collect(const Container& container)
	{
		m_EntityDataChunkSize = container.m_EntityManager.GetEntityDataChunkSize();
		m_CreatedEntityCount = container.GetEntityCount();
		m_EntityCapacity = container.m_EntityManager.GetCapacity();
		m_EmptyEntityCount = container.GetEmptyEntitiesCount();

		m_EntityCreateListenerCount = container.m_CreateEntityObservers.Size();
		m_EntityDestroyListenerCount = container.m_DestroyEntityObservers.Size();
		m_EntityEnableListenerCount = container.m_EnableEntityObservers.Size();
		m_EntityDisableListenerCount = container.m_DisableEntityObservers.Size();

		container.m_ComponentContextManager.FillStatistics(m_ComponentsStats);

		// ARCHETYPES:
		{
			size_t archetypeIdx = 0;
			m_ArchetypesStats.resize(container.m_ArchetypesMap.GetArchetypesCount());
			container.m_ArchetypesMap.IterateOverArchetypes_Forward([&] (const Archetype& archetype)
			{
				m_ArchetypesStats[archetypeIdx].Collect(archetype);
				archetypeIdx++;
			});
		}
	}
}