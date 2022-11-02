#pragma once
#include "decspch.h"

#include "ComponentContainer.h"
#include "Observers.h"

namespace decs
{
	class ComponentContextBase
	{
	public:
		ComponentContextBase()
		{

		}


		virtual ~ComponentContextBase()
		{

		}

		virtual BaseComponentAllocator* GetAllocator() = 0;

		virtual void InvokeOnCreateComponent_S(void* component, const EntityID& entity, Container& container) = 0; // awful but i have no idea how do it better
		virtual void InvokeOnDestroyComponent_S(void* component, const EntityID& entity, Container& container) = 0; //
	};

	template<typename ComponentType>
	class ComponentContext : public ComponentContextBase
	{
	public:
		StableComponentAllocator<ComponentType> Allocator;
		std::vector<CreateComponentObserver<ComponentType>*> CreateObservers;
		std::vector<DestroyComponentObserver<ComponentType>*> DestroyObservers;

	public:
		ComponentContext(const uint64_t& allocatorBucketCapacity) :
			Allocator(allocatorBucketCapacity)
		{

		}

		~ComponentContext()
		{

		}

		virtual BaseComponentAllocator* GetAllocator() override { return &Allocator; }

		virtual void InvokeOnCreateComponent_S(void* component, const EntityID& entity, Container& container)override
		{
			for (auto& observer : CreateObservers)
				observer->OnCreateComponent(*reinterpret_cast<ComponentType*>(component), entity, container);
		}

		virtual void InvokeOnDestroyComponent_S(void* component, const EntityID& entity, Container& container)override
		{
			for (auto& observer : DestroyObservers)
				observer->OnDestroyComponent(*reinterpret_cast<ComponentType*>(component), entity, container);
		}

		void InvokeOnCreateComponent(ComponentType& component, const EntityID& entity, Container& container)
		{
			for (auto& observer : CreateObservers)
				observer->OnCreateComponent(component, entity, container);
		}

		void InvokeOnDestroyComponent(ComponentType& component, const EntityID& entity, Container& container)
		{
			for (auto& observer : DestroyObservers)
				observer->OnDestroyComponent(component, entity, container);
		}
	};

}