#pragma once

#include "decs/Core/Core.h"


namespace decs
{
	class Archetype;
	class Container;

	class ComponentStatistics
	{
	public:
		TypeID m_ComponentTypeID = InvalidTypeID;
		size_t m_CreatedComponentCount = 0;
		size_t m_ChunkCapacity = 0;
		size_t m_AllocatedChunks = 0;
		size_t m_AllocatorCapacity = 0;
		size_t m_CreateListenerCount = 0;
		size_t m_DestroyListenerCount = 0;
		size_t m_EnableListenerCount = 0;
		size_t m_DisableListenerCount = 0;
		size_t m_ComponentSize = 0;
		size_t m_ComponentAlignment = 0;

	public:
		ComponentStatistics() = default;
		~ComponentStatistics() = default;
	};

	class ArchetypeStatistics
	{
	public:
		std::vector<TypeID> m_ComponentTypeIDs{};
		std::vector<TypeID> m_TagTypeIDs{};

		size_t m_EntityCount = 0;
		size_t m_Capacity = 0;
		size_t m_ComponentCount = 0;
		size_t m_TagCount = 0;
		size_t m_NeighbourCount = 0;

	public:
		ArchetypeStatistics() = default;
		~ArchetypeStatistics() = default;

		void Collect(const Archetype& archetype);

	};

	class ContainerStatistics
	{
	public:
		std::vector<ComponentStatistics> m_ComponentsStats{};
		std::vector<ArchetypeStatistics> m_ArchetypesStats{};
		size_t m_EntityDataChunkSize = 0;
		size_t m_CreatedEntityCount = 0;
		size_t m_EntityCapacity = 0;
		size_t m_EmptyEntityCount = 0;
		size_t m_EntityCreateListenerCount = 0;
		size_t m_EntityDestroyListenerCount = 0;
		size_t m_EntityEnableListenerCount = 0;
		size_t m_EntityDisableListenerCount = 0;

	public:
		ContainerStatistics() = default;
		~ContainerStatistics() = default;

		void Collect(const Container& container);
	};

}
