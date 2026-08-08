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

	template<typename Observer>
	concept entity_create_observer_concept = requires
	{
		{ static_cast<void(Observer::*)(const Entity&)>(&Observer::OnCreateEntity) };
	};

	template<typename Observer>
	concept entity_destroy_observer_concept = requires
	{
		{ static_cast<void(Observer::*)(const Entity&)>(&Observer::OnDestroyEntity) };
	};

	template<typename Observer>
	concept entity_enable_observer_concept = requires
	{
		{ static_cast<void(Observer::*)(const Entity&)>(&Observer::OnEnableEntity) };
	};

	template<typename Observer>
	concept entity_disable_observer_concept = requires
	{
		{ static_cast<void(Observer::*)(const Entity&)>(&Observer::OnDisableEntity) };
	};

	template<typename Observer>
	concept any_entity_observer_concept = entity_create_observer_concept<Observer>
		|| entity_destroy_observer_concept<Observer>
		|| entity_enable_observer_concept<Observer>
		|| entity_disable_observer_concept<Observer>
		;

	class IObserverRecord
	{
	public:
		virtual ~IObserverRecord() = default;

		virtual void CreateObservers(Container& container) = 0;
		virtual void RemoveObservers(Container& container) = 0;
		virtual std::shared_ptr<IObserverRecord> Clone() const = 0;
		virtual bool IsComponentObserver() const noexcept = 0;
		virtual bool IsEntityObserver() const noexcept = 0;
		virtual TypeID GetComponentTypeID() const noexcept = 0;
		virtual TypeID GetObserverTypeID() const noexcept = 0;
	};

	template<typename ObserverType>
	class TObserverRecordBase : public IObserverRecord
	{
	public:
		ObserverType* m_ObserverPtr = nullptr;

	public:
		TObserverRecordBase(ObserverType* observer) :
			m_ObserverPtr(observer)
		{

		}
	};

	template<component_concept ComponentType, any_component_observer<ComponentType> ObserverType>
	class TComponentObserverRecord final : public TObserverRecordBase<ObserverType>
	{
	public:
		ObserverFunctionID m_CreateID{};
		ObserverFunctionID m_DestroyID{};
		ObserverFunctionID m_EnableID{};
		ObserverFunctionID m_DisableID{};
		int m_Order = 0;

	public:
		TComponentObserverRecord(ObserverType* observerPtr, int order) :
			TObserverRecordBase<ObserverType>(observerPtr),
			m_Order(order)
		{

		}

		void CreateObservers(Container& container) override final
		{
			if constexpr (component_create_observer_concept<ObserverType, ComponentType>)
			{
				auto func = [observer = this->m_ObserverPtr] (const Entity& entity, ComponentType& component)
				{
					observer->OnCreateComponent(entity, component);
				};
				m_CreateID = container.AddComponentCreateObserver<ComponentType>(func, m_Order);
			}
			if constexpr (component_destroy_observer_concept<ObserverType, ComponentType>)
			{
				auto func = [observer = this->m_ObserverPtr] (const Entity& entity, ComponentType& component)
				{
					observer->OnDestroyComponent(entity, component);
				};
				m_DestroyID = container.AddComponentDestroyObserver<ComponentType>(func, m_Order);
			}
			if constexpr (component_enable_observer_concept<ObserverType, ComponentType>)
			{
				auto func = [observer = this->m_ObserverPtr] (const Entity& entity, ComponentType& component)
				{
					observer->OnEnableComponent(entity, component);
				};
				m_EnableID = container.AddComponentEnableObserver<ComponentType>(func, m_Order);
			}
			if constexpr (component_disable_observer_concept<ObserverType, ComponentType>)
			{
				auto func = [observer = this->m_ObserverPtr] (const Entity& entity, ComponentType& component)
				{
					observer->OnDisableComponent(entity, component);
				};
				m_DisableID = container.AddComponentDisableObserver<ComponentType>(func, m_Order);
			}
		}

		void RemoveObservers(Container& container) override final
		{
			container.RemoveComponentObservers<ComponentType>(m_CreateID, m_DestroyID, m_EnableID, m_DisableID);
		}

		bool IsComponentObserver() const noexcept override final
		{
			return true;
		}

		bool IsEntityObserver() const noexcept override final
		{
			return false;
		}

		TypeID GetComponentTypeID() const noexcept override final
		{
			return Type<ComponentType>::ID();
		}

		TypeID GetObserverTypeID() const noexcept override final
		{
			return Type<ObserverType>::ID();
		}

		std::shared_ptr<IObserverRecord> Clone() const override final
		{
			return std::make_shared<TComponentObserverRecord>(this->m_ObserverPtr, m_Order);
		}
	};

	template<any_entity_observer_concept ObserverType>
	class TEntityObserverRecord final : public TObserverRecordBase<ObserverType>
	{
	public:
		ObserverFunctionID m_CreateID{};
		ObserverFunctionID m_DestroyID{};
		ObserverFunctionID m_EnableID{};
		ObserverFunctionID m_DisableID{};
		int m_Order = 0;

	public:
		TEntityObserverRecord(ObserverType* observer, int order) :
			TObserverRecordBase<ObserverType>(observer),
			m_Order(order)
		{

		}

		void CreateObservers(Container& container) override final
		{
			if constexpr (entity_create_observer_concept<ObserverType>)
			{
				auto func = [observer = this->m_ObserverPtr] (const Entity& entity)
				{
					observer->OnCreateEntity(entity);
				};
				m_CreateID = container.AddEntityCreateObserver(func, m_Order);
			}
			if constexpr (entity_destroy_observer_concept<ObserverType>)
			{
				auto func = [observer = this->m_ObserverPtr] (const Entity& entity)
				{
					observer->OnDestroyEntity(entity);
				};
				m_DestroyID = container.AddEntityDestroyObserver(func, m_Order);
			}
			if constexpr (entity_enable_observer_concept<ObserverType>)
			{
				auto func = [observer = this->m_ObserverPtr] (const Entity& entity)
				{
					observer->OnEnableEntity(entity);
				};
				m_EnableID = container.AddEntityEnableObserver(func, m_Order);
			}
			if constexpr (entity_disable_observer_concept<ObserverType>)
			{
				auto func = [observer = this->m_ObserverPtr] (const Entity& entity)
				{
					observer->OnDisableEntity(entity);
				};
				m_DisableID = container.AddEntityDisableObserver(func, m_Order);
			}
		}

		void RemoveObservers(Container& container) override final
		{
			container.RemoveEntityObservers(m_CreateID, m_DestroyID, m_EnableID, m_DisableID);
		}

		bool IsComponentObserver() const noexcept override final
		{
			return false;
		}

		bool IsEntityObserver() const noexcept override final
		{
			return true;
		}

		TypeID GetComponentTypeID() const noexcept override final
		{
			return InvalidTypeID;
		}

		TypeID GetObserverTypeID() const noexcept override final
		{
			return Type<ObserverType>::ID();
		}

		std::shared_ptr<IObserverRecord> Clone() const override final
		{
			return std::make_shared<TEntityObserverRecord>(this->m_ObserverPtr, m_Order);
		}
	};

	class ObserversManager
	{
	private:

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
			if (container == nullptr || m_ContainerStates.contains(container->GetLifeTimeData()))
			{
				return false;
			}

			auto& state = m_ContainerStates[container->GetLifeTimeData()];
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

			auto it = m_ContainerStates.find(container->GetLifeTimeData());
			if (it == m_ContainerStates.end())
			{
				return false;
			}

			it->second.RemoveAllObservers();
			m_ContainerStates.erase(it);
			return true;
		}

		template<typename ObserverType>
		bool AddEntityObserver(ObserverType* observer, int order = 0)
		{
			if constexpr (!any_entity_observer_concept<ObserverType>)
			{
				return false;
			}
			else
			{
				constexpr TypeID observerTypeID = Type<ObserverType>::ID();
				if (observer == nullptr || m_Observers.contains(observerTypeID))
				{
					return false;
				}

				auto observerRecord = std::make_shared<TEntityObserverRecord<ObserverType>>(observer, order);
				m_Observers[observerTypeID] = observerRecord;

				for (auto& [lifetime, containerState] : m_ContainerStates)
				{
					if (lifetime->IsAlive())
					{
						containerState.AddObserver(observerTypeID, observerRecord);
					}
				}

				return true;
			}
		}

		template<component_concept ComponentType, typename ObserverType>
		bool AddObserver(ObserverType* observer, int order = 0)
		{
			if constexpr (!any_component_observer<ObserverType, ComponentType>)
			{
				return false;
			}
			else
			{
				constexpr TypeID observerTypeID = Type<ObserverType>::ID();
				if (observer == nullptr || m_Observers.contains(observerTypeID))
				{
					return false;
				}

				auto observerRecord = std::make_shared<TComponentObserverRecord<ComponentType, ObserverType>>(observer, order);
				m_Observers[observerTypeID] = observerRecord;

				for (auto& [lifetime, containerState] : m_ContainerStates)
				{
					if (lifetime->IsAlive())
					{
						containerState.AddObserver(observerTypeID, observerRecord);
					}
				}

				return true;
			}
		}

		template<typename ObserverType>
		bool RemoveObserver(ObserverType* observer)
		{
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

			auto observerRecord = std::dynamic_pointer_cast<TObserverRecordBase<ObserverType>>(observerRecordIt->second);
			if (!observerRecord || observerRecord->m_ObserverPtr != observer)
			{
				return false;
			}

			for (auto& [lifetime, containerState] : m_ContainerStates)
			{
				if (lifetime->IsAlive())
				{
					containerState.RemoveObserver(observerTypeID);
				}
			}
			m_Observers.erase(observerRecordIt);

			return true;
		}

		template<typename ObserverType>
		bool RemoveObserver()
		{
			constexpr TypeID observerTypeID = Type<ObserverType>::ID();
			auto observerRecordIt = m_Observers.find(observerTypeID);
			if (observerRecordIt == m_Observers.end())
			{
				return false;
			}

			for (auto& [lifetime, containerState] : m_ContainerStates)
			{
				if (lifetime->IsAlive())
				{
					containerState.RemoveObserver(observerTypeID);
				}
			}
			m_Observers.erase(observerRecordIt);

			return true;
		}

		void ClearObserversAndContainers()
		{
			for (auto& [lifetime, state] : m_ContainerStates)
			{
				if (lifetime->IsAlive())
				{
					state.RemoveAllObservers();
				}
			}
			m_ContainerStates.clear();
			m_Observers.clear();
		}

		void ClearDeadContainers()
		{
			std::vector<ContainerLifetimeDataHandle> statesToRemove{};
			for (auto& [lifetime, state] : m_ContainerStates)
			{
				if (!lifetime->IsAlive())
				{
					statesToRemove.push_back(lifetime);
				}
			}

			for (auto& lifetime : statesToRemove)
			{
				m_ContainerStates.erase(lifetime);
			}
		}

	private:
		ecsMap<TypeID, std::shared_ptr<IObserverRecord>>m_Observers{};
		ecsMap<ContainerLifetimeDataHandle, ContainerState> m_ContainerStates{};
	};

	class ConatinerObserversManager
	{
	public:
		ConatinerObserversManager(Container& container):
			m_Container(&container)
		{

		}

		template<typename ObserverType>
		bool AddEntityObserver(ObserverType* observer, int order = 0)
		{
			if constexpr (!any_entity_observer_concept<ObserverType>)
			{
				return false;
			}
			else
			{
				constexpr TypeID observerTypeID = Type<ObserverType>::ID();
				if (observer == nullptr || m_Observers.contains(observerTypeID))
				{
					return false;
				}

				auto observerRecord = std::make_shared<TEntityObserverRecord<ObserverType>>(observer, order);
				m_Observers[observerTypeID] = observerRecord;

				if (m_Container != nullptr)
				{
					observerRecord->CreateObservers(*m_Container);
				}

				return true;
			}
		}

		template<component_concept ComponentType, typename ObserverType>
		bool AddObserver(ObserverType* observer, int order = 0)
		{
			if constexpr (!any_component_observer<ObserverType, ComponentType>)
			{
				return false;
			}
			else
			{
				constexpr TypeID observerTypeID = Type<ObserverType>::ID();
				if (observer == nullptr || m_Observers.contains(observerTypeID))
				{
					return false;
				}

				auto observerRecord = std::make_shared<TComponentObserverRecord<ComponentType, ObserverType>>(observer, order);
				m_Observers[observerTypeID] = observerRecord;

				if (m_Container != nullptr)
				{
					observerRecord->CreateObservers(*m_Container);
				}

				return true;
			}
		}

		template<typename ObserverType>
		bool RemoveObserver(ObserverType* observer)
		{
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

			std::shared_ptr<TObserverRecordBase<ObserverType>> observerRecord = std::dynamic_pointer_cast<TObserverRecordBase<ObserverType>>(observerRecordIt->second);
			if (!observerRecord || observerRecord->m_ObserverPtr != observer)
			{
				return false;
			}

			if (m_Container != nullptr)
			{
				observerRecord->RemoveObservers(*m_Container);
			}

			m_Observers.erase(observerRecordIt);

			return true;
		}

		template<typename ObserverType>
		bool RemoveObserver()
		{
			constexpr TypeID observerTypeID = Type<ObserverType>::ID();
			auto observerRecordIt = m_Observers.find(observerTypeID);
			if (observerRecordIt == m_Observers.end())
			{
				return false;
			}

			std::shared_ptr<IObserverRecord> observerRecord = observerRecordIt->second;
			if (m_Container != nullptr)
			{
				observerRecord->RemoveObservers(*m_Container);
			}

			m_Observers.erase(observerRecordIt);

			return true;
		}


	private:
		ecsMap<TypeID, std::shared_ptr<IObserverRecord>>m_Observers{};;
		Container* m_Container = nullptr;
	};

}