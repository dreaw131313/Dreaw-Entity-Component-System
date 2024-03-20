#pragma once
#include "Core.h"
#include "Container.h"
#include "Entity.h"

namespace decs
{
	/// <summary>
	/// Class for iterating over container in the same order like ContainerSerializer.
	/// </summary>
	class ContainerIterator final
	{
	public:
		template<typename TCallable>
		void Foreach(Container& container, TCallable&& callable) const
		{
			auto& archetypesMap = container.m_ArchetypesMap;
			auto& archetypesVector = container.m_ArchetypesMap.m_Archetypes;

			uint64_t archetypesChunks = archetypesVector.ChunkCount();

			decs::Entity entityBuffer = {};

			for (uint32_t i = 0; i < container.m_EmptyEntities.Size(); i++)
			{
				entityBuffer.Set(container.m_EmptyEntities[i], &container);
				callable(entityBuffer);
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
							entityBuffer.Set(archetype.m_EntitiesData[entityIdx].m_EntityData, &container);
							callable(entityBuffer);
						}
					}
				}
			}
		}
	};
}
