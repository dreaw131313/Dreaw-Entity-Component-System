#pragma once
#include "decs\Type.h"
#include "decs\Observers\Observers.h"

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

		virtual void InvokeOnCreateComponent(void* component, const Entity& entity) = 0;

		virtual void InvokeOnDestroyComponent(void* component, const Entity& entity) = 0;

		/// <summary>
		/// This function tries invoke Create observer without checking if observer is valid.
		/// </summary>
		/// <param name="component"></param>
		/// <param name="entity"></param>
		virtual void InvokeOnCreateComponentRaw(void* component, const Entity& entity) = 0;

		/// <summary>
		/// This function tries invoke Destroy observer without checking if observer is valid.
		/// </summary>
		/// <param name="component"></param>
		/// <param name="entity"></param>
		virtual void InvokeOnDestroyComponentRaw(void* component, const Entity& entity) = 0;

		virtual void InvokeOnEnableEntity(void* component, const Entity& entity) = 0;

		virtual void InvokeOnOnDisableEntity(void* component, const  Entity& entity) = 0;

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

	template<typename T>
	class ComponentContext : public ComponentContextBase
	{
		friend class Container;

		using TComponentType = component_type<T>::Type;

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
			return Type<T>::ID();
		}

		/// <summary>
		/// 
		/// </summary>
		/// <returns>Name of component if coponent is stable (T = decs::stable<ComponentType>) it will return name of ComponentType without decs::stable</returns>
		inline std::string GetComponentName() const override
		{
			return decs::Type<TComponentType>::Name();
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
			return new ComponentContext<T>(GetObserverOrder());
		}

		void InvokeOnCreateComponent(void* component, const Entity& entity)override
		{
			if (CanInvokeCreateObservers() && m_Observers.m_CreateObserver)
			{
				m_Observers.m_CreateObserver->OnCreateComponent(*static_cast<TComponentType*>(component), entity);
			}
		}

		void InvokeOnDestroyComponent(void* component, const Entity& entity)override
		{
			if (CanInvokeCreateObservers() && m_Observers.m_DestroyObserver != nullptr)
			{
				m_Observers.m_DestroyObserver->OnDestroyComponent(*static_cast<TComponentType*>(component), entity);
			}
		}

		void InvokeOnCreateComponentRaw(void* component, const Entity& entity) override
		{
			m_Observers.m_CreateObserver->OnCreateComponent(*static_cast<TComponentType*>(component), entity);
		}

		void InvokeOnDestroyComponentRaw(void* component, const Entity& entity) override
		{
			m_Observers.m_DestroyObserver->OnDestroyComponent(*static_cast<TComponentType*>(component), entity);
		}

		void InvokeOnEnableEntity(void* component, const Entity& entity) override
		{
			if (CanInvokeCreateObservers() && m_Observers.m_EnableObserver)
			{
				m_Observers.m_EnableObserver->OnEnableEntity(*static_cast<TComponentType*>(component), entity);
			}
		}

		void InvokeOnOnDisableEntity(void* component, const Entity& entity) override
		{
			if (CanInvokeCreateObservers() && m_Observers.m_DisableObserver)
			{
				m_Observers.m_DisableObserver->OnDisableEntity(*static_cast<TComponentType*>(component), entity);
			}
		}

	private:
		ComponentObserversGroup<T> m_Observers = {};
	};
}