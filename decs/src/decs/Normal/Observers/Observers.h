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

	struct FlatObserverRecord
	{
	public:
		using AddObserverFuncType = void(*)(FlatObserverRecord& record, Container& container, void* observerPtr, int order);
		using RemoveObserverFuncType = void(*)(FlatObserverRecord& record, Container& container, void* observerPtr);

	public:
		AddObserverFuncType m_AddObserverFunc = nullptr;
		RemoveObserverFuncType m_RemoveObserverFunc = nullptr;

		void* m_ObserverPtr = nullptr;
		TypeID m_ObserverID = InvalidTypeID;
		ObserverFunctionID m_CreateID{};
		ObserverFunctionID m_DestroyID{};
		ObserverFunctionID m_EnableID{};
		ObserverFunctionID m_DisableID{};
		int m_Order = 0;

	public:
		template<typename ComponentType, typename ObserverType>
		void SetupComponentObserver(ObserverType* observer, int order)
		{
			m_ObserverPtr = observer;
			m_ObserverID = Type<ObserverType>::ID();
			m_Order = order;

			m_AddObserverFunc = &AddComponentObserver<ComponentType, ObserverType>;
			m_RemoveObserverFunc = &RemoveComponentObserver<ComponentType>;
		}

		template<typename ObserverType>
		void SetupEntityObserver(ObserverType* observer, int order)
		{
			m_ObserverPtr = observer;
			m_ObserverID = Type<ObserverType>::ID();
			m_Order = order;

			m_AddObserverFunc = &AddEntityObserver<ObserverType>;
			m_RemoveObserverFunc = &RemoveEntityObserver;
		}

		void AddObservers(Container& container)
		{
			if (m_AddObserverFunc != nullptr)
			{
				m_AddObserverFunc(*this, container, m_ObserverPtr, m_Order);
			}
		}

		void RemoveObservers(Container& container)
		{
			if (m_AddObserverFunc != nullptr)
			{
				m_RemoveObserverFunc(*this, container, m_ObserverPtr);
			}
		}

	private:
		template<typename ComponentType, typename ObserverType>
		static void AddComponentObserver(FlatObserverRecord& record, Container& container, void* observerPtr, int order)
		{
			if (observerPtr == nullptr)
			{
				return;
			}
			ObserverType* castedObserver = static_cast<ObserverType*>(observerPtr);

			if constexpr (component_create_observer_concept<ObserverType, ComponentType>)
			{
				auto func = [observer = castedObserver] (const Entity& entity, ComponentType& component)
				{
					observer->OnCreateComponent(entity, component);
				};
				record.m_CreateID = container.AddComponentCreateObserver<ComponentType>(func, order);
			}
			if constexpr (component_destroy_observer_concept<ObserverType, ComponentType>)
			{
				auto func = [observer = castedObserver] (const Entity& entity, ComponentType& component)
				{
					observer->OnDestroyComponent(entity, component);
				};
				record.m_DestroyID = container.AddComponentDestroyObserver<ComponentType>(func, order);
			}
			if constexpr (component_enable_observer_concept<ObserverType, ComponentType>)
			{
				auto func = [observer = castedObserver] (const Entity& entity, ComponentType& component)
				{
					observer->OnEnableComponent(entity, component);
				};
				record.m_EnableID = container.AddComponentEnableObserver<ComponentType>(func, order);
			}
			if constexpr (component_disable_observer_concept<ObserverType, ComponentType>)
			{
				auto func = [observer = castedObserver] (const Entity& entity, ComponentType& component)
				{
					observer->OnDisableComponent(entity, component);
				};
				record.m_DisableID = container.AddComponentDisableObserver<ComponentType>(func, order);
			}

		}

		template<typename ComponentType>
		static void RemoveComponentObserver(FlatObserverRecord& record, Container& container, void* observerPtr)
		{
			if (observerPtr == nullptr)
			{
				return;
			}
			container.RemoveComponentObservers<ComponentType>(record.m_CreateID, record.m_DestroyID, record.m_EnableID, record.m_DisableID);
		}

		template<typename ObserverType>
		static void AddEntityObserver(FlatObserverRecord& record, Container& container, void* observerPtr, int order)
		{
			if (observerPtr == nullptr)
			{
				return;
			}
			ObserverType* castedObserver = static_cast<ObserverType*>(observerPtr);

			if constexpr (entity_create_observer_concept<ObserverType>)
			{
				auto func = [observer = castedObserver] (const Entity& entity)
				{
					observer->OnCreateEntity(entity);
				};
				record.m_CreateID = container.AddEntityCreateObserver(func, order);
			}
			if constexpr (entity_destroy_observer_concept<ObserverType>)
			{
				auto func = [observer = castedObserver] (const Entity& entity)
				{
					observer->OnDestroyEntity(entity);
				};
				record.m_DestroyID = container.AddEntityDestroyObserver(func, order);
			}
			if constexpr (entity_enable_observer_concept<ObserverType>)
			{
				auto func = [observer = castedObserver] (const Entity& entity)
				{
					observer->OnEnableEntity(entity);
				};
				record.m_EnableID = container.AddEntityEnableObserver(func, order);
			}
			if constexpr (entity_disable_observer_concept<ObserverType>)
			{
				auto func = [observer = castedObserver] (const Entity& entity)
				{
					observer->OnDisableEntity(entity);
				};
				record.m_DisableID = container.AddEntityDisableObserver(func, order);
			}
		}

		static void RemoveEntityObserver(FlatObserverRecord& record, Container& container, void* observerPtr)
		{
			if (observerPtr == nullptr)
			{
				return;
			}
			container.RemoveEntityObservers(record.m_CreateID, record.m_DestroyID, record.m_EnableID, record.m_DisableID);
		}

	};

	class ContainerObserversManager
	{
	public:
		ContainerObserversManager(Container& container) :
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
				FlatObserverRecord& observerRecord = m_Observers[observerTypeID];
				if (observerRecord.m_ObserverPtr != nullptr)
				{
					return false;
				}

				observerRecord.SetupEntityObserver(observer, order);

				if (m_Container != nullptr)
				{
					observerRecord.AddObservers(*m_Container);
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
				FlatObserverRecord& observerRecord = m_Observers[observerTypeID];
				if (observerRecord.m_ObserverPtr != nullptr)
				{
					return false;
				}

				observerRecord.SetupComponentObserver<ComponentType, ObserverType>(observer, order);

				if (m_Container != nullptr)
				{
					observerRecord.AddObservers(*m_Container);
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

			FlatObserverRecord& observerRecord = observerRecordIt->second;
			if (observerRecord.m_ObserverPtr != observer)
			{
				return false;
			}

			if (m_Container != nullptr)
			{
				observerRecord.RemoveObservers(*m_Container);
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

			FlatObserverRecord& observerRecord = observerRecordIt->second;
			if (m_Container != nullptr)
			{
				observerRecord.RemoveObservers(*m_Container);
			}

			m_Observers.erase(observerRecordIt);

			return true;
		}

		template<typename ObserverType>
		inline ObserverType* GetObserver() const noexcept
		{
			auto it = m_Observers.find(Type<ObserverType>::ID());
			if (it == m_Observers.end())
			{
				return nullptr;
			}

			const FlatObserverRecord& observerRecord = it->second;
			return static_cast<ObserverType*>(observerRecord.m_ObserverPtr);
		}

	private:
		ecsHashMap<TypeID, FlatObserverRecord> m_Observers{};;
		Container* m_Container = nullptr;
	};

}