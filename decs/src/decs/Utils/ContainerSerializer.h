#pragma once
#include "Core.h"
#include "Type.h"

#include "Container.h"
#include "Entity.h"

#include "Component/Component.h"


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

	protected:
		virtual void SerializeComponentFromVoid(ComponentBase* component, SerializerData& serializerData) const = 0;

	};

	template<typename TComponent, typename SerializerData>
	class ComponentSerializer : ComponentSerializerBase<SerializerData>
	{
		template<typename>
		friend class ContainerSerializer;
	public:
		virtual void SerializeComponent(const TComponent& component, SerializerData& serializerData) const = 0;

		inline virtual TypeID GetComponentTypeID() const override final
		{
			return Type<TComponent>::ID();
		}

		inline virtual std::string GetComponentTypeName() const override final
		{
			return Type<TComponent>::Name();
		}


	private:
		virtual void SerializeComponentFromVoid(ComponentBase* component, SerializerData& serializerData) const override final
		{
			SerializeComponent(*static_cast<TComponent*>(component), serializerData);
		}
	};

	template<typename SerializerData>
	class TagSerializerBase
	{
	public:
		inline virtual TypeID GetTagTypeID() const = 0;

		inline virtual std::string GetComponentTypeName() const = 0;

		virtual void SerializeTag(SerializerData& serializerData) const = 0;
	};

	template<typename TTag, typename SerializerData>
	class TagSerializer : ComponentSerializerBase<SerializerData>
	{
	public:
		inline virtual TypeID GetComponentTypeID() const override final
		{
			return Type<TTag>::ID();
		}

		inline virtual std::string GetComponentTypeName() const override final
		{
			return Type<TTag>::Name();
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
		struct TagSerializationData
		{
		public:
			const TagSerializerBase<SerializerData>* m_Serializer = nullptr;
		};

	public:
		ContainerSerializer()
		{

		}

		~ContainerSerializer()
		{

		}

		template<typename TComponent>
		void SetComponentSerializer(ComponentSerializer<TComponent, SerializerData>* serializer)
		{
			TYPE_ID_CONSTEXPR TypeID id = Type<TComponent>::ID();
			m_ComponentSerializers[id] = serializer;
		}

		void SetComponentSerializer(ComponentSerializerBase<SerializerData>* serializer)
		{
			m_ComponentSerializers[serializer->GetComponentTypeID()] = serializer;
		}

		void Serialize(Container& container, SerializerData& serializerData)
		{
			std::vector<ComponentSerializationData> componentSerializersData;
			std::vector<TagSerializationData> tagSerializersData;

			auto& archetypesMap = container.m_ArchetypesMap;
			auto& archetypesVector = container.m_ArchetypesMap.m_Archetypes;

			uint64_t archetypesChunks = archetypesVector.ChunkCount();

			decs::Entity entityBuffer = {};

			for (uint32_t i = 0; i < container.m_EmptyEntities.size(); i++)
			{
				entityBuffer.Set(container.m_EmptyEntities[i]);
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
						GetTagSerializers(archetype, tagSerializersData);

						uint64_t componentCount = componentSerializersData.size();
						uint64_t tagCount = tagSerializersData.size();

						for (uint64_t entityIdx = 0; entityIdx < entitesCount; entityIdx++)
						{
							auto& archetypeEntityData = archetype.m_EntitiesData[entityIdx];
							if (archetypeEntityData.IsValid())
							{
								entityBuffer.Set(archetypeEntityData.m_EntityData);
								if (BeginEntitySerialize(entityBuffer, serializerData))
								{
									for (uint64_t tagIdx = 0; tagIdx < tagCount; tagIdx++)
									{
										TagSerializationData& tagSerializationData = tagSerializersData[tagIdx];
										BeginTagSerialize(entityBuffer, tagSerializationData.m_Serializer, serializerData);
										{
											tagSerializationData.m_Serializer->SerializeTag(serializerData);
										}
										EndTagSerialize(entityBuffer, tagSerializationData.m_Serializer, serializerData);
									}

									for (uint64_t componentIdx = 0; componentIdx < componentCount; componentIdx++)
									{
										ComponentSerializationData& componentSerializerData = componentSerializersData[componentIdx];
										BeginComponentSerialize(entityBuffer, componentSerializerData.m_Serializer, serializerData);
										{
											componentSerializerData.m_Serializer->SerializeComponentFromVoid(
												componentSerializerData.m_PackedContainer->GetComponentBasePtr(entityIdx),
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

		virtual void BeginTagSerialize(const Entity& entity, const TagSerializerBase<SerializerData>* componentSerializer, SerializerData& serializerData) = 0;

		virtual void EndTagSerialize(const Entity& entity, const TagSerializerBase<SerializerData>* componentSerializer, SerializerData& serializerData) = 0;

	private:
		ecsMap<TypeID, const ComponentSerializerBase<SerializerData>*> m_ComponentSerializers = {};
		ecsMap<TypeID, const TagSerializerBase<SerializerData>*> m_TagSerializers = {};

	private:
		void GetComponentSerializers(Archetype& archetype, std::vector<ComponentSerializationData>& serializers)
		{
			serializers.clear();

			for (uint32_t i = 0; i < archetype.GetComponentAndTagCount(); i++)
			{
				TypeID componentType = archetype.GetTypeID(i);
				if (!archetype.IsTypeTag(i))
				{
					auto it = m_ComponentSerializers.find(componentType);
					if (it != m_ComponentSerializers.end())
					{
						serializers.push_back({ it->second, archetype.GetPackedContainerAt(i) });
					}
				}
			}
		}
		void GetTagSerializers(Archetype& archetype, std::vector<TagSerializationData>& serializers)
		{
			serializers.clear();

			for (uint32_t i = 0; i < archetype.GetComponentAndTagCount(); i++)
			{
				TypeID componentType = archetype.GetTypeID(i);
				if (archetype.IsTypeTag(i))
				{
					auto it = m_TagSerializers.find(componentType);
					if (it != m_TagSerializers.end())
					{
						serializers.push_back({ it->second });
					}
				}
			}
		}
	};
}