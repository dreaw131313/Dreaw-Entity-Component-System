#pragma once
#include "decs/Core/Type.h"
#include "decs/Core/check_cast.h"
#include "decs/Core/ObserverFunction.h"

#include "decs/Normal/Component/Component.h"
#include "decs/Normal/Component/StableComponentContainer.h"
#include "decs/Normal/Component/PackedComponentContainer.h"

#include "decs/Normal/Statistics.h"

namespace decs
{
	struct Entity;
	class Container;

	enum class EComponentObserver
	{
		Create,
		Destroy,
		Enable,
		Disable
	};

	class IComponentContext
	{
		friend class Container;
	public:
		IComponentContext(int observerOrder = 0) :
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

		virtual IComponentContext* Clone(int observerOrder, uint32_t stableComponentChunkSize) = 0;

		virtual IStableComponentContainer* GetStableContainer() = 0;

		/// <summary>
		/// Life time of container must be managed manualy.
		/// </summary>
		/// <returns></returns>
		virtual IPackedComponentContainer* CreatePackedContainer() const = 0;

		virtual void ClearStableContainer() = 0;

		virtual bool HasCreateObservers() const noexcept = 0;
		virtual bool HasDestroyObservers() const noexcept = 0;
		virtual bool HasEnableObservers() const noexcept = 0;
		virtual bool HasDisableObservers() const noexcept = 0;

		virtual void FillStatistics(ComponentStatistics& statistics) const = 0;

	private:
		int m_ObserverOrder = 0;
	};

	template<typename ComponentType>
	class ComponentContext : public IComponentContext
	{
		friend class Container;

	public:
		using ObserverFunction = ::decs::TObserverFunction<void(const Entity&, ComponentType&)>;

		ObserverFunction m_CreateObservers{};
		ObserverFunction m_DestroyObservers{};
		ObserverFunction m_EnableObservers{};
		ObserverFunction m_DisableObservers{};

	public:
		ComponentContext(int order, uint32_t stableComponentChunkSize) :
			IComponentContext(order),
			m_StableContainer(stableComponentChunkSize > 0 ? stableComponentChunkSize : 1000)
		{

		}

		~ComponentContext()
		{
		}

		inline TypeID GetComponentTypeID() const override
		{
			return Type<ComponentType>::ID();
		}

		IComponentContext* Clone(int observerOrder, uint32_t stableComponentChunkSize) override
		{
			return new ComponentContext<ComponentType>(observerOrder, stableComponentChunkSize);
		}

		void InvokeOnCreateComponent(EntityComponent* component, const Entity& entity)override
		{
			if (!component->IsCreatedByECS())
			{
				component->SetCreated(true);
				m_CreateObservers.Invoke(entity, *::decs::check_cast<ComponentType*>(component));
			}
		}
		void InvokeOnDestroyComponent(EntityComponent* component, const Entity& entity)override
		{
			if (component->IsCreatedByECS())
			{
				component->SetCreated(false);
				m_DestroyObservers.Invoke(entity, *::decs::check_cast<ComponentType*>(component));
			}
		}
		void InvokeOnEnableComponent(EntityComponent* component, const Entity& entity) override
		{
			if (!component->IsEnabledByECS())
			{
				component->SetEnabled(true);
				m_EnableObservers.Invoke(entity, *::decs::check_cast<ComponentType*>(component));
			}
		}
		void InvokeOnDisableComponent(EntityComponent* component, const Entity& entity) override
		{
			if (component->IsEnabledByECS())
			{
				component->SetEnabled(false);
				m_DisableObservers.Invoke(entity, *::decs::check_cast<ComponentType*>(component));
			}
		}

		IStableComponentContainer* GetStableContainer() override
		{
			return &m_StableContainer;
		}

		IPackedComponentContainer* CreatePackedContainer() const override
		{
			return new PackedStableComponentContainer<ComponentType>();
		}

		void ClearStableContainer() override
		{
			m_StableContainer.Clear();
		}

		bool HasCreateObservers() const noexcept override
		{
			return !m_CreateObservers.Empty();
		}
		bool HasDestroyObservers() const noexcept override
		{
			return !m_DestroyObservers.Empty();
		}
		bool HasEnableObservers() const noexcept override
		{
			return !m_EnableObservers.Empty();
		}
		bool HasDisableObservers() const noexcept override
		{
			return !m_DisableObservers.Empty();
		}

		void FillStatistics(ComponentStatistics& stats) const override
		{
			stats.m_ComponentTypeID = Type<ComponentType>::ID();;
			m_StableContainer.FillStatistics(stats);
			stats.m_CreateListenerCount = m_CreateObservers.Size();
			stats.m_DestroyListenerCount = m_DestroyObservers.Size();
			stats.m_EnableListenerCount = m_EnableObservers.Size();
			stats.m_DisableListenerCount = m_DisableObservers.Size();
		}

	private:
		StableComponentContainer<ComponentType> m_StableContainer;
	};
}