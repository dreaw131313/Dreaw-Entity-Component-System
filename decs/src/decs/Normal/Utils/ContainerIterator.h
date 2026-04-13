#pragma once
#include "decs/Normal/Container.h"
#include "decs/Normal/Entity.h"

namespace decs
{
	/// <summary>
	/// Class for iterating over container in the same order like ContainerSerializer.
	/// </summary>
	class ContainerIterator final
	{
	public:
		template<container_iterator_entity_func TCallable>
		void Foreach(Container& container, TCallable&& entityFunc) const
		{
			auto& archetypesMap = container.m_ArchetypesMap;
			auto& archetypesVector = container.m_ArchetypesMap.m_Archetypes;

			uint64_t archetypesChunks = archetypesVector.ChunkCount();

			decs::Entity entityBuffer = {};

			for (uint32_t i = 0; i < container.m_EmptyEntities.size(); i++)
			{
				entityBuffer.Set_Internal(*container.m_EmptyEntities[i]);
				entityFunc(entityBuffer);
			}

			for (uint64_t chunkIdx = 0; chunkIdx < archetypesChunks; chunkIdx++)
			{
				uint64_t elementsCount = archetypesVector.GetChunkSize(chunkIdx);

				auto chunk = archetypesVector.GetChunk(chunkIdx);

				for (uint64_t archetypeIdx = 0; archetypeIdx < elementsCount; archetypeIdx++)
				{
					Archetype& archetype = chunk[archetypeIdx];
					uint64_t entitesCount = archetype.EntityCount();
					if (entitesCount > 0)
					{
						for (uint64_t entityIdx = 0; entityIdx < entitesCount; entityIdx++)
						{
							auto& archetypeEntityData = archetype.m_EntitiesData[entityIdx];
							if (archetypeEntityData.IsValid())
							{
								entityBuffer.Set_Internal(*archetypeEntityData.m_EntityData);
								entityFunc(entityBuffer);
							}
						}
					}
				}
			}
		}

		/// <summary>
		/// Iterates over entities, and archetypes.First calls ArchetypeFunc with nullptr archetype (for entitieis without archetype). Then iterate over entities without archetype. Then iterate over entities in each archetype. Before iterating over entities in each archetype, calls ArchetypeFunc.
		/// </summary>
		/// <typeparam name="EntityFunc"></typeparam>
		/// <typeparam name="ArchetypeFunc"></typeparam>
		template<
			container_iterator_entity_func EntityFunc,
			container_iterator_archetype_func ArchetypeFunc
		>
		void ForEach(Container& container, EntityFunc&& entityFunc, ArchetypeFunc&& archetypeFunc)
		{
			auto& archetypesMap = container.m_ArchetypesMap;
			auto& archetypesVector = container.m_ArchetypesMap.m_Archetypes;

			uint64_t archetypesChunks = archetypesVector.ChunkCount();

			archetypeFunc(nullptr);
			decs::Entity entityBuffer = {};

			for (uint32_t i = 0; i < container.m_EmptyEntities.size(); i++)
			{
				entityBuffer.Set_Internal(*container.m_EmptyEntities[i]);
				entityFunc(entityBuffer);
			}

			for (uint64_t chunkIdx = 0; chunkIdx < archetypesChunks; chunkIdx++)
			{
				uint64_t elementsCount = archetypesVector.GetChunkSize(chunkIdx);

				auto chunk = archetypesVector.GetChunk(chunkIdx);

				for (uint64_t archetypeIdx = 0; archetypeIdx < elementsCount; archetypeIdx++)
				{
					Archetype& archetype = chunk[archetypeIdx];
					archetypeFunc(&archetype);

					uint64_t entitesCount = archetype.EntityCount();
					if (entitesCount > 0)
					{
						for (uint64_t entityIdx = 0; entityIdx < entitesCount; entityIdx++)
						{
							auto& archetypeEntityData = archetype.m_EntitiesData[entityIdx];
							if (archetypeEntityData.IsValid())
							{
								entityBuffer.Set_Internal(*archetypeEntityData.m_EntityData);
								entityFunc(entityBuffer);
							}
						}
					}
				}
			}
		}

	};
}
