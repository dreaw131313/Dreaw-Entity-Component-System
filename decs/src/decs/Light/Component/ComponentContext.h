#pragma once

#include <vector>
#include <functional>

#include "decs/Core/trait.h"
#include "decs/Core/check_cast.h"

namespace decs::light
{
	class Entity;

	template<light_component_concept ComponentType>
	class ComponentObserverFunction
	{
		using ObserverFunctionType = void(const Entity&, ComponentType&);
		using FunctionType = std::function<ObserverFunctionType>;
		using IDType = uint32_t;

		struct ItemRecord
		{
		public:
			FunctionType m_Function{};
			IDType m_ID = std::numeric_limits<IDType>::max();
		};

	public:
		template<typename Func>
		uint32_t AddFunction(Func&& func)
		{
			IDType id = GenerateID();

			m_Functions.push_back({ FunctionType(func), id });

			return id;
		}

		bool RemoveFunction(IDType id)
		{
			for (size_t i = 0; i < m_Functions.size(); i++)
			{
				ItemRecord& item = m_Functions[i];
				if (item.m_ID == id)
				{
					m_Functions.erase(m_Functions.begin() + i);
					return true;
				}
			}

			return false;
		}

		void Invoke(const Entity& entity, ComponentType& component)
		{
			for (ItemRecord& record : m_Functions)
			{
				record.m_Function(entity, component);
			}
		}

	private:
		std::vector<ItemRecord> m_Functions{};
		IDType m_IDGenerator = 0;

	private:
		IDType GenerateID()
		{
			IDType newID = m_IDGenerator;
			m_IDGenerator++;
			return newID;
		}
	};

	template<light_component_concept>
	class TComponentContext;

	class IComponentContext
	{
	public:
		virtual ~IComponentContext() = default;

		virtual IComponentContext* CreateMatchingContext() const = 0;

		template<light_component_concept ComponentType>
		void InvoketCreateObserver(const Entity& entity, ComponentType& component);

		template<light_component_concept ComponentType>
		void InvoketDestroyObserver(const Entity& entity, ComponentType& component);

	};

	template<light_component_concept ComponentType>
	class TComponentContext
	{
	public:
		using ObserverFunction = ComponentObserverFunction<ComponentType>;

		ObserverFunction m_OnCreateFunction{};
		ObserverFunction m_OnDestroyFunction{};

	public:
		TComponentContext()
		{

		}

		IComponentContext* CreateMatchingContext() const override
		{
			return new TComponentContext<ComponentType>()
		}

		inline void InvokeOnCreate(const Entity& entity, ComponentType& component)
		{
			m_OnCreateFunction.Invoke(entity, component);
		}

		inline void InvokeOnDestroy(const Entity& entity, ComponentType& component)
		{
			m_OnDestroyFunction.Invoke(entity, component);
		}
	};

	template<light_component_concept ComponentType>
	void IComponentContext::InvoketCreateObserver(const Entity& entity, ComponentType& component)
	{
		using ContextType = TComponentContext<ComponentType>;
		ContextType* componentCtx = check_cast<ContextType*>(this);
		componentCtx->InvokeOnCreate(entity, component);
	}

	template<light_component_concept ComponentType>
	void IComponentContext::InvoketDestroyObserver(const Entity& entity, ComponentType& component)
	{
		using ContextType = TComponentContext<ComponentType>;
		ContextType* componentCtx = check_cast<ContextType*>(this);
		componentCtx->InvokeOnDestroy(entity, component);
	}

	class ComponentContextManager
	{
	public:
		~ComponentContextManager();

		template<light_component_concept ComponentType>
		inline TComponentContext<ComponentType>* GeOrCreatetContext()
		{
			auto& context = m_Contexts[Type<pure_type_t<ComponentType>>::ID()];
			if (context == nullptr)
			{
				context = new TComponentContext<ComponentType>();
			}

			return context;
		}

		IComponentContext* GetOrCreateContext(TypeID componentTypeID, const IComponentContext* referenceContext);

	private:
		ecsMap<TypeID, IComponentContext*> m_Contexts{};
	};
}
