#pragma once
#include "decs\Type.h"
#include "decs\Observers\Observers.h"

#include "Component/Component.h"
#include "decs/ComponentContainers/StableContainer.h"

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

		inline virtual bool IsStableComponentContext() const = 0;

		inline int GetObserverOrder() const { return m_ObserverOrder; }

		inline void SetComponentOrder(int order)
		{
			m_ObserverOrder = order;
		}

		virtual void InvokeOnCreateComponent(ComponentBase* component, const Entity& entity) = 0;

		virtual void InvokeOnDestroyComponent(ComponentBase* component, const Entity& entity) = 0;

		virtual void InvokeOnEnableComponent(ComponentBase* component, const Entity& entity) = 0;

		virtual void InvokeOnDisableComponent(ComponentBase* component, const  Entity& entity) = 0;

		inline virtual bool HasCreateObserver() const = 0;

		inline virtual bool HasDestroyObserver() const = 0;

		virtual ComponentContextBase* Clone(int observerOrder, uint32_t stableComponentChunkSize) = 0;

		virtual StableContainerBase* GetStableContainer() const = 0;

		virtual void ClearStableContainer() = 0;

	private:
		int m_ObserverOrder = 0;
	};

	template<typename TComponent>
	class ComponentContext : public ComponentContextBase
	{
		friend class Container;

	public:
		ComponentContext(int order, uint32_t stableComponentChunkSize) :
			ComponentContextBase(order)
		{
			if constexpr (TComponent::IsStable)
			{
				m_StableContainer = new StableContainer<TComponent>(stableComponentChunkSize > 0 ? stableComponentChunkSize : 1000);
			}
		}

		~ComponentContext()
		{
			if (m_StableContainer != nullptr)
			{
				delete m_StableContainer;
			}
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

		inline bool IsStableComponentContext() const override
		{
			return TComponent::IsStable;
		}

		inline bool HasCreateObserver() const override
		{
			return m_Observers.m_CreateObserver != nullptr;
		}

		inline bool HasDestroyObserver() const override
		{
			return m_Observers.m_DestroyObserver != nullptr;
		}

		ComponentContextBase* Clone(int observerOrder, uint32_t stableComponentChunkSize) override
		{
			return new ComponentContext<TComponent>(observerOrder, stableComponentChunkSize);
		}

		void InvokeOnCreateComponent(ComponentBase* component, const Entity& entity)override
		{
			if (!component->m_bIsCreatedByContainer)
			{
				component->m_bIsCreatedByContainer = true;
				if (m_Observers.m_CreateObserver != nullptr)
				{
					m_Observers.m_CreateObserver->OnCreateComponent(*static_cast<TComponent*>(component), entity);
				}
			}
		}

		void InvokeOnDestroyComponent(ComponentBase* component, const Entity& entity)override
		{
			if (component->m_bIsCreatedByContainer)
			{
				component->m_bIsCreatedByContainer = false;
				if (m_Observers.m_DestroyObserver != nullptr)
				{
					m_Observers.m_DestroyObserver->OnDestroyComponent(*static_cast<TComponent*>(component), entity);
				}
			}
		}

		void InvokeOnEnableComponent(ComponentBase* component, const Entity& entity) override
		{
			if (!component->m_bIsEnabledByECS)
			{
				component->m_bIsEnabledByECS = true;
				if (m_Observers.m_EnableObserver != nullptr)
				{
					m_Observers.m_EnableObserver->OnEnableEntity(*static_cast<TComponent*>(component), entity);
				}
			}
		}

		void InvokeOnDisableComponent(ComponentBase* component, const Entity& entity) override
		{
			if (component->m_bIsEnabledByECS)
			{
				component->m_bIsEnabledByECS = false;
				if (m_Observers.m_DisableObserver != nullptr)
				{
					m_Observers.m_DisableObserver->OnDisableEntity(*static_cast<TComponent*>(component), entity);
				}
			}
		}

		StableContainerBase* GetStableContainer() const override
		{
			return m_StableContainer;
		}

		virtual void ClearStableContainer() override
		{
			if (m_StableContainer != nullptr)
			{
				m_StableContainer->Clear();
			}
		}
	private:
		ComponentObserversGroup<TComponent> m_Observers = {};
		StableContainer<TComponent>* m_StableContainer = nullptr;
	};
}