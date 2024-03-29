#pragma once
#include "Core.h"
#include "Type.h"

#include "Container.h"
#include "Entity.h"


namespace decs
{

	template<typename SerializerData>
	class ComponentSerializerBase
	{
		template<typename>
		friend class ContainerSerializer;

	public:
		inline virtual TypeID GetComponentTypeID() const = 0;

		inline virtual std::string GetComponentTypeName() const = 0;

		inline virtual std::string GetComponentTypeNameWithoutNamespace() const = 0;

	protected:
		virtual void SerializeComponentFromVoid(void* component, SerializerData& serializerData) const = 0;

	};

	template<typename ComponentType, typename SerializerData>
	class ComponentSerializer : ComponentSerializerBase<SerializerData>
	{
		template<typename>
		friend class ContainerSerializer;
	public:
		virtual void SerializeComponent(const typename component_type<ComponentType>::Type& component, SerializerData& serializerData) const = 0;

		inline virtual TypeID GetComponentTypeID() const override final
		{
			return Type<ComponentType>::ID();
		}

		inline virtual std::string GetComponentTypeName() const override final
		{
			return Type<ComponentType>::Name();
		}

		inline virtual std::string GetComponentTypeNameWithoutNamespace() const override final
		{
			return Type<ComponentType>::NameWithoutNamespace();
		}

	private:
		virtual void SerializeComponentFromVoid(void* component, SerializerData& serializerData) const override final
		{
			SerializeComponent(*static_cast<typename component_type<ComponentType>::Type*>(component), serializerData);
		}
	};

	template<typename SerializerData>
	class ContainerSerializer
	{
	private:
		struct ComponentSerializationData
		{
		public:
			const ComponentSerializerBase<SerializerData>* m_Serializer = nullptr;
			PackedContainerBase* m_PackedContainer = nullptr;
		};

	public:
		ContainerSerializer()
		{

		}

		~ContainerSerializer()
		{

		}

		template<typename ComponentType>
		void SetComponentSerializer(ComponentSerializer<ComponentType, SerializerData>* serializer)
		{
			TYPE_ID_CONSTEXPR TypeID id = Type<ComponentType>::ID();
			m_ComponentSerializers[id] = serializer;
		}

		void SetComponentSerializer(ComponentSerializerBase<SerializerData>* serializer)
		{
			m_ComponentSerializers[serializer->GetComponentTypeID()] = serializer;
		}

		void Serialize(Container& container, SerializerData& serializerData)
		{
			std::vector<ComponentSerializationData> componentSerializersData;

			auto& archetypesMap = container.m_ArchetypesMap;
			auto& archetypesVector = container.m_ArchetypesMap.m_Archetypes;

			uint64_t archetypesChunks = archetypesVector.ChunkCount();

			decs::Entity entityBuffer = {};

			for (uint32_t i = 0; i < container.m_EmptyEntities.Size(); i++)
			{
				entityBuffer.Set(container.m_EmptyEntities[i], &container);
				if (BeginEntitySerialize(entityBuffer, serializerData))
				{
					EndEntitySerialize(entityBuffer, serializerData);
				}
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
						GetComponentSerializers(archetype, componentSerializersData);
						uint64_t componentCount = componentSerializersData.size();

						for (uint64_t entityIdx = 0; entityIdx < entitesCount; entityIdx++)
						{
							entityBuffer.Set(archetype.m_EntitiesData[entityIdx].m_EntityData, &container);
							if (BeginEntitySerialize(entityBuffer, serializerData))
							{
								for (uint64_t componentIdx = 0; componentIdx < componentCount; componentIdx++)
								{
									auto& componentSerializerData = componentSerializersData[componentIdx];
									BeginComponentSerialize(entityBuffer, componentSerializerData.m_Serializer, serializerData);
									{
										componentSerializerData.m_Serializer->SerializeComponentFromVoid(
											componentSerializerData.m_PackedContainer->GetComponentPtrAsVoid(entityIdx),
											serializerData
										);
									}
									EndComponentSerialize(entityBuffer, componentSerializerData.m_Serializer, serializerData);
								}
								EndEntitySerialize(entityBuffer, serializerData);
							}
						}
					}
				}
			}
		}

	protected:
		/// <summary>
		/// If return false subsequent methods of entity and its components serialization will not be invoked, if returns true, subsequent serialization methods of entity and its components will be invoked.
		/// </summary>
		/// <param name="entity"></param>
		/// <returns></returns>
		virtual bool BeginEntitySerialize(const Entity& entity, SerializerData& serializerData) = 0;

		virtual void EndEntitySerialize(const Entity& entity, SerializerData& serializerData) = 0;

		virtual void BeginComponentSerialize(const Entity& entity, const ComponentSerializerBase<SerializerData>* componentSerializer, SerializerData& serializerData) = 0;

		virtual void EndComponentSerialize(const Entity& entity, const ComponentSerializerBase<SerializerData>* componentSerializer, SerializerData& serializerData) = 0;

	private:
		ecsMap<TypeID, const ComponentSerializerBase<SerializerData>*> m_ComponentSerializers = {};

	private:
		void GetComponentSerializers(Archetype& archetype, std::vector<ComponentSerializationData>& serializers)
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
	};
}