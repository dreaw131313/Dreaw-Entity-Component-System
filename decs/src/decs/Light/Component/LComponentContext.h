#pragma once

#include <vector>
#include <functional>

#include "decs/Core/trait.h"
#include "decs/Core/check_cast.h"

#include "LPackedComponentContainer.h"

namespace decs::light
{
	class Entity;

	struct ObserverID final
	{
		template<light_component_concept>
		friend class ComponentObserverFunction;

	private:
		size_t m_Value = std::numeric_limits<size_t>::max();
	};

	template<light_component_concept ComponentType>
	class ComponentObserverFunction
	{
		using ObserverFunctionType = void(const Entity&, ComponentType&);
		using FunctionType = std::function<ObserverFunctionType>;

	public:

		struct ItemRecord
		{
		public:
			FunctionType m_Function{};
			ObserverID m_ID{};
		};

	public:
		template<typename Func>
		ObserverID AddFunction(Func&& func)
		{
			ObserverID id = GenerateID();

			m_Functions.push_back({ FunctionType(func), id });

			return id;
		}

		bool RemoveFunction(ObserverID id)
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
		size_t m_IDGenerator = 0;

	private:
		ObserverID GenerateID()
		{
			ObserverID newID{};
			newID.m_Value = m_IDGenerator;

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

		virtual TypeID GetComponentTypeID() const noexcept = 0;

		virtual IComponentContext* CreateMatchingContext() const = 0;

		virtual IPackedLightComponentContainer* CreatePackedContainer() const = 0;

		virtual void InvokeOnCreateObserver(const Entity& entity, void* compPtr) = 0;

		virtual void InvokeOnDestroyObserver(const Entity& entity, void* compPtr) = 0;

		template<light_component_concept ComponentType>
		void InvokeOnCreateObserver(const Entity& entity, ComponentType& component);

		template<light_component_concept ComponentType>
		void InvokeOnDestroyObserver(const Entity& entity, ComponentType& component);

	};

	template<light_component_concept ComponentType>
	class TComponentContext : public IComponentContext
	{
	public:
		using ObserverFunction = ComponentObserverFunction<ComponentType>;

		ObserverFunction m_OnCreateFunction{};
		ObserverFunction m_OnDestroyFunction{};

	public:
		TComponentContext()
		{

		}

		inline TypeID GetComponentTypeID() const noexcept override
		{
			return Type<ComponentType>::ID();
		}

		IComponentContext* CreateMatchingContext() const override
		{
			return new TComponentContext<ComponentType>();
		}

		IPackedLightComponentContainer* CreatePackedContainer() const override
		{
			return new PackedLightComponentContainer<ComponentType>();
		}

		inline void InvokeOnCreate(const Entity& entity, ComponentType& component)
		{
			m_OnCreateFunction.Invoke(entity, component);
		}

		inline void InvokeOnDestroy(const Entity& entity, ComponentType& component)
		{
			m_OnDestroyFunction.Invoke(entity, component);
		}

		void InvokeOnCreateObserver(const Entity& entity, void* compPtr) override
		{
			m_OnCreateFunction.Invoke(entity, *static_cast<ComponentType*>(compPtr));
		}

		void InvokeOnDestroyObserver(const Entity& entity, void* compPtr) override
		{
			m_OnDestroyFunction.Invoke(entity, *static_cast<ComponentType*>(compPtr));
		}

	};

	template<light_component_concept ComponentType>
	void IComponentContext::InvokeOnCreateObserver(const Entity& entity, ComponentType& component)
	{
		using ContextType = TComponentContext<ComponentType>;
		ContextType* componentCtx = check_cast<ContextType*>(this);
		componentCtx->InvokeOnCreate(entity, component);
	}

	template<light_component_concept ComponentType>
	void IComponentContext::InvokeOnDestroyObserver(const Entity& entity, ComponentType& component)
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
		inline TComponentContext<ComponentType>* GetOrCreateContext()
		{
			auto& context = m_Contexts[Type<pure_type_t<ComponentType>>::ID()];
			if (context == nullptr)
			{
				context = new TComponentContext<ComponentType>();
			}

			return ::decs::check_cast<TComponentContext<ComponentType>*>(context);
		}

		IComponentContext* GetOrCreateContext(const IComponentContext* referenceContext);

	private:
		ecsMap<TypeID, IComponentContext*> m_Contexts{};
	};
}
