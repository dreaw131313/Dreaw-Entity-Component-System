#pragma once
#include "decs/Type.h"
#include "decs/Observers/Observers.h"

#include "decs/Component/Component.h"
#include "decs/ComponentContainers/StableContainer.h"
#include "decs/ComponentContainers/PackedContainer.h"

namespace decs
{
	class Entity;
	class Container;

	class ComponentContextBase
	{
		friend class Container;
	public:
		ComponentContextBase(int observerOrder = 0):
			m_ObserverOrder(observerOrder)
		{

		}

		virtual ~ComponentContextBase() = default;

		inline virtual TypeID GetComponentTypeID() const = 0;

		inline int GetObserverOrder() const { return m_ObserverOrder; }

		inline void SetComponentOrder(int order)
		{
			m_ObserverOrder = order;
		}

		virtual void InvokeOnCreateComponent(EntityComponent* component, const Entity& entity) = 0;

		virtual void InvokeOnDestroyComponent(EntityComponent* component, const Entity& entity) = 0;

		virtual void InvokeOnEnableComponent(EntityComponent* component, const Entity& entity) = 0;

		virtual void InvokeOnDisableComponent(EntityComponent* component, const  Entity& entity) = 0;

		inline virtual bool HasCreateObserver() const = 0;

		inline virtual bool HasDestroyObserver() const = 0;

		virtual ComponentContextBase* Clone(int observerOrder, uint32_t stableComponentChunkSize) = 0;

		virtual StableContainerBase* GetStableContainer() = 0;

		/// <summary>
		/// Life time of container must be managed manualy.
		/// </summary>
		/// <returns></returns>
		virtual PackedContainerBase* CreatePackedContainer() const = 0;

		virtual void ClearStableContainer() = 0;

	private:
		int m_ObserverOrder = 0;
	};

	template<typename TComponent>
	class ComponentContext : public ComponentContextBase
	{
		friend class Container;

	public:
		ComponentContext(int order, uint32_t stableComponentChunkSize):
			ComponentContextBase(order),
			m_StableContainer(stableComponentChunkSize > 0 ? stableComponentChunkSize : 1000)
		{

		}

		~ComponentContext()
		{
		}

		inline TypeID GetComponentTypeID() const override
		{
			return Type<TComponent>::ID();
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

		void InvokeOnCreateComponent(EntityComponent* component, const Entity& entity)override
		{
			if (!component->IsCreatedByECS())
			{
				component->SetCreated(true);
				if (m_Observers.m_CreateObserver != nullptr)
				{
					m_Observers.m_CreateObserver->OnCreateComponent(*static_cast<TComponent*>(component), entity);
				}
			}
		}

		void InvokeOnDestroyComponent(EntityComponent* component, const Entity& entity)override
		{
			if (component->IsCreatedByECS())
			{
				component->SetCreated(false);
				if (m_Observers.m_DestroyObserver != nullptr)
				{
					m_Observers.m_DestroyObserver->OnDestroyComponent(*static_cast<TComponent*>(component), entity);
				}
			}
		}

		void InvokeOnEnableComponent(EntityComponent* component, const Entity& entity) override
		{
			if (!component->IsEnabledByECS())
			{
				component->SetEnabled(true);
				if (m_Observers.m_EnableObserver != nullptr)
				{
					m_Observers.m_EnableObserver->OnEnableComponent(*static_cast<TComponent*>(component), entity);
				}
			}
		}

		void InvokeOnDisableComponent(EntityComponent* component, const Entity& entity) override
		{
			if (component->IsEnabledByECS())
			{
				component->SetEnabled(false);
				if (m_Observers.m_DisableObserver != nullptr)
				{
					m_Observers.m_DisableObserver->OnDisableComponent(*static_cast<TComponent*>(component), entity);
				}
			}
		}

		StableContainerBase* GetStableContainer() override
		{
			return &m_StableContainer;
		}

		PackedContainerBase* CreatePackedContainer() const override
		{
			return new StablePackedContainer<TComponent>();
		}

		virtual void ClearStableContainer() override
		{
			m_StableContainer.Clear();
		}
	private:
		ComponentObserversGroup<TComponent> m_Observers = {};
		StableContainer<TComponent> m_StableContainer;
	};
}