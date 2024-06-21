#pragma once
#include "decs\Type.h"
#include "decs\Observers\Observers.h"

#include "Component/Component.h"

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

		inline virtual TypeID GetComponentTypeID() const = 0;

		inline virtual std::string GetComponentName() const = 0;

		inline int GetObserverOrder() const { return m_ObserverOrder; }

		inline void SetComponentOrder(int order)
		{
			m_ObserverOrder = order;
		}

		virtual void InvokeOnCreateComponent(ComponentBase* component, const Entity& entity) = 0;

		virtual void InvokeOnDestroyComponent(ComponentBase* component, const Entity& entity) = 0;

		/// <summary>
		/// This function tries invoke Create observer without checking if observer is valid.
		/// </summary>
		/// <param name="component"></param>
		/// <param name="entity"></param>
		virtual void InvokeOnCreateComponentRaw(ComponentBase* component, const Entity& entity) = 0;

		/// <summary>
		/// This function tries invoke Destroy observer without checking if observer is valid.
		/// </summary>
		/// <param name="component"></param>
		/// <param name="entity"></param>
		virtual void InvokeOnDestroyComponentRaw(ComponentBase* component, const Entity& entity) = 0;

		virtual void InvokeOnEnableEntity(ComponentBase* component, const Entity& entity) = 0;

		virtual void InvokeOnDisableEntity(ComponentBase* component, const  Entity& entity) = 0;

		virtual void InvokeOnEnableEntityRaw(ComponentBase* component, const Entity& entity) = 0;

		virtual void InvokeOnDisableEntityRaw(ComponentBase* component, const Entity& entity) = 0;

		void SetCanInvokeCreateObservers(bool bCanInvokeObservers)
		{
			m_bCanInvokeCreateObservers = bCanInvokeObservers;
		}

		inline bool CanInvokeCreateObservers() const
		{
			return m_bCanInvokeCreateObservers;
		}

		inline virtual bool HasCreateObserver() const = 0;

		inline virtual bool HasDestroyObserver() const = 0;

		virtual ComponentContextBase* Clone() = 0;

	private:
		int m_ObserverOrder = 0;
		bool m_bCanInvokeCreateObservers = true;
	};

	template<typename TComponent>
	class ComponentContext : public ComponentContextBase
	{
		friend class Container;

	public:
		ComponentContext(int order) :
			ComponentContextBase(order)
		{

		}

		~ComponentContext()
		{

		}

		inline TypeID GetComponentTypeID() const override
		{
			return Type<TComponent>::ID();
		}

		/// <summary>
		/// 
		/// </summary>
		/// <returns>Name of component if coponent is stable (T = decs::stable<ComponentType>) it will return name of ComponentType without decs::stable</returns>
		inline std::string GetComponentName() const override
		{
			return decs::Type<TComponent>::Name();
		}

		inline bool HasCreateObserver() const override
		{
			return m_Observers.m_CreateObserver != nullptr;
		}

		inline bool HasDestroyObserver() const override
		{
			return m_Observers.m_DestroyObserver != nullptr;
		}

		ComponentContextBase* Clone() override
		{
			return new ComponentContext<TComponent>(GetObserverOrder());
		}

		void InvokeOnCreateComponent(ComponentBase* component, const Entity& entity)override
		{
			if (!component->m_bIsCreatedByECS)
			{
				component->m_bIsCreatedByECS = true;
				if (CanInvokeCreateObservers() && m_Observers.m_CreateObserver != nullptr)
				{
					m_Observers.m_CreateObserver->OnCreateComponent(*static_cast<TComponent*>(component), entity);
				}
			}
		}

		void InvokeOnDestroyComponent(ComponentBase* component, const Entity& entity)override
		{
			if (component->m_bIsCreatedByECS)
			{
				component->m_bIsCreatedByECS = false;
				if (CanInvokeCreateObservers() && m_Observers.m_DestroyObserver != nullptr)
				{
					m_Observers.m_DestroyObserver->OnDestroyComponent(*static_cast<TComponent*>(component), entity);
				}
			}
		}

		void InvokeOnCreateComponentRaw(ComponentBase* component, const Entity& entity) override
		{
			if (!component->m_bIsCreatedByECS)
			{
				component->m_bIsCreatedByECS = true;
				m_Observers.m_CreateObserver->OnCreateComponent(*static_cast<TComponent*>(component), entity);
			}
		}

		void InvokeOnDestroyComponentRaw(ComponentBase* component, const Entity& entity) override
		{
			if (component->m_bIsCreatedByECS)
			{
				component->m_bIsCreatedByECS = false;
				m_Observers.m_DestroyObserver->OnDestroyComponent(*static_cast<TComponent*>(component), entity);
			}
		}

		void InvokeOnEnableEntity(ComponentBase* component, const Entity& entity) override
		{
			if (!component->m_bIsEnabledByECS)
			{
				component->m_bIsEnabledByECS = true;
				if (CanInvokeCreateObservers() && m_Observers.m_EnableObserver != nullptr)
				{
					m_Observers.m_EnableObserver->OnEnableEntity(*static_cast<TComponent*>(component), entity);
				}
			}
		}

		void InvokeOnDisableEntity(ComponentBase* component, const Entity& entity) override
		{
			if (component->m_bIsEnabledByECS)
			{
				component->m_bIsEnabledByECS = false;
				if (CanInvokeCreateObservers() && m_Observers.m_DisableObserver != nullptr)
				{
					m_Observers.m_DisableObserver->OnDisableEntity(*static_cast<TComponent*>(component), entity);
				}
			}
		}

		void InvokeOnEnableEntityRaw(ComponentBase* component, const Entity& entity) override
		{
			if (!component->m_bIsEnabledByECS)
			{
				component->m_bIsEnabledByECS = true;
				m_Observers.m_EnableObserver->OnEnableEntity(*static_cast<TComponent*>(component), entity);
			}
		}

		void InvokeOnDisableEntityRaw(ComponentBase* component, const Entity& entity) override
		{
			if (component->m_bIsEnabledByECS)
			{
				component->m_bIsEnabledByECS = false;
				m_Observers.m_DisableObserver->OnDisableEntity(*static_cast<TComponent*>(component), entity);
			}
		}

	private:
		ComponentObserversGroup<TComponent> m_Observers = {};
	};
}