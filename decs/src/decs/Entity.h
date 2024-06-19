#pragma once
#include "Core.h"
#include "Container.h"
#include "Type.h"
#include "Archetypes/Archetype.h"

namespace decs
{
	class Container;

	class Entity final
	{
		template<typename ...>
		friend class Query;
		template<typename ...>
		friend class MultiQuery;
		friend class Container;
		template<typename TComponent>
		friend class ComponentRef;
		friend class ComponentBaseRef;
		template<typename>
		friend class ContainerSerializer;
		friend class ContainerSerializerComplex;
		friend class ContainerIterator;

		friend class ConstEntity;

		friend struct std::hash<decs::Entity>;

	public:
		Entity()
		{

		}

		Entity(EntityID id, Container* container) :
			m_ID(id),
			m_Container(container),
			m_EntityData(&container->m_EntityManager->GetEntityData(id)),
			m_Version(m_EntityData->GetVersion())
		{

		}

		Entity(EntityData* entityData, Container* container) :
			m_ID(entityData->GetID()),
			m_Container(container),
			m_EntityData(entityData),
			m_Version(entityData->GetVersion())
		{

		}

	public:
		bool operator==(const Entity& rhs)const
		{
			return this->m_EntityData == rhs.m_EntityData && this->m_Version == rhs.m_Version;
		}

		inline EntityID GetID() const
		{
			return m_ID;
		}

		inline Container* GetContainer() const
		{
			return m_Container;
		}

		inline bool IsValid() const
		{
			return m_EntityData != nullptr && m_Version == m_EntityData->GetVersion() && m_EntityData->IsAlive();
		}

		inline bool IsNull() const
		{
			return !IsValid();
		}

		inline bool IsActive() const
		{
			return IsValid() && m_EntityData->IsActive();
		}

		inline void SetActive(const bool& isActive) const
		{
			if (IsValid())
				m_Container->SetEntityActive(*this, isActive);
		}

		inline bool Destroy() const
		{
			if (IsValid())
			{
				m_Container->DestroyEntityInternal(*this);
				Invalidate();
				return true;
			}
			return false;
		}

		template<typename TComponent>
		inline TComponent* GetComponent() const
		{
			if (IsValid())
				return m_Container->GetComponent<TComponent>(*m_EntityData);

			return nullptr;
		}

		/// <summary>
		/// Iterates over all components on entity and use dynamic cast. If casted component is not nullptr returns it. If none of componets can be casted to TComponent returns nullptr.
		/// </summary>
		/// <typeparam name="TComponent"></typeparam>
		/// <returns></returns>
		template<typename TComponent>
		inline TComponent* GetComponentDynamic() const
		{
			if (IsValid())
				return m_Container->GetComponentDynamic<TComponent>(*m_EntityData);

			return nullptr;
		}

		template<typename TComponent>
		inline bool HasComponent() const
		{
			return IsValid() && m_Container->HasComponent<TComponent>(*m_EntityData);
		}

		template<typename TComponent>
		inline bool TryGetComponent(TComponent*& component) const
		{
			if (IsValid())
				component = m_Container->GetComponent<TComponent>(*m_EntityData);
			else
				component = nullptr;

			return component != nullptr;
		}

		template<typename TComponent, typename... Args>
		inline typename TComponent* AddComponent(Args&&... args) const
		{
			if (IsValid())
				return m_Container->AddComponent<TComponent>(*this, *m_EntityData, std::forward<Args>(args)...);

			return nullptr;
		}

		template<typename TComponent>
		inline bool RemoveComponent() const
		{
			return IsValid() && m_Container->RemoveComponent<TComponent>(*this);
		}

		inline bool RemoveComponent(TypeID componentTypeID) const
		{
			return IsValid() && m_Container->RemoveComponent(*this, componentTypeID);
		}

		/*template<typename... Ts>
		inline uint32_t RemoveComponents() const
		{
			if (IsValid())
			{
				return m_Container->RemoveMultipleComponnets<Ts...>(*this, *m_EntityData);
			}
			return 0;
		}*/

		inline EntityVersion GetVersion() const
		{
			return m_Version;
		}

		inline uint32_t ComponentCount() const
		{
			if (IsValid())
				return m_EntityData->ComponentCount();
			return 0;
		}

		inline const Archetype* GetArchetype() const
		{
			if (IsValid())
			{
				return m_EntityData->m_Archetype;
			}

			return nullptr;
		}

