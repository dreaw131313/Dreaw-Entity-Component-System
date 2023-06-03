#pragma once
#include "Core.h"
#include "Type.h"

#include "Container.h"
#include "Entity.h"


namespace decs
{

	class ComponentSerializerBase
	{
		friend class ContainerSerializer;
	protected:
		virtual void SerializeComponentFromVoid(void* component, void* serializerData) = 0;
	};

	template<typename ComponentType>
	class ComponentSerializer : ComponentSerializerBase
	{
		friend class ContainerSerializer;
	public:
		virtual void SerializeComponent(const typename component_type<ComponentType>::Type& component, void* serializerData) = 0;

	private:
		virtual void SerializeComponentFromVoid(void* component, void* serializerData) override
		{
			SerializeComponent(*reinterpret_cast<typename component_type<ComponentType>::Type*>(component), serializerData);
		}
	};

	struct ComponentSerializationData
	{
	public:
		ComponentSerializerBase* m_Serializer = nullptr;
		PackedContainerBase* m_PackedContainer = nullptr;
	};

	class ContainerSerializer
	{
	public:
		ContainerSerializer();

		~ContainerSerializer();

		template<typename ComponentType>
		void SetComponentSerializer(ComponentSerializer<ComponentType>* serializer)
		{
			TYPE_ID_CONSTEXPR TypeID id = Type<ComponentType>::ID();
			m_ComponentSerializers[id] = serializer;
		}

		void Serialize(Container& container, void* serializerData = nullptr);

	protected:
		virtual void OnBeginSerialization(Container& container);

		virtual void OnEndSerialization(Container& container);

		/// <summary>
		/// If return false subsequent methods of entity and its components serialization will not be invoked, if returns true, subsequent serialization methods of entity and its components will be invoked.
		/// </summary>
		/// <param name="entity"></param>
		/// <returns></returns>
		virtual bool BeginEntitySerialize(const Entity& entity) = 0;

		virtual void EndEntitySerialize(const Entity& entity) = 0;

	private:
		ecsMap<TypeID, ComponentSerializerBase*> m_ComponentSerializers = {};

	private:
		void GetComponentSerializers(Archetype& archetype, std::vector<ComponentSerializationData>& serializers);
	};
}