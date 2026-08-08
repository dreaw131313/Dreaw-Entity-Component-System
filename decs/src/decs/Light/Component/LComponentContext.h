#pragma once

#include "decs/Core/trait.h"
#include "decs/Core/check_cast.h"
#include "decs/Core/ObserverFunction.h"

#include "LPackedComponentContainer.h"


namespace decs::light
{
	struct Entity;


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
		using ObserverFunction = ::decs::TObserverFunction<void(const Entity&, ComponentType&)>;

		ObserverFunction m_CreateObservers{};
		ObserverFunction m_DestroyObservers{};
		ObserverFunction m_OnSetFunction{};

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
			m_CreateObservers.Invoke(entity, component);
		}

		inline void InvokeOnDestroy(const Entity& entity, ComponentType& component)
		{
			m_DestroyObservers.Invoke(entity, component);
		}

		inline void InvokeOnSet(const Entity& entity, ComponentType& component)
		{
			m_OnSetFunction.Invoke(entity, component);
		}

		void InvokeOnCreateObserver(const Entity& entity, void* compPtr) override
		{
			m_CreateObservers.Invoke(entity, *static_cast<ComponentType*>(compPtr));
		}

		void InvokeOnDestroyObserver(const Entity& entity, void* compPtr) override
		{
			m_DestroyObservers.Invoke(entity, *static_cast<ComponentType*>(compPtr));
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
		ecsHashMap<TypeID, IComponentContext*> m_Contexts{};
	};
}