	private:
		mutable Container* m_Container = nullptr;
		mutable EntityData* m_EntityData = nullptr;
		mutable EntityID m_ID = std::numeric_limits<EntityID>::max();
		mutable EntityVersion m_Version = std::numeric_limits<EntityVersion>::max();

	private:
		inline void Set(EntityID id, Container* container)
		{
			m_ID = id;
			m_Container = container;
			m_EntityData = &container->m_EntityManager->GetEntityData(id);
			m_Version = m_EntityData->GetVersion();
		}

		inline void Set(EntityData& data, Container* container)
		{
			m_ID = data.GetID();
			m_Container = container;
			m_EntityData = &data;
			m_Version = data.GetVersion();
		}

		inline void Set(EntityData* data, Container* container)
		{
			m_ID = data->GetID();
			m_Container = container;
			m_EntityData = data;
			m_Version = data->GetVersion();
		}

		inline void Invalidate() const
		{
			m_ID = std::numeric_limits<EntityID>::max();
			m_Container = nullptr;
			m_EntityData = nullptr;
			m_Version = std::numeric_limits<EntityVersion>::max();
		}

	};

	class ConstEntity final
	{
		template<typename ...>
		friend class Query;
		template<typename ...>
		friend class MultiQuery;
		friend class Container;
		template<typename TComponent>
		friend class ComponentRef;
		friend class ComponentBaseRef;
		template<typename>
		friend class ContainerSerializer;
		friend class ContainerSerializerComplex;
		friend class ContainerIterator;

		friend struct std::hash<decs::ConstEntity>;

	public:
		ConstEntity()
		{

		}

		ConstEntity(const Entity& entity) :
			m_Entity(entity)
		{

		}

		bool operator==(const ConstEntity& rhs)const
		{
			return rhs.m_Entity == m_Entity;
		}

		bool operator==(const Entity& rhs)const
		{
			return rhs == m_Entity;
		}

		inline bool IsValid() const
		{
			return m_Entity.IsValid();
		}

		inline bool IsNull() const
		{
			return m_Entity.IsNull();
		}

		inline bool IsActive() const
		{
			return m_Entity.IsActive();
		}

		inline EntityID GetID() const
		{
			return m_Entity.GetID();
		}

		inline Container* GetContainer() const
		{
			return m_Entity.GetContainer();
		}

		template<typename TComponent>
		inline bool HasComponent() const
		{
			return m_Entity.HasComponent<TComponent>();
		}

		template<typename TComponent>
		inline TComponent* GetComponent() const
		{
			return m_Entity.GetComponent<TComponent>();
		}

		template<typename TComponent>
		inline TComponent* GetComponentDynamic() const
		{
			return m_Entity->GetComponentDynamic<TComponent>();
		}

		template<typename TComponent>
		inline bool TryGetComponent(typename TComponent*& component) const
		{
			return m_Entity.TryGetComponent(component);
		}

		inline EntityVersion GetVersion() const
		{
			return m_Entity.GetVersion();
		}

		inline uint32_t ComponentCount() const
		{
			return m_Entity.ComponentCount();
		}

		inline const Archetype* GetArchetype() const
		{
			return m_Entity.GetArchetype();
		}

		inline bool Destroy() const
		{
			return m_Entity.Destroy();
		}

	private:
		Entity m_Entity = {};

	private:
		inline void Set(EntityID id, Container* container)
		{
			m_Entity.Set(id, container);
		}

		inline void Set(EntityData& data, Container* container)
		{
			m_Entity.Set(data, container);
		}

		inline void Set(EntityData* data, Container* container)
		{
			m_Entity.Set(data, container);
		}

		inline void Invalidate() const
		{
			m_Entity.Invalidate();
		}
	};
}

template<>
struct std::hash<decs::Entity>
{
	std::size_t operator()(const decs::Entity& entity) const
	{
		uint64_t entityDataHash = std::hash<decs::EntityData*>{}(entity.m_EntityData);
		uint64_t entityVersionHash = std::hash<decs::EntityVersion>{}(entity.GetVersion());

		return entityDataHash ^ (entityVersionHash + 0x9e3779b9 + (entityDataHash << 6) + (entityDataHash >> 2));
	}
};

template<>
struct std::hash<decs::ConstEntity>
{
	std::size_t operator()(const decs::ConstEntity& entity) const
	{
		return std::hash<decs::Entity>{}(entity.m_Entity);
	}
};