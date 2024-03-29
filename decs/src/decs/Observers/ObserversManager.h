#pragma once
#include "decs\Core.h"
#include "decs\Type.h"

#include "Observers.h"

namespace decs
{
	class Entity;

	class ComponentObserversGroupBase
	{
	public:
		virtual ~ComponentObserversGroupBase() = default;
	};

	template<typename ComponentType>
	class ComponentObserversGroup final : public ComponentObserversGroupBase
	{
	public:
		CreateComponentObserver<ComponentType>* m_CreateObserver = nullptr;
		DestroyComponentObserver<ComponentType>* m_DestroyObserver = nullptr;
	};

	class ObserversManager
	{
	public:
		ObserversManager()
		{

		}

		~ObserversManager()
		{
			for (auto& [key, value] : m_ComponentyContainers)
			{
				delete value;
			}
		}

#pragma region ENTITY OBSERVERS:
	private:
		CreateEntityObserver* m_EntityCreationObserver = nullptr;
		DestroyEntityObserver* m_EntityDestructionObserver = nullptr;

		ActivateEntityObserver* m_EntityActivateObserver = nullptr;
		DeactivateEntityObserver* m_EntityDeactivateObserver = nullptr;
	public:

		bool SetEntityCreationObserver(CreateEntityObserver* observer)
		{
			if (observer == nullptr) return false;
			m_EntityCreationObserver = observer;
			return true;
		}

		bool SetEntityDestructionObserver(DestroyEntityObserver* observer)
		{
			if (observer == nullptr) return false;
			m_EntityDestructionObserver = observer;
			return true;
		}

		bool SetEntityActivationObserver(ActivateEntityObserver* observer)
		{
			if (observer == nullptr) return false;
			m_EntityActivateObserver = observer;
			return true;
		}

		bool SetEntityDeactivationObserver(DeactivateEntityObserver* observer)
		{
			if (observer == nullptr) return false;
			m_EntityDeactivateObserver = observer;
			return true;
		}

		inline void InvokeEntityCreationObservers(const Entity& entity)
		{
			if (m_EntityCreationObserver != nullptr)
			{
				m_EntityCreationObserver->OnCreateEntity(entity);
			}
		}

		inline void InvokeEntityDestructionObservers(const Entity& entity)
		{
			if (m_EntityDestructionObserver != nullptr)
			{
				m_EntityDestructionObserver->OnDestroyEntity(entity);
			}
		}

		inline void InvokeEntityActivationObservers(const Entity& entity)
		{
			if (m_EntityActivateObserver != nullptr)
			{
				m_EntityActivateObserver->OnSetEntityActive(entity);
			}
		}

		inline void InvokeEntityDeactivationObservers(const Entity& entity)
		{
			if (m_EntityDeactivateObserver != nullptr)
			{
				m_EntityDeactivateObserver->OnSetEntityInactive(entity);
			}
		}

#pragma endregion

#pragma region COMPONENTS OBSERVERS
	public:
		template<typename ComponentType>
		ComponentObserversGroup<ComponentType>* GetComponentObserverGroup()
		{
			ComponentObserversGroupBase*& observer = m_ComponentyContainers[Type<ComponentType>::ID()];
			if (observer == nullptr)
			{
				observer = new ComponentObserversGroup<ComponentType>();
			}
			ComponentObserversGroup<ComponentType>* finalObserver = dynamic_cast<ComponentObserversGroup<ComponentType>*>(observer);
			if (finalObserver == nullptr)
			{
				throw std::runtime_error("Failed to create componenty observer!");
			}
			return finalObserver;
		}

		template<typename ComponentType>
		void SetComponentCreateObserver(CreateComponentObserver<ComponentType>* observer)
		{
			auto componentObserver = GetComponentObserverGroup<ComponentType>();
			componentObserver->m_CreateObserver = observer;
		}

		template<typename ComponentType>
		void SetComponentDestroyObserver(DestroyComponentObserver<ComponentType>* observer)
		{
			auto componentObserver = GetComponentObserverGroup<ComponentType>();
			componentObserver->m_DestroyObserver = observer;
		}

	private:
		ecsMap<TypeID, ComponentObserversGroupBase*> m_ComponentyContainers;
#pragma endregion
	};
}