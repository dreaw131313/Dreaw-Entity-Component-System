#pragma once
#include "Core.h"
#include "Container.h"
#include "Type.h"

namespace decs
{
	class Container;

	class Entity final
	{
		template<typename ...Types>
		friend class View;
		friend class Container;
		template<typename ComponentType>
		friend class ComponentRef;
		friend class ComponentRefAsVoid;

	public:
		Entity()
		{

		}

		Entity(const EntityID& id, Container* container) :
			m_ID(id),
			m_Container(container),
			m_EntityData(&container->m_EntityManager->GetEntityData(id)),
			m_Version(m_EntityData->m_Version)
		{

		}

		Entity(EntityData& entityData, Container* container) :
			m_ID(entityData.ID()),
			m_Container(container),
			m_EntityData(&entityData),
			m_Version(m_EntityData->m_Version)
		{

		}

	public:

		bool operator==(const Entity& rhs)const
		{
			return this->m_Container == rhs.m_Container && this->m_ID == rhs.m_ID && this->m_Version == rhs.m_Version;
		}

		inline EntityID ID() const { return m_ID; }

		inline Container* GetContainer() const { return m_Container; }

		inline bool IsNull() const { return !IsValid() || !m_EntityData->IsAlive(); }

		inline bool IsActive() const
		{
			return IsValid() && m_EntityData->m_IsActive;
		}

		inline void SetActive(const bool& isActive)
		{
			if (IsValid())
				m_Container->SetEntityActive(m_ID, isActive);
		}

		inline bool Destroy()
		{
			if (IsValid())
			{
				m_Container->DestroyEntity(*this);
				Invalidate();
				return true;
			}
			return false;
		}

		template<typename T>
		inline T* GetComponent() const
		{
			if (IsValid())
				return m_Container->GetComponent<T>(*m_EntityData);

			return nullptr;
		}

		template<typename T>
		inline bool HasComponent() const
		{
			return IsValid() && m_Container->HasComponent<T>(*m_EntityData);
		}

		template<typename T>
		inline bool TryGetComponent(T*& component) const
		{
			if (IsValid())
				component = m_Container->GetComponent<T>(*m_EntityData);
			else
				component = nullptr;

			return component != nullptr;
		}

		template<typename T, typename... Args>
		inline T* AddComponent(Args&&... args)
		{
			if (IsValid())
				return m_Container->AddComponent<T>(*m_EntityData, std::forward<Args>(args)...);

			return nullptr;
		}

		template<typename T>
		inline bool RemoveComponent()
		{
			constexpr TypeID componentTypeID = Type<T>::ID();
			return IsValid() && m_Container->RemoveComponent(*this, componentTypeID);
		}

		inline uint32_t GetVersion() const
		{
			return m_Version;
		}

		inline uint32_t ComponentsCount()
		{
			if (IsValid())
				return m_Container->ComponentsCount(m_ID);
			return 0;
		}

	private:
		EntityID m_ID = std::numeric_limits<EntityID>::max();
		Container* m_Container = nullptr;
		EntityData* m_EntityData = nullptr;
		uint32_t m_Version = std::numeric_limits<uint32_t>::max();

	private:
		void Set(const EntityID& id, Container* container)
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

		inline void Invalidate()
		{
			m_ID = std::numeric_limits<EntityID>::max();
			m_Container = nullptr;
			m_EntityData = nullptr;
			m_Version = std::numeric_limits<uint32_t>::max();
		}

		inline bool IsValid() const
		{
			return m_EntityData != nullptr && m_Version == m_EntityData->m_Version;
		}
	};
}

template<>
struct std::hash<decs::Entity>
{
	std::size_t operator()(const decs::Entity& entity) const
	{
		return std::hash<uint64_t>{}(entity.ID());
	}
};