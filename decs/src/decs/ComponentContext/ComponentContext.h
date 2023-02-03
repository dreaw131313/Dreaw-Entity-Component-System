#pragma once
#include "decs\Type.h"
#include "decs\Observers\ObserversManager.h"

namespace decs
{
	class Entity;
	class Container;

	class ComponentContextBase
	{
		friend class Container;
	public:
		ComponentContextBase()
		{

		}

		virtual ~ComponentContextBase()
		{

		}

		virtual void InvokeOnCreateComponent_S(void* component, Entity& entity) = 0;
		virtual void InvokeOnDestroyComponent_S(void* component, Entity& entity) = 0;
		virtual void SetObserverManager(ObserversManager* observerManager) = 0;
		virtual ComponentContextBase* CreateOwnEmptyCopy(ObserversManager* observerManager) = 0;

		inline int GetObservatorOrder() const { return m_ObservatorOrder; }

	private:
		int m_ObservatorOrder = 0;
	};

	template<typename ComponentType>
	class ComponentContext : public ComponentContextBase
	{
		friend class Container;
	public:
		ComponentObserver<ComponentType>* m_Observer = nullptr;

	public:
		ComponentContext(
			ComponentObserver<ComponentType>* observer
		) :
			m_Observer(observer)
		{

		}

		~ComponentContext()
		{

		}

		virtual void InvokeOnCreateComponent_S(void* component, Entity& entity)override
		{
			if (m_Observer != nullptr && m_Observer->m_CreateObserver != nullptr)
			{
				m_Observer->m_CreateObserver->OnCreateComponent(*reinterpret_cast<ComponentType*>(component), entity);
			}
		}

		virtual void InvokeOnDestroyComponent_S(void* component, Entity& entity)override
		{
			if (m_Observer != nullptr && m_Observer->m_DestroyObserver != nullptr)
			{
				m_Observer->m_DestroyObserver->OnDestroyComponent(*reinterpret_cast<ComponentType*>(component), entity);
			}
		}

		virtual void SetObserverManager(ObserversManager* observerManager) override
		{
			if (observerManager == nullptr)
			{
				m_Observer = nullptr;
			}
			else
			{
				m_Observer = observerManager->GetComponentObserver<ComponentType>();
			}
		}

		ComponentContextBase* CreateOwnEmptyCopy(ObserversManager* observerManager) override
		{
			return new ComponentContext<ComponentType>(
				observerManager != nullptr ? observerManager->GetComponentObserver<ComponentType>() : nullptr
				);
		}
	};


	template<typename ComponentType>
	class ComponentContext<Stable<ComponentType>> : public ComponentContextBase
	{
		friend class Container;
	public:
		ComponentObserver<Stable<ComponentType>>* m_Observer = nullptr;

	public:
		ComponentContext(
			ComponentObserver<Stable<ComponentType>>* observer
		) :
			m_Observer(observer)
		{

		}

		~ComponentContext()
		{

		}

		virtual void InvokeOnCreateComponent_S(void* component, Entity& entity)override
		{
			if (m_Observer != nullptr && m_Observer->m_CreateObserver != nullptr)
			{
				m_Observer->m_CreateObserver->OnCreateComponent(*reinterpret_cast<ComponentType*>(component), entity);
			}
		}

		virtual void InvokeOnDestroyComponent_S(void* component, Entity& entity)override
		{
			if (m_Observer != nullptr && m_Observer->m_DestroyObserver != nullptr)
			{
				m_Observer->m_DestroyObserver->OnDestroyComponent(*reinterpret_cast<ComponentType*>(component), entity);
			}
		}

		virtual void SetObserverManager(ObserversManager* observerManager) override
		{
			if (observerManager == nullptr)
			{
				m_Observer = nullptr;
			}
			else
			{
				m_Observer = observerManager->GetComponentObserver<Stable<ComponentType>>();
			}
		}

		ComponentContextBase* CreateOwnEmptyCopy(ObserversManager* observerManager) override
		{
			return new ComponentContext<Stable<ComponentType>>(
				observerManager != nullptr ? observerManager->GetComponentObserver<Stable<ComponentType>>() : nullptr
				);
		}
	};
}