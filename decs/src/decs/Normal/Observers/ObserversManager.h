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

		template<typename ComponentType>
		struct ComponentObserversGroupSetter : public ComponentObserversGroupSetterBase
		{
		public:
			CreateComponentObserver<ComponentType>* m_CreateObserver = nullptr;
			DestroyComponentObserver<ComponentType>* m_DestroyObserver = nullptr;
			EnableComponentObserver<ComponentType>* m_EnableObserver = nullptr;
			DisableComponentObserver<ComponentType>* m_DisableObserver = nullptr;

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

		template<typename ComponentType>
		void SetComponentObservers(
			CreateComponentObserver<ComponentType>* createObserver,
			DestroyComponentObserver<ComponentType>* destroyObserver,
			EnableComponentObserver<ComponentType>* enableObserver,
			DisableComponentObserver<ComponentType>* disableObserver
		)
		{
			auto group = GetComponentObserverGroupSetter<ComponentType>();
			group->m_CreateObserver = createObserver;
			group->m_DestroyObserver = destroyObserver;
			group->m_EnableObserver = enableObserver;
			group->m_DisableObserver = disableObserver;
		}

		template<typename ComponentType>
		void SetCreateComponentObserver(CreateComponentObserver<ComponentType>* createObserver)
		{
			auto group = GetComponentObserverGroupSetter<ComponentType>();
			group->m_CreateObserver = createObserver;
		}

		template<typename ComponentType>
		void SetDestroyComponentObserver(DestroyComponentObserver<ComponentType>* destroyObserver)
		{
			auto group = GetComponentObserverGroupSetter<ComponentType>();
			group->m_DestroyObserver = destroyObserver;
		}

		template<typename ComponentType>
		void SetEnableComponentObserver(EnableComponentObserver<ComponentType>* enableObserver)
		{
			auto group = GetComponentObserverGroupSetter<ComponentType>();
			group->m_EnableObserver = enableObserver;
		}

		template<typename ComponentType>
		void SetDisableComponentObserver(DisableComponentObserver<ComponentType>* disableObserver)
		{
			auto group = GetComponentObserverGroupSetter<ComponentType>();
			group->m_DisableObserver = disableObserver;
		}

		template<typename ComponentType>
		void SetCreateDestroyComponentObservers(
			CreateComponentObserver<ComponentType>* createObserver,
			DestroyComponentObserver<ComponentType>* destroyObserver
		)
		{
			auto group = GetComponentObserverGroupSetter<ComponentType>();
			group->m_CreateObserver = createObserver;
			group->m_DestroyObserver = destroyObserver;
		}

		template<typename ComponentType>
		void SetEnableDisableComponentObservers(
			EnableComponentObserver<ComponentType>* enableObserver,
			DisableComponentObserver<ComponentType>* disableObserver
		)
		{
			auto group = GetComponentObserverGroupSetter<ComponentType>();
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
		ecsVector<ComponentObserversGroupSetterBase*> m_ComponentObserverGroups = {};

	private:
		template<typename ComponentType>
		ComponentObserversGroupSetter<ComponentType>* GetComponentObserverGroupSetter()
		{
			auto& group = m_ComponentObserverGroupIndexes[Type<ComponentType>::ID()];
			if (group == nullptr)
			{
				group = new ComponentObserversGroupSetter<ComponentType>();
				m_ComponentObserverGroups.push_back(group);
			}
			return dynamic_cast<ComponentObserversGroupSetter<ComponentType>*>(group);
		}

	};
}