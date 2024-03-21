#pragma once
#include "Core.h"

#include "Container.h"
#include "Entity.h"

namespace decs
{
	class ContainerSerializerComplex
	{
	public:
		void Serialize(decs::Container& container)
		{
			auto& archetypesMap = container.m_ArchetypesMap;
			auto& archetypesVector = container.m_ArchetypesMap.m_Archetypes;

			uint64_t archetypesChunks = archetypesVector.ChunkCount();

			decs::Entity entityBuffer = {};

			for (uint32_t i = 0; i < container.m_EmptyEntities.Size(); i++)
			{
				entityBuffer.Set(container.m_EmptyEntities[i], &container);

				// entity serialize:
			}

			for (uint64_t chunkIdx = 0; chunkIdx < archetypesChunks; chunkIdx++)
			{
				uint64_t archetypeCountInChunk = archetypesVector.GetChunkSize(chunkIdx);
				auto chunk = archetypesVector.GetChunk(chunkIdx);

				for (uint64_t archetypeIdx = 0; archetypeIdx < archetypeCountInChunk; archetypeIdx++)
				{
					Archetype& archetype = chunk[archetypeIdx];

					if (BeginArchetypeSerialize(archetype))
					{
						uint64_t entitesCount = archetype.EntityCount();
						if (entitesCount > 0)
						{
							uint64_t componentCount = archetype.ComponentCount();
							for (uint64_t entityIdx = 0; entityIdx < entitesCount; entityIdx++)
							{
								entityBuffer.Set(archetype.m_EntitiesData[entityIdx].m_EntityData, &container);
								if (BeginEntitySerialize(entityBuffer))
								{
									for (uint64_t componentIdx = 0; componentIdx < componentCount; componentIdx++)
									{
										const auto& archetypeComponentData = archetype.m_TypeData[componentIdx];

										SerializeComponent(
											archetypeComponentData.m_PackedContainer->GetComponentPtrAsVoid(entityIdx),
											archetypeComponentData.m_PackedContainer->GetComponentSize(),
											archetypeComponentData.m_TypeID,
											componentIdx
										);
									}
									EndEntitySerialize(entityBuffer);
								}
							}
						}
						EndArchetypeSerialize(archetype);
					}
				}
			}
		}

	protected:
		virtual bool BeginArchetypeSerialize(const Archetype& archetype) = 0;

		virtual void EndArchetypeSerialize(const Archetype& archetype) = 0;

		/// <summary>
		/// If return false subsequent methods of entity and its components serialization will not be invoked, if returns true, subsequent serialization methods of entity and its components will be invoked.
		/// </summary>
		/// <param name="entity"></param>
		/// <returns></returns>
		virtual bool BeginEntitySerialize(const Entity& entity) = 0;

		virtual void EndEntitySerialize(const Entity& entity) = 0;

		virtual void SerializeComponent(void* component, uint64_t componentSize, TypeID componentTypeID, uint64_t componentIndexInArchetype) = 0;

	};
}