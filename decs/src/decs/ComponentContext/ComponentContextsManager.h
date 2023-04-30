#pragma once
#include "decs\Core.h"
#include "ComponentContext.h"

namespace decs
{
	class ComponentContextsManager final
	{
		friend class Container;
		friend class ArchetypesMap;
	public:
		ComponentContextsManager()
		{

		}

		ComponentContextsManager(
			ObserversManager* observersManager
		) :
			m_ObserversManager(observersManager)
		{

		}

		ComponentContextsManager(const ComponentContextsManager&) = delete;
		ComponentContextsManager(ComponentContextsManager&&) = delete;

		ComponentContextsManager& operator=(const ComponentContextsManager&) = delete;
		ComponentContextsManager& operator=(ComponentContextsManager&&) = delete;

		~ComponentContextsManager()
		{
			DestroyComponentsContexts();
		}

		bool SetObserversManager(ObserversManager* observersManager)
		{
			if (observersManager == m_ObserversManager) return false;

			m_ObserversManager = observersManager;
			for (auto& [typeID, context] : m_Contexts)
			{
				context->SetObserverManager(m_ObserversManager);
			}

			return true;
		}

		template<typename ComponentType>
		ComponentContext<ComponentType>* GetOrCreateComponentContext()
		{
			TYPE_ID_CONSTEXPR TypeID id = Type<ComponentType>::ID();

			auto& componentContext = m_Contexts[id];
			if (componentContext == nullptr)
			{
				ComponentContext<ComponentType>* context;
				if (m_ObserversManager != nullptr)
				{
					context = new ComponentContext<ComponentType>(m_ObserversManager->GetComponentObserver<ComponentType>());
				}
				else
				{
					context = new ComponentContext<ComponentType>(nullptr);
				}
				componentContext = context;
				return context;
			}
			else
			{
				ComponentContext<ComponentType>* containedContext = dynamic_cast<ComponentContext<ComponentType>*>(componentContext);

				if (containedContext == nullptr)
				{
					std::string errorMessage = "decs::Container contains component context with id " + std::to_string(id) + " to type other than " + typeid(ComponentType).name();
					throw new std::runtime_error(errorMessage.c_str());
				}

				return containedContext;
			}
		}

		ComponentContextBase* GetComponentContext(const TypeID& typeID)
		{
			auto it = m_Contexts.find(typeID);
			return it != m_Contexts.end() ? it->second : nullptr;
		}
	private:
		ecsMap<TypeID, ComponentContextBase*> m_Contexts;
		ObserversManager* m_ObserversManager = nullptr;

	private:
		inline void DestroyComponentsContexts()
		{
			for (auto& [key, value] : m_Contexts)
				delete value;
		}
	};
}
