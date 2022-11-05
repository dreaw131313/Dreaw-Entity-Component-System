#pragma once
#include "decspch.h"

#include "Core.h"
#include "Container.h"

namespace decs
{
	class Container;

	class Entity final
	{
	public:
		Entity()
		{

		}

		Entity(const EntityID& id, Container* container) :
			m_ID(id), m_Container(container)
		{

		}

		~Entity()
		{

		}

		inline operator const EntityID& () { return m_ID; }

		inline EntityID ID() const { return m_ID; }
		inline Container* GetContainer() const { return m_Container; }
		inline bool IsValid() const { return m_Container != nullptr; }

		// Entity methods implementation:
		template<typename T>
		inline T* GetComponent()
		{
			if (IsValid())
				return m_Container->GetComponent<T>(m_ID);

			return nullptr;
		}

		template<typename T>
		inline bool HasComponent()
		{
			return IsValid() && m_Container->HasComponent<T>(m_ID);
		}

		template<typename T>
		inline bool TryGetComponent(T*& component)
		{
			if (IsValid())
				component = m_Container->GetComponent<T>(m_ID);
			else
				component = nullptr;

			return component != nullptr;
		}

		template<typename T, typename... Args>
		inline T* AddComponent(Args&&... args)
		{
			if (IsValid())
				return m_Container->CreateComponent<T>(m_ID, std::forward<Args>(args)...);

			return nullptr;
		}

		template<typename T>
		inline bool RemoveComponent()
		{
			return IsValid() && m_Container->RemoveComponent<T>(m_ID);
		}

		inline void SetActive(const bool& isActive)
		{
			if (IsValid())
				m_Container->SetEntityActive(m_ID, isActive);
		}

		inline bool IsActive() const
		{
			return IsValid() && m_Container->IsEntityActive(m_ID);
		}

		inline bool IsAlive() const
		{
			return IsValid() && m_Container->IsEntityAlive(m_ID);
		}

		inline uint32_t GetVersion() const
		{
			if (IsValid())
				return m_Container->GetEntityVersion(m_ID);

			return std::numeric_limits<uint32_t>::max();
		}

		inline bool Destroy()
		{
			if (IsValid())
			{
				m_Container->DestroyEntity(m_ID);
				m_Container = nullptr;
				m_ID = std::numeric_limits<EntityID>::max();
				return true;
			}
			return false;
		}

	private:
		EntityID m_ID = std::numeric_limits<EntityID>::max();
		Container* m_Container = nullptr;
	};
}

template<>
struct std::hash<decs::Entity>
{
	std::size_t operator()(decs::Entity& entity)
	{
		return std::hash<uint64_t>{}(entity.ID());
	}
};