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

		// Entity methods implementation:
		template<typename T>
		inline T* GetComponent()
		{
			return m_Container->GetComponent<T>(m_ID);
		}

		template<typename T>
		inline bool HasComponent()
		{
			return m_Container->HasComponent<T>(m_ID);
		}

		template<typename T>
		inline bool TryGetComponent(T*& component)
		{
			component = m_Container->GetComponent<T>(m_ID);
			return component != nullptr;
		}

		template<typename T, typename... Args>
		inline T* AddComponent(Args&&... args)
		{
			return m_Container->CreateComponent<T>(m_ID, std::forward<Args>(args)...);
		}

		template<typename T>
		inline bool RemoveComponent()
		{
			return m_Container->RemoveComponent<T>(m_ID);
		}

		inline void SetActive(const bool& isActive)
		{
			m_Container->SetEntityActive(m_ID, isActive);
		}

		inline bool IsActive() const
		{
			m_Container->IsEntityActive(m_ID);
		}

		inline bool IsAlive() const
		{
			m_Container->IsEntityAlive(m_ID);
		}

		inline uint32_t GetVersion() const
		{
			m_Container->GetEntityVersion(m_ID);
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