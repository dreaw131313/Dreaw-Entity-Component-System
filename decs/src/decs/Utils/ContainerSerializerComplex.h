#pragma once
#include "Core.h"

#include "Container.h"
#include "Entity.h"

#include "Component/Component.h"

namespace decs
{
	/// <summary>
	/// Tag serialization callback is ommited because tags can be serialized per archetype instead of per entity. If tags need to be serialized per entity it can be done in BeginArchetypeSerialize
	/// </summary>
	class ContainerSerializerComplex
	{
	public:
		void Serialize(decs::Container& container)
		{
			auto& archetypesMap = container.m_ArchetypesMap;
			auto& archetypesVector = container.m_ArchetypesMap.m_Archetypes;

			uint64_t archetypesChunks = archetypesVector.ChunkCount();

			decs::Entity entityBuffer = {};
			//std::vector<TypeID> entityTagsIDs{};

			for (uint32_t i = 0; i < container.m_EmptyEntities.size(); i++)
			{
				entityBuffer.Set_Internal(*container.m_EmptyEntities[i]);

				if (BeginEntitySerialize(entityBuffer))
				{
					EndEntitySerialize(entityBuffer);
				}
			}

			for (uint64_t chunkIdx = 0; chunkIdx < archetypesChunks; chunkIdx++)
			{
				uint64_t archetypeCountInChunk = archetypesVector.GetChunkSize(chunkIdx);
				auto chunk = archetypesVector.GetChunk(chunkIdx);

				for (uint64_t archetypeIdx = 0; archetypeIdx < archetypeCountInChunk; archetypeIdx++)
				{
					Archetype& archetype = chunk[archetypeIdx];

					//FetchTagsTypeIDsFromArchetype(archetype, entityTagsIDs);

					if (BeginArchetypeSerialize(archetype))
					{
						uint64_t entitesCount = archetype.EntityCount();
						if (entitesCount > 0)
						{
							uint64_t componentCount = archetype.GetComponentAndTagCount();
							for (uint64_t entityIdx = 0; entityIdx < entitesCount; entityIdx++)
							{
								entityBuffer.Set_Internal(*archetype.m_EntitiesData[entityIdx].m_EntityData);
								if (BeginEntitySerialize(entityBuffer))
								{
									for (uint64_t componentIdx = 0; componentIdx < componentCount; componentIdx++)
									{
										const auto& archetypeComponentData = archetype.m_TypeData[componentIdx];
										if (!archetypeComponentData.IsTag())
										{
											SerializeComponent(
												entityBuffer,
												archetypeComponentData.m_PackedContainer->GetComponentBasePtr(entityIdx),
												archetypeComponentData.m_PackedContainer->GetComponentSize(),
												archetypeComponentData.m_TypeID,
												componentIdx
											);
										}
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
		void FetchTagsTypeIDsFromArchetype(const Archetype& archetype, std::vector<TypeID>& entityTagsID)
		{
			entityTagsID.clear();
			const uint32_t componentCount = archetype.GetComponentAndTagCount();
			for (uint32_t i = 0; i < componentCount; i++)
			{
				if (archetype.IsTypeTag(i))
				{
					entityTagsID.push_back(archetype.GetTypeID(i));
				}
			}
		}

		virtual bool BeginArchetypeSerialize(const Archetype& archetype) = 0;

		virtual void EndArchetypeSerialize(const Archetype& archetype) = 0;

		/// <summary>
		/// If return false subsequent methods of entity and its components serialization will not be invoked, if returns true, subsequent serialization methods of entity and its components will be invoked.
		/// </summary>
		/// <param name="entity"></param>
		/// <returns></returns>
		virtual bool BeginEntitySerialize(const Entity& entity) = 0;

		virtual void EndEntitySerialize(const Entity& entity) = 0;

		virtual void SerializeComponent(const Entity& entity, EntityComponent* component, uint64_t componentSize, TypeID componentTypeID, uint64_t componentIndexInArchetype) = 0;


	};
}