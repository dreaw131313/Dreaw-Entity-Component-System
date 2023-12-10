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
		ComponentContextBase(int observerOrder = 0) :
			m_ObserverOrder(observerOrder)
		{

		}

		virtual ~ComponentContextBase() = default;

		inline virtual std::string GetComponentName() const = 0;

		inline int GetObserverOrder() const { return m_ObserverOrder; }

		inline void SetComponentOrder(int order)
		{
			m_ObserverOrder = order;
		}

		virtual void SetObserverManager(ObserversManager* observerManager) = 0;

		virtual ComponentContextBase* Clone(ObserversManager* observerManager) = 0;

		virtual void InvokeOnCreateComponent(void* component, Entity& entity) = 0;

		virtual void InvokeOnDestroyComponent(void* component, Entity& entity) = 0;

		virtual void InvokeOnEnableEntity(void* component, Entity& entity) = 0;

		virtual void InvokeOnOnDisable(void* component, Entity& entity) = 0;

	private:
		int m_ObserverOrder = 0;
	};

	template<typename ComponentType>
	class ComponentContext : public ComponentContextBase
	{
		friend class Container;
	public:
		ComponentObserversGroup<ComponentType>* m_ObserversGroup = nullptr;

	public:
		ComponentContext(ComponentObserversGroup<ComponentType>* observer, bool order) :
			ComponentContextBase(order),
			m_ObserversGroup(observer)
		{

		}

		~ComponentContext()
		{

		}

		inline virtual std::string GetComponentName() const override
		{
			return decs::Type<component_type<ComponentType>::Type>::Name();
		}

		virtual void SetObserverManager(ObserversManager* observerManager) override
		{
			if (observerManager == nullptr)
			{
				m_ObserversGroup = nullptr;
			}
			else
			{
				m_ObserversGroup = observerManager->GetComponentObserverGroup<ComponentType>();
			}
		}

		ComponentContextBase* Clone(ObserversManager* observerManager) override
		{
			return new ComponentContext<ComponentType>(
				observerManager != nullptr ? observerManager->GetComponentObserverGroup<ComponentType>() : nullptr,
				GetObserverOrder()
			);
		}

		virtual void InvokeOnCreateComponent(void* component, Entity& entity)override
		{
			if (m_ObserversGroup != nullptr && m_ObserversGroup->m_CreateObserver != nullptr)
			{
				m_ObserversGroup->m_CreateObserver->OnCreateComponent(*reinterpret_cast<ComponentType*>(component), entity);
			}
		}

		virtual void InvokeOnDestroyComponent(void* component, Entity& entity)override
		{
			if (m_ObserversGroup != nullptr && m_ObserversGroup->m_DestroyObserver != nullptr)
			{
				m_ObserversGroup->m_DestroyObserver->OnDestroyComponent(*reinterpret_cast<ComponentType*>(component), entity);
			}
		}

		virtual void InvokeOnEnableEntity(void* component, Entity& entity) override
		{

		}

		virtual void InvokeOnOnDisable(void* component, Entity& entity) override
		{

		}

	};


	template<typename ComponentType>
	class ComponentContext<stable<ComponentType>> : public ComponentContextBase
	{
		friend class Container;
	public:
		ComponentObserversGroup<stable<ComponentType>>* m_ObserversGroup = nullptr;

	public:
		ComponentContext(ComponentObserversGroup<stable<ComponentType>>* observer, int order) :
			ComponentContextBase(order),
			m_ObserversGroup(observer)
		{

		}

		~ComponentContext()
		{

		}

		inline virtual std::string GetComponentName() const override
		{
			return decs::Type<component_type<ComponentType>::Type>::Name();
		}

		virtual void SetObserverManager(ObserversManager* observerManager) override
		{
			if (observerManager == nullptr)
			{
				m_ObserversGroup = nullptr;
			}
			else
			{
				m_ObserversGroup = observerManager->GetComponentObserverGroup<stable<ComponentType>>();
			}
		}

		ComponentContextBase* Clone(ObserversManager* observerManager) override
		{
			return new ComponentContext<stable<ComponentType>>(
				observerManager != nullptr ? observerManager->GetComponentObserverGroup<stable<ComponentType>>() : nullptr,
				GetObserverOrder()
			);
		}

		virtual void InvokeOnCreateComponent(void* component, Entity& entity)override
		{
			if (m_ObserversGroup != nullptr && m_ObserversGroup->m_CreateObserver != nullptr)
			{
				m_ObserversGroup->m_CreateObserver->OnCreateComponent(*reinterpret_cast<ComponentType*>(component), entity);
			}
		}

		virtual void InvokeOnDestroyComponent(void* component, Entity& entity)override
		{
			if (m_ObserversGroup != nullptr && m_ObserversGroup->m_DestroyObserver != nullptr)
			{
				m_ObserversGroup->m_DestroyObserver->OnDestroyComponent(*reinterpret_cast<ComponentType*>(component), entity);
			}
		}

		virtual void InvokeOnEnableEntity(void* component, Entity& entity) override
		{

		}

		virtual void InvokeOnOnDisable(void* component, Entity& entity) override
		{

		}

	};
}