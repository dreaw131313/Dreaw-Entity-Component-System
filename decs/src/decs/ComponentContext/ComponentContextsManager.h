#pragma once
#include "decs\Core.h"
#include "ComponentContext.h"

namespace decs
{
	struct ComponentContextRecord
	{
	public:
		ComponentContextBase* m_Context = nullptr;
		int m_BufferOrder = 0;
	};

	class ComponentContextsManager final
	{
		friend class Container;
		friend class ArchetypesMap;
	public:
		NON_COPYABLE(ComponentContextsManager);
		NON_MOVEABLE(ComponentContextsManager);

		ComponentContextsManager()
		{

		}

		ComponentContextsManager(ObserversManager* observersManager) :
			m_ObserversManager(observersManager)
		{

		}

		~ComponentContextsManager()
		{
			DestroyComponentsContexts();
		}

		bool SetObserversManager(ObserversManager* observersManager)
		{
			if (observersManager == m_ObserversManager) return false;

			m_ObserversManager = observersManager;
			for (auto& [typeID, contextRecord] : m_Contexts)
			{
				if (contextRecord.m_Context != nullptr)
				{
					contextRecord.m_Context->SetObserverManager(m_ObserversManager);
				}
			}

			return true;
		}

		template<typename ComponentType>
		ComponentContext<ComponentType>* GetOrCreateComponentContext()
		{
			TYPE_ID_CONSTEXPR TypeID id = Type<ComponentType>::ID();

			auto& contextRecord = m_Contexts[id];
			if (contextRecord.m_Context == nullptr)
			{
				ComponentContext<ComponentType>* context = nullptr;
				if (m_ObserversManager != nullptr)
				{
					context = new ComponentContext<ComponentType>(m_ObserversManager->GetComponentObserver<ComponentType>(), contextRecord.m_BufferOrder);
				}
				else
				{
					context = new ComponentContext<ComponentType>(nullptr, contextRecord.m_BufferOrder);
				}
				contextRecord.m_Context = context;
				return context;
			}
			else
			{
				ComponentContext<ComponentType>* containedContext = dynamic_cast<ComponentContext<ComponentType>*>(contextRecord.m_Context);

				if (containedContext == nullptr)
				{
					std::string errorMessage = "decs::Container contains component context with id " + std::to_string(id) + " to type other than " + Type<ComponentType>::Name();
					throw new std::runtime_error(errorMessage.c_str());
				}

				return containedContext;
			}
		}

		ComponentContextBase* GetComponentContext(const TypeID& typeID)
		{
			auto it = m_Contexts.find(typeID);
			return it != m_Contexts.end() ? it->second.m_Context : nullptr;
		}

		template<typename ComponentType>
		bool SetObserverOrder(int order)
		{
			ComponentContext<ComponentType>* context = GetOrCreateComponentContext<ComponentType>();
			if (context->GetObserverOrder() != order)
			{
				context->SetObserverOrder(order);
				return true;
			}
			return false;
		}

		// Return true only when context form component with type id equal componentTypeID exist, else returns false
		bool SetObserverOrder(TypeID componentTypeID, int order)
		{
			auto& record = m_Contexts[componentTypeID];
			if (record.m_Context != nullptr)
			{
				if (record.m_Context->GetObserverOrder() != order)
				{
					record.m_Context->SetObserverOrder(order);
					return true;
				}
			}
			else
			{
				record.m_BufferOrder = order;
			}

			return false;
		}

	private:
		ecsMap<TypeID, ComponentContextRecord> m_Contexts;
		ObserversManager* m_ObserversManager = nullptr;

	private:
		inline void DestroyComponentsContexts()
		{
			for (auto& [key, value] : m_Contexts)
			{
				delete value.m_Context;
			}
		}
	};
}
