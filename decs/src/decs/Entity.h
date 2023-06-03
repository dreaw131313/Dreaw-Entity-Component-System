#pragma once
#include "Core.h"
#include "Container.h"
#include "Type.h"

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
		template<typename ComponentType>
		friend class ComponentRef;
		friend class ComponentRefAsVoid;
		template<typename T>
		friend class ContainerSerializer;

		friend struct std::hash<decs::Entity>;

	public:
		Entity()
		{

		}

		Entity(EntityID id, Container* container) :
			m_ID(id),
			m_Container(container),
			m_EntityData(&container->m_EntityManager->GetEntityData(id)),
			m_Version(m_EntityData->m_Version)
		{

		}

		Entity(EntityData* entityData, Container* container) :
			m_ID(entityData->GetID()),
			m_Container(container),
			m_EntityData(entityData),
			m_Version(entityData->m_Version)
		{

		}

	public:

		bool operator==(const Entity& rhs)const
		{
			return this->m_Container == rhs.m_Container && this->m_ID == rhs.m_ID && this->m_Version == rhs.m_Version;
		}

		inline EntityID GetID() const { return m_ID; }

		inline Container* GetContainer() const { return m_Container; }

		inline bool IsValid() const
		{
			return m_EntityData != nullptr && m_Version == m_EntityData->m_Version;
		}

		inline bool IsNull() const { return !IsValid() || !m_EntityData->IsAlive(); }

		inline bool IsActive() const
		{
			return IsValid() && m_EntityData->m_bIsActive;
		}

		inline void SetActive(const bool& isActive)
		{
			if (IsValid())
				m_Container->SetEntityActive(*this, isActive);
		}

		inline bool Destroy()
		{
			if (IsValid())
			{
				m_Container->DestroyEntityInternal(*this);
				Invalidate();
				return true;
			}
			return false;
		}

		template<typename T>
		inline typename component_type<T>::Type* GetComponent() const
		{
			if (IsValid())
				return m_Container->GetComponent<T>(*m_EntityData);

			return nullptr;
		}

		template<typename T>
		inline typename component_type<T>::Type* GetStableComponent() const
		{
			if (IsValid())
				return m_Container->GetComponent<decs::Stable<T>>(*m_EntityData);

			return nullptr;
		}

		template<typename T>
		inline bool HasComponent() const
		{
			return IsValid() && m_Container->HasComponent<T>(*m_EntityData);
		}

		template<typename T>
		inline bool HasStableComponent() const
		{
			return IsValid() && m_Container->HasComponent<decs::Stable<T>>(*m_EntityData);
		}

		template<typename T>
		inline bool TryGetComponent(typename component_type<T>::Type*& component) const
		{
			if (IsValid())
				component = m_Container->GetComponent<T>(*m_EntityData);
			else
				component = nullptr;

			return component != nullptr;
		}

		template<typename T>
		inline bool TryGetStableComponent(typename component_type<T>::Type*& component) const
		{
			if (IsValid())
				component = m_Container->GetComponent<decs::Stable<T>>(*m_EntityData);
			else
				component = nullptr;

			return component != nullptr;
		}

		template<typename T, typename... Args>
		inline typename component_type<T>::Type* AddComponent(Args&&... args)
		{
			if (IsValid())
				return m_Container->AddComponent<T>(*this, *m_EntityData, std::forward<Args>(args)...);

			return nullptr;
		}

		template<typename T, typename... Args>
		inline typename component_type<T>::Type* AddStableComponent(Args&&... args)
		{
			if (IsValid())
				return m_Container->AddComponent<decs::Stable<T>>(*this, *m_EntityData, std::forward<Args>(args)...);

			return nullptr;
		}

		template<typename T>
		inline bool RemoveComponent()
		{
			return IsValid() && m_Container->RemoveComponent<T>(*this);
		}

		template<typename T>
		inline bool RemoveStableComponent()
		{
			return IsValid() && m_Container->RemoveComponent<decs::Stable<T>>(*this);
		}

		inline EntityVersion GetVersion() const
		{
			return m_Version;
		}

		inline uint32_t ComponentCount()
		{
			if (IsValid())
				return m_EntityData->ComponentCount();
			return 0;
		}

	private:
		Container* m_Container = nullptr;
		EntityData* m_EntityData = nullptr;
		EntityID m_ID = std::numeric_limits<EntityID>::max();
		EntityVersion m_Version = std::numeric_limits<EntityVersion>::max();

	private:
		void Set(EntityID id, Container* container)
		{
			m_ID = id;
			m_Container = container;
			m_EntityData = &container->m_EntityManager->GetEntityData(id);
			m_Version = m_EntityData->m_Version;
		}

		void Set(EntityData& data, Container* container)
		{
			m_ID = data.m_ID;
			m_Container = container;
			m_EntityData = &data;
			m_Version = data.m_Version;
		}

		void Set(EntityData* data, Container* container)
		{
			m_ID = data->m_ID;
			m_Container = container;
			m_EntityData = data;
			m_Version = data->m_Version;
		}

		inline void Invalidate()
		{
			m_ID = std::numeric_limits<EntityID>::max();
			m_Container = nullptr;
			m_EntityData = nullptr;
			m_Version = std::numeric_limits<EntityVersion>::max();
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