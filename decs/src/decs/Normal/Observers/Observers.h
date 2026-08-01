#pragma once
#include "decs/Core/Core.h"
#include "decs/Core/trait.h"

#include "decs/Normal/Component/Component.h"
#include "decs/Normal/Container.h"

namespace decs
{
	struct Entity;

	template<typename Observer, typename ComponentType>
	concept component_create_observer_concept = requires
	{
		{ static_cast<void(Observer::*)(const Entity&, ComponentType&)>(&Observer::OnCreateComponent) };
	};

	template<typename Observer, typename ComponentType>
	concept component_destroy_observer_concept = requires
	{
		{ static_cast<void(Observer::*)(const Entity&, ComponentType&)>(&Observer::OnDestroyComponent) };
	};

	template<typename Observer, typename ComponentType>
	concept component_enable_observer_concept = requires
	{
		{ static_cast<void(Observer::*)(const Entity&, ComponentType&)>(&Observer::OnEnableComponent) };
	};

	template<typename Observer, typename ComponentType>
	concept component_disable_observer_concept = requires
	{
		{ static_cast<void(Observer::*)(const Entity&, ComponentType&)>(&Observer::OnDisableComponent) };
	};

	template<typename Observer, typename ComponentType>
	concept any_component_observer = component_concept<ComponentType>
		&& (component_create_observer_concept<Observer, ComponentType>
		|| component_destroy_observer_concept<Observer, ComponentType>
		|| component_enable_observer_concept<Observer, ComponentType>
		|| component_disable_observer_concept<Observer, ComponentType>)
		;


	class ObserversManager
	{
	private:
		class IObserverRecord
		{
		public:
			virtual ~IObserverRecord() = default;

			virtual void CreateObservers(Container& container) = 0;

			virtual void RemoveObservers(Container& container) = 0;

			virtual std::shared_ptr<IObserverRecord> Clone() const = 0;
		};

		template<typename ObserverType, component_concept ComponentType>
		class ObserverRecord : public IObserverRecord
		{
		public:
			ObserverType* m_ObserverPtr = nullptr;
			ObserverFunctionID m_CreateID{};
			ObserverFunctionID m_DestroyID{};
			ObserverFunctionID m_EnableID{};
			ObserverFunctionID m_DisableID{};
			int m_Order = 0;

		public:
			ObserverRecord(ObserverType* observerPtr, int order) :
				m_ObserverPtr(observerPtr),
				m_Order(order)
			{

			}

			void CreateObservers(Container& container) override
			{
				if constexpr (component_create_observer_concept<ObserverType, ComponentType>)
				{
					auto func = [obsrever = m_ObserverPtr] (const Entity& entity, ComponentType& component)
					{
						obsrever->OnCreateComponent(entity, component);
					};
					m_CreateID = container.AddComponentObserver<ComponentType>(EComponentObserver::Create, func, m_Order);
				}
				if constexpr (component_destroy_observer_concept<ObserverType, ComponentType>)
				{
					auto func = [obsrever = m_ObserverPtr] (const Entity& entity, ComponentType& component)
					{
						obsrever->OnDestroyComponent(entity, component);
					};
					m_DestroyID = container.AddComponentObserver<ComponentType>(EComponentObserver::Destroy, func, m_Order);
				}
				if constexpr (component_enable_observer_concept<ObserverType, ComponentType>)
				{
					auto func = [obsrever = m_ObserverPtr] (const Entity& entity, ComponentType& component)
					{
						obsrever->OnEnableComponent(entity, component);
					};
					m_EnableID = container.AddComponentObserver<ComponentType>(EComponentObserver::Enable, func, m_Order);
				}
				if constexpr (component_disable_observer_concept<ObserverType, ComponentType>)
				{
					auto func = [obsrever = m_ObserverPtr] (const Entity& entity, ComponentType& component)
					{
						obsrever->OnDisableComponent(entity, component);
					};
					m_DisableID = container.AddComponentObserver<ComponentType>(EComponentObserver::Disable, func, m_Order);
				}
			}

			void RemoveObservers(Container& container)  override
			{
				container.RemoveComponentObservers<ComponentType>(m_CreateID, m_DestroyID, m_EnableID, m_DisableID);
			}

			std::shared_ptr<IObserverRecord> Clone() const override
			{
				return std::make_shared<ObserverRecord>(m_ObserverPtr, m_Order);
			}
		};

		struct ContainerState
		{
		public:
			Container* m_Container{};
			ecsMap<TypeID, std::shared_ptr<IObserverRecord>> m_Observers{};

		public:
			void AddObserver(TypeID id, const std::shared_ptr<const IObserverRecord>& observerRecord)
			{
				auto clone = observerRecord->Clone();
				m_Observers[id] = clone;
				clone->CreateObservers(*m_Container);
			}

			void RemoveObserver(TypeID id)
			{
				auto observerIt = m_Observers.find(id);
				if (observerIt != m_Observers.end())
				{
					observerIt->second->RemoveObservers(*m_Container);
					m_Observers.erase(observerIt);
				}
			}

			void RemoveAllObservers()
			{
				for (auto& [id, observer] : m_Observers)
				{
					observer->RemoveObservers(*m_Container);
				}
			}
		};

	public:
		ObserversManager() = default;

		~ObserversManager()
		{
			ClearObserversAndContainers();
		}

		bool AddContainer(Container* container)
		{
			if (container == nullptr || m_ContainerStates.contains(container))
			{
				return false;
			}

			auto& state = m_ContainerStates[container];
			state.m_Container = container;

			for (auto& [observerTypeID, observerRecord] : m_Observers)
			{
				state.AddObserver(observerTypeID, observerRecord);
			}

			return true;
		}

		bool RemoveContainer(Container* container)
		{
			if (container == nullptr)
			{
				return false;
			}

			auto it = m_ContainerStates.find(container);
			if (it == m_ContainerStates.end())
			{
				return false;
			}

			it->second.RemoveAllObservers();
			m_ContainerStates.erase(it);
		}

		template<component_concept ComponentType, typename ObserverType>
		bool AddObserver(ObserverType* observer, int order = 0)
		{
			if constexpr (!any_component_observer<ObserverType, ComponentType>)
			{
				return false;
			}

			constexpr TypeID observerTypeID = Type<ObserverType>::ID();
			if (observer == nullptr || m_Observers.contains(observerTypeID))
			{
				return false;
			}

			auto observerRecord = std::make_shared<ObserverRecord<ObserverType, ComponentType>>(observer, order);
			m_Observers[observerTypeID] = observerRecord;

			for (auto& [container, containerState] : m_ContainerStates)
			{
				containerState.AddObserver(observerTypeID, observerRecord);
			}

			return true;
		}

		template<component_concept ComponentType, typename ObserverType>
		bool RemoveObserver(ObserverType* observer)
		{
			if constexpr (!any_component_observer<ObserverType, ComponentType>)
			{
				return false;
			}

			constexpr TypeID observerTypeID = Type<ObserverType>::ID();
			if (observer == nullptr)
			{
				return false;
			}

			auto observerRecordIt = m_Observers.find(observerTypeID);
			if (observerRecordIt == m_Observers.end())
			{
				return false;
			}

			auto observerRecord = std::dynamic_pointer_cast<ObserverRecord<ObserverType, ComponentType>>(observerRecordIt->second);
			if (!observerRecord || observerRecord->m_ObserverPtr != observer)
			{
				return false;
			}

			for (auto& [container, containerState] : m_ContainerStates)
			{
				containerState.RemoveObserver(observerTypeID);
			}
			m_Observers.erase(observerRecordIt);

			return true;
		}

		void ClearObserversAndContainers()
		{
			for (auto& [container, state] : m_ContainerStates)
			{
				state.RemoveAllObservers();
			}
			m_ContainerStates.clear();
			m_Observers.clear();
		}
	private:
		ecsMap<TypeID, std::shared_ptr<IObserverRecord>>m_Observers{};
		ecsMap<Container*, ContainerState> m_ContainerStates{};

	};
}