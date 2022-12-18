#pragma once
#include "decspch.h"
#include "Core.h"
#include "Type.h"

#include "Observers.h"


namespace decs
{
	class Entity;

	class ComponentObserverBase
	{
	protected:
		virtual void FunctionToHaveDynamicCast() = 0;
	};

	template<typename ComponentType>
	class ComponentObserver final : public ComponentObserverBase
	{
	public:
		CreateComponentObserver<ComponentType>* m_CreateObserver = nullptr;
		DestroyComponentObserver<ComponentType>* m_DestroyObserver = nullptr;

	protected:
		virtual void FunctionToHaveDynamicCast() override
		{

		}
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
		CreateEntityObserver* m_EntityCreationObserver;
		DestroyEntityObserver* m_EntityDestructionObserver;

		ActivateEntityObserver* m_EntityActivateObserver;
		DeactivateEntityObserver* m_EntityDeactivateObserver;
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

		inline void InvokeEntityCreationObservers(Entity& entity)
		{
			if (m_EntityCreationObserver != nullptr)
			{
				m_EntityCreationObserver->OnCreateEntity(entity);
			}
		}

		inline void InvokeEntityDestructionObservers(Entity& entity)
		{
			if (m_EntityDestructionObserver != nullptr)
			{
				m_EntityDestructionObserver->OnDestroyEntity(entity);
			}
		}

		inline void InvokeEntityActivationObservers(Entity& entity)
		{
			if (m_EntityActivateObserver != nullptr)
			{
				m_EntityActivateObserver->OnSetEntityActive(entity);
			}
		}

		inline void InvokeEntityDeactivationObservers(Entity& entity)
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
		ComponentObserver<ComponentType>* GetComponentObserver()
		{
			ComponentObserverBase*& observer = m_ComponentyContainers[Type<ComponentType>::ID()];
			if (observer == nullptr)
			{
				observer = new ComponentObserver<ComponentType>();
			}
			ComponentObserver<ComponentType>* finalObserver = dynamic_cast<ComponentObserver<ComponentType>*>(observer);
			if (finalObserver == nullptr)
			{
				throw std::runtime_error("Failed to create componenty observer!");
			}
			return finalObserver;
		}

		template<typename ComponentType>
		void SetComponentCreateObserver(CreateComponentObserver<ComponentType>* observer)
		{
			auto componentObserver = GetComponentObserver<ComponentType>();
			componentObserver->m_CreateObserver = observer;
		}

		template<typename ComponentType>
		void SetComponentDestroyObserver(DestroyComponentObserver<ComponentType>* observer)
		{
			auto componentObserver = GetComponentObserver<ComponentType>();
			componentObserver->m_DestroyObserver = observer;
		}

	private:
		ecsMap<TypeID, ComponentObserverBase*> m_ComponentyContainers;
#pragma endregion
	};
}