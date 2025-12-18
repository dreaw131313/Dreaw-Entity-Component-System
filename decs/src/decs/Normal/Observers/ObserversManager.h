#pragma once

#include "Observers.h"
#include "decs/Normal/Container.h"

namespace decs
{
	class ObserversManager
	{
	private:
		class ComponentObserversGroupSetterBase
		{
		public:
			virtual ~ComponentObserversGroupSetterBase() = default;

			virtual void SetObserversToContainer(Container& container) = 0;
		};

		template<typename TComponent>
		struct ComponentObserversGroupSetter : public ComponentObserversGroupSetterBase
		{
		public:
			CreateComponentObserver<TComponent>* m_CreateObserver = nullptr;
			DestroyComponentObserver<TComponent>* m_DestroyObserver = nullptr;
			EnableComponentObserver<TComponent>* m_EnableObserver = nullptr;
			DisableComponentObserver<TComponent>* m_DisableObserver = nullptr;

		public:
			virtual void SetObserversToContainer(Container& container) override
			{
				container.SetComponentObservers(
					m_CreateObserver,
					m_DestroyObserver,
					m_EnableObserver,
					m_DisableObserver
				);
			}
		};
	public:
		ObserversManager() = default;

		~ObserversManager()
		{
			for (auto group : m_ComponentObserverGroups)
			{
				delete group;
			}
		}

		inline void SetCreateEntityObserver(CreateEntityObserver* createEntityObserver)
		{
			m_CreateEntityObserver = createEntityObserver;
		}

		inline void SetDestroyEntityObserver(DestroyEntityObserver* destroyEntityObserver)
		{
			m_DestroyEntityObserver = destroyEntityObserver;
		}

		inline void SetEnableEntityObserver(EnableEntityObserver* enableEntityObserver)
		{
			m_EnableEntityObserver = enableEntityObserver;
		}

		inline void SetDisableEntityObserver(DisableEntityObserver* disableEntityObserver)
		{
			m_DisableEntityObserver = disableEntityObserver;
		}

		template<typename TComponent>
		void SetComponentObservers(
			CreateComponentObserver<TComponent>* createObserver,
			DestroyComponentObserver<TComponent>* destroyObserver,
			EnableComponentObserver<TComponent>* enableObserver,
			DisableComponentObserver<TComponent>* disableObserver
		)
		{
			auto group = GetComponentObserverGroupSetter<TComponent>();
			group->m_CreateObserver = createObserver;
			group->m_DestroyObserver = destroyObserver;
			group->m_EnableObserver = enableObserver;
			group->m_DisableObserver = disableObserver;
		}

		template<typename TComponent>
		void SetCreateComponentObserver(CreateComponentObserver<TComponent>* createObserver)
		{
			auto group = GetComponentObserverGroupSetter<TComponent>();
			group->m_CreateObserver = createObserver;
		}

		template<typename TComponent>
		void SetDestroyComponentObserver(DestroyComponentObserver<TComponent>* destroyObserver)
		{
			auto group = GetComponentObserverGroupSetter<TComponent>();
			group->m_DestroyObserver = destroyObserver;
		}

		template<typename TComponent>
		void SetEnableComponentObserver(EnableComponentObserver<TComponent>* enableObserver)
		{
			auto group = GetComponentObserverGroupSetter<TComponent>();
			group->m_EnableObserver = enableObserver;
		}

		template<typename TComponent>
		void SetDisableComponentObserver(DisableComponentObserver<TComponent>* disableObserver)
		{
			auto group = GetComponentObserverGroupSetter<TComponent>();
			group->m_DisableObserver = disableObserver;
		}

		template<typename TComponent>
		void SetCreateDestroyComponentObservers(
			CreateComponentObserver<TComponent>* createObserver,
			DestroyComponentObserver<TComponent>* destroyObserver
		)
		{
			auto group = GetComponentObserverGroupSetter<TComponent>();
			group->m_CreateObserver = createObserver;
			group->m_DestroyObserver = destroyObserver;
		}

		template<typename TComponent>
		void SetEnableDisableComponentObservers(
			EnableComponentObserver<TComponent>* enableObserver,
			DisableComponentObserver<TComponent>* disableObserver
		)
		{
			auto group = GetComponentObserverGroupSetter<TComponent>();
			group->m_EnableObserver = enableObserver;
			group->m_DisableObserver = disableObserver;
		}

		void FillContainerObservers(Container& container)
		{
			container.SetEntityObservers(
				m_CreateEntityObserver,
				m_DestroyEntityObserver,
				m_EnableEntityObserver,
				m_DisableEntityObserver
			);

			for (auto group : m_ComponentObserverGroups)
			{
				if (group != nullptr)
				{
					group->SetObserversToContainer(container);
				}
			}
		}

	private:
		CreateEntityObserver* m_CreateEntityObserver = nullptr;
		DestroyEntityObserver* m_DestroyEntityObserver = nullptr;
		EnableEntityObserver* m_EnableEntityObserver = nullptr;
		DisableEntityObserver* m_DisableEntityObserver = nullptr;

		ecsMap<TypeID, ComponentObserversGroupSetterBase*> m_ComponentObserverGroupIndexes = {};
		std::vector<ComponentObserversGroupSetterBase*> m_ComponentObserverGroups = {};

	private:
		template<typename TComponent>
		ComponentObserversGroupSetter<TComponent>* GetComponentObserverGroupSetter()
		{
			auto& group = m_ComponentObserverGroupIndexes[Type<TComponent>::ID()];
			if (group == nullptr)
			{
				group = new ComponentObserversGroupSetter<TComponent>();
				m_ComponentObserverGroups.push_back(group);
			}
			return dynamic_cast<ComponentObserversGroupSetter<TComponent>*>(group);
		}

	};
}