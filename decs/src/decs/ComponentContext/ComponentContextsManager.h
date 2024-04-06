#pragma once
#include "decs\Core.h"
#include "ComponentContext.h"

namespace decs
{
	struct ComponentContextRecord
	{
	public:
		ComponentContextBase* m_Context = nullptr;
		uint64_t m_OrderIndex = std::numeric_limits<uint64_t>::max();
		int m_Order = 0;
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
					context = new ComponentContext<ComponentType>(m_ObserversManager->GetComponentObserverGroup<ComponentType>(), contextRecord.m_Order);
				}
				else
				{
					context = new ComponentContext<ComponentType>(nullptr, contextRecord.m_Order);
				}
				contextRecord.m_Context = context;

				OnSetComponentTypeOrder(contextRecord);
				return context;
			}
			else
			{
				ComponentContext<ComponentType>* containedContext = dynamic_cast<ComponentContext<ComponentType>*>(contextRecord.m_Context);

				if (containedContext == nullptr)
				{
					std::string errorMessage = "decs::Container contains component context with id " + std::to_string(id) + " to type other than " + Type<ComponentType>::Name();
					throw std::runtime_error(errorMessage.c_str());
				}
				return containedContext;
			}
		}

		ComponentContextBase* GetComponentContext(const TypeID& typeID)
		{
			auto it = m_Contexts.find(typeID);
			return it != m_Contexts.end() ? it->second.m_Context : nullptr;
		}

		// Return true only when context form component with type id equal componentTypeID exist, else returns false
		bool SetComponentOrder(TypeID componentTypeID, int order)
		{
			auto& record = m_Contexts[componentTypeID];
			if (record.m_Order != order)
			{
				record.m_Order = order;
				if (record.m_Context != nullptr)
				{
					record.m_Context->SetComponentOrder(order);
					OnSetComponentTypeOrder(record);
				}
				return true;
			}
			return false;
		}

		template<typename ComponentType>
		bool SetComponentOrder(int order)
		{
			GetOrCreateComponentContext<ComponentType>();
			return SetComponentOrder(Type<ComponentType>::ID(), order);
		}

		uint32_t SetComponentsOrder(const std::vector<std::pair<TypeID, int>>& componentOrders)
		{
			uint32_t counter = 0;

			return counter;
		}

		const std::vector<ComponentContextBase*>& GetComponentContextsInOrder() const
		{
			return m_ComponentContextsInOrder;
		}

		/// <summary>
		/// Used only when invoking create observers by Container class.
		/// </summary>
		/// <typeparam name="Callable"></typeparam>
		/// <param name="func"></param>
		template<typename Callable>
		void IterateOverComponentContexts(Callable&& func)
		{
			for (m_IterationIndex = 0; m_IterationIndex < (int64_t)m_ComponentContextsInOrder.size(); m_IterationIndex++)
			{
				func(m_ComponentContextsInOrder[m_IterationIndex]);
			}
			m_IterationIndex = std::numeric_limits<int64_t>::max();
		}

		/// <summary>
		/// Used only when invoking destroy observers by Container class.
		/// </summary>
		/// <typeparam name="Callable"></typeparam>
		/// <param name="func"></param>
		template<typename Callable>
		void IterateOverComponentContextsForDestryObservers(Callable&& func)
		{
			for (int64_t idx = (int64_t)m_ComponentContextsInOrder.size() - 1; idx >= 0; idx--)
			{
				func(m_ComponentContextsInOrder[idx]);
			}
		}

	private:
		ecsMap<TypeID, ComponentContextRecord> m_Contexts = {};
		std::vector<ComponentContextBase*> m_ComponentContextsInOrder = {};

		ObserversManager* m_ObserversManager = nullptr;
		int64_t m_IterationIndex = std::numeric_limits<int64_t>::max();

	private:
		inline bool IsIterating() const
		{
			return m_IterationIndex != std::numeric_limits<int64_t>::max();
		}

		inline void DestroyComponentsContexts()
		{
			for (auto& [key, value] : m_Contexts)
			{
				delete value.m_Context;
			}
		}

		void OnSetComponentTypeOrder(ComponentContextRecord& contextRecord)
		{
			if (contextRecord.m_Context != nullptr)
			{
				if (contextRecord.m_OrderIndex != std::numeric_limits<uint64_t>::max()
					&& m_ComponentContextsInOrder[contextRecord.m_OrderIndex] == contextRecord.m_Context)
				{
					m_ComponentContextsInOrder.erase(m_ComponentContextsInOrder.begin() + contextRecord.m_OrderIndex);
				}

				for (uint64_t i = 0; i < m_ComponentContextsInOrder.size(); i++)
				{
					if (m_ComponentContextsInOrder[i]->GetObserverOrder() > contextRecord.m_Order)
					{
						m_ComponentContextsInOrder.insert(m_ComponentContextsInOrder.begin() + i, contextRecord.m_Context);
						contextRecord.m_OrderIndex = i;
						RegenerateIndexes(i + 1);

						if (IsIterating())
						{
							if (m_IterationIndex >= (int64_t)i)
							{
								m_IterationIndex += 1;
							}
							else
							{
								contextRecord.m_Context->SetCanInvokeCreateObservers(false);
							}
						}
						return;
					}
				}

				contextRecord.m_OrderIndex = m_ComponentContextsInOrder.size();
				m_ComponentContextsInOrder.push_back(contextRecord.m_Context);
				if (IsIterating())
				{
					contextRecord.m_Context->SetCanInvokeCreateObservers(false);
				}
			}
		}

		void RegenerateIndexes(uint64_t fromIndex)
		{
			for (uint64_t i = fromIndex; i < m_ComponentContextsInOrder.size(); i++)
			{
				auto context = m_ComponentContextsInOrder[i];
				m_Contexts[context->GetComponentTypeID()].m_OrderIndex = i;
			}
		}
	};
}
