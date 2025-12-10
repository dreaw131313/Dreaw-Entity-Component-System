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
		uint32_t m_StableComponentChunkSize = 0;
		int m_Order = 0;
	};

	class ComponentContextsManager final
	{
		friend class Container;
		friend class ArchetypesMap;
	public:
		NON_COPYABLE(ComponentContextsManager);
		NON_MOVEABLE(ComponentContextsManager);

		ComponentContextsManager(uint32_t defaultStableComponentChunkSize) :
			m_DefaultStableComponentChunkSize(defaultStableComponentChunkSize)
		{

		}

		~ComponentContextsManager()
		{
			DestroyComponentsContexts();
		}

		template<TComponentConcept TComponent>
		ComponentContext<TComponent>* GetOrCreateComponentContext()
		{
			TYPE_ID_CONSTEXPR TypeID id = Type<TComponent>::ID();

			auto& contextRecord = m_Contexts[id];
			if (contextRecord.m_Context == nullptr)
			{
				ComponentContext<TComponent>* context = new ComponentContext<TComponent>(
					contextRecord.m_Order,
					contextRecord.m_StableComponentChunkSize >= 0 ? contextRecord.m_StableComponentChunkSize : m_DefaultStableComponentChunkSize
				);
				contextRecord.m_Context = context;

				OnSetComponentTypeOrder(contextRecord);
				return context;
			}
			else
			{
				ComponentContext<TComponent>* containedContext = dynamic_cast<ComponentContext<TComponent>*>(contextRecord.m_Context);

				if (containedContext == nullptr)
				{
					std::string errorMessage = "decs::Container contains component context with id " + std::to_string(id) + " to type other than " + Type<TComponent>::Name();
					throw std::runtime_error(errorMessage.c_str());
				}
				return containedContext;
			}
		}

		ComponentContextBase* GetOrCreateComponentContextFromOtherContext(ComponentContextBase* other)
		{
			auto& contextRecord = m_Contexts[other->GetComponentTypeID()];
			if (contextRecord.m_Context == nullptr)
			{
				ComponentContextBase* newContext = other->Clone(
					contextRecord.m_Order,
					contextRecord.m_StableComponentChunkSize >= 0 ? contextRecord.m_StableComponentChunkSize : m_DefaultStableComponentChunkSize
				);
				contextRecord.m_Context = newContext;

				OnSetComponentTypeOrder(contextRecord);
				return newContext;
			}
			else
			{
				return contextRecord.m_Context;
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

		template<TComponentConcept TComponent>
		bool SetComponentOrder(int order)
		{
			GetOrCreateComponentContext<TComponent>();
			return SetComponentOrder(Type<TComponent>::ID(), order);
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

		bool SetComponentChunkSize(TypeID typeID, uint32_t chunkSize)
		{
			if (chunkSize == 0)
			{
				return false;
			}

			auto& contextRecord = m_Contexts[typeID];

			if (contextRecord.m_Context != nullptr)
			{
				return false;
			}

			contextRecord.m_StableComponentChunkSize = chunkSize;
			return true;
		}

		template<TComponentConcept TComponentType>
		bool SetComponentChunkSize(uint32_t chunkSize)
		{
			return SetComponentChunkSize(Type<TComponentType>::ID(), chunkSize);
		}


		uint64_t GetComponentChunkSize(TypeID typeID)
		{
			auto it = m_Contexts.find(typeID);
			if (it == m_Contexts.end())
			{
				return 0;
			}

			auto& contextRecord = it->second;

			if (contextRecord.m_Context == nullptr)
			{
				return contextRecord.m_StableComponentChunkSize;
			}
			else
			{
				return contextRecord.m_Context->GetStableContainer()->GetChunkSize();
			}
		}

		template<TComponentConcept TComponentType>
		uint64_t GetComponentChunkSize()
		{
			return GetComponentChunkSize(Type<TComponentType>::ID());
		}

		void SetDefaultStableComponentChunkSize(uint32_t chunkSize)
		{
			m_DefaultStableComponentChunkSize = chunkSize;
		}

		void ClearStableContainers()
		{
			for (auto compCtx : m_ComponentContextsInOrder)
			{
				if (compCtx != nullptr)
				{
					compCtx->ClearStableContainer();
				}
			}
		}
	private:
		ecsMap<TypeID, ComponentContextRecord> m_Contexts = {};
		std::vector<ComponentContextBase*> m_ComponentContextsInOrder = {};

		int64_t m_IterationIndex = std::numeric_limits<int64_t>::max();
		uint32_t m_DefaultStableComponentChunkSize = 1000;

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
			m_Contexts.clear();
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

						if (IsIterating() && m_IterationIndex >= static_cast<int64_t>(i))
						{
							m_IterationIndex += 1;
						}
						return;
					}
				}

				contextRecord.m_OrderIndex = m_ComponentContextsInOrder.size();
				m_ComponentContextsInOrder.push_back(contextRecord.m_Context);
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
