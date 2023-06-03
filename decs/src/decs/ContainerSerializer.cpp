#include "ContainerSerializer.h"
#include "ContainerSerializer.h"

namespace decs
{
	ContainerSerializer::ContainerSerializer()
	{
	}

	ContainerSerializer::~ContainerSerializer()
	{
	}

	void ContainerSerializer::Serialize(Container& container, void* serializerData)
	{
		OnBeginSerialization(container);
		{
			std::vector<ComponentSerializationData> componentSerializersData;

			auto& archetypesMap = container.m_ArchetypesMap;
			auto& archetypesVector = container.m_ArchetypesMap.m_Archetypes;

			uint64_t archetypesChunks = archetypesVector.ChunkCount();

			decs::Entity entityBuffer = {};

			for (uint64_t chunkIdx = 0; chunkIdx < archetypesChunks; chunkIdx++)
			{
				uint64_t elementsCount = archetypesVector.GetChunkSize(chunkIdx);

				auto chunk = archetypesVector.GetChunk(chunkIdx);

				for (uint64_t archetypeIdx = 0; archetypeIdx < elementsCount; archetypeIdx++)
				{
					Archetype& archetype = chunk[archetypeIdx];
					GetComponentSerializers(archetype, componentSerializersData);
					uint64_t componentCount = componentSerializersData.size();

					uint64_t entitesCount = archetype.EntityCount();
					for (uint64_t entityIdx = 0; entityIdx < entitesCount; entityIdx++)
					{
						entityBuffer.Set(archetype.m_EntitiesData[entityIdx].m_EntityData, &container);
						if (BeginEntitySerialize(entityBuffer))
						{
							for (uint64_t componentIdx = 0; componentIdx < componentCount; componentIdx++)
							{
								auto& componentSerializerData = componentSerializersData[componentIdx];

								componentSerializerData.m_Serializer->SerializeComponentFromVoid(
									componentSerializerData.m_PackedContainer->GetComponentPtrAsVoid(entityIdx), 
									serializerData
								);
							}
							EndEntitySerialize(entityBuffer);
						}
					}
				}
			}
		}
		OnEndSerialization(container);
	}

	void ContainerSerializer::OnBeginSerialization(Container& container)
	{
	}

	void ContainerSerializer::OnEndSerialization(Container& container)
	{
	}

	void ContainerSerializer::GetComponentSerializers(Archetype& archetype, std::vector<ComponentSerializationData>& serializers)
	{
		serializers.clear();

		for (uint32_t i = 0; i < archetype.ComponentCount(); i++)
		{
			TypeID componentType = archetype.GetTypeID(i);
			auto it = m_ComponentSerializers.find(componentType);
			if (it != m_ComponentSerializers.end())
			{
				serializers.push_back({ it->second, archetype.GetPackedContainerAt(i) });
			}
		}
	}
}