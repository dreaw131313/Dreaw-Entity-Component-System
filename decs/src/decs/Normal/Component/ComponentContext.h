#pragma once
#include "decs/Core/Type.h"
#include "decs/Core/check_cast.h"

#include "decs/Normal/Observers/Observers.h"
#include "decs/Normal/Component/Component.h"
#include "decs/Normal/Component/StableComponentContainer.h"
#include "decs/Normal/Component/PackedComponentContainer.h"

namespace decs
{
	class Entity;
	class Container;

	class IComponentContext
	{
		friend class Container;
	public:
		IComponentContext(int observerOrder = 0):
			m_ObserverOrder(observerOrder)
		{

		}

		virtual ~IComponentContext() = default;

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

		virtual IComponentContext* Clone(int observerOrder, uint32_t stableComponentChunkSize) = 0;

		virtual IStableComponentContainer* GetStableContainer() = 0;

		/// <summary>
		/// Life time of container must be managed manualy.
		/// </summary>
		/// <returns></returns>
		virtual IPackedComponentContainer* CreatePackedContainer() const = 0;

		virtual void ClearStableContainer() = 0;

	private:
		int m_ObserverOrder = 0;
	};

	template<typename TComponent>
	class ComponentContext : public IComponentContext
	{
		friend class Container;

	public:
		ComponentContext(int order, uint32_t stableComponentChunkSize):
			IComponentContext(order),
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

		IComponentContext* Clone(int observerOrder, uint32_t stableComponentChunkSize) override
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
					m_Observers.m_CreateObserver->OnCreateComponent(*::decs::check_cast<TComponent*>(component), entity);
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
					m_Observers.m_DestroyObserver->OnDestroyComponent(*::decs::check_cast<TComponent*>(component), entity);
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
					m_Observers.m_EnableObserver->OnEnableComponent(*::decs::check_cast<TComponent*>(component), entity);
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
					m_Observers.m_DisableObserver->OnDisableComponent(*::decs::check_cast<TComponent*>(component), entity);
				}
			}
		}

		IStableComponentContainer* GetStableContainer() override
		{
			return &m_StableContainer;
		}

		IPackedComponentContainer* CreatePackedContainer() const override
		{
			return new PackedStableComponentContainer<TComponent>();
		}

		virtual void ClearStableContainer() override
		{
			m_StableContainer.Clear();
		}
	private:
		ComponentObserversGroup<TComponent> m_Observers = {};
		StableComponentContainer<TComponent> m_StableContainer;
	};
}