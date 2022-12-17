#pragma once
#include "decspch.h"

#include "Type.h"
#include "ComponentContainer.h"
#include "Observers.h"

namespace decs
{
	class Entity;

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
		virtual TypeID GetComponentTypeID() const = 0;
		virtual void InvokeOnCreateComponent_S(void* component, Entity& entity) = 0;
		virtual void InvokeOnDestroyComponent_S(void* component, Entity& entity) = 0;
		virtual ComponentContextBase* CreateOwnEmptyCopy() = 0;

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

		virtual TypeID GetComponentTypeID() const { return Type<ComponentType>::ID(); }
		virtual BaseComponentAllocator* GetAllocator() override { return &Allocator; }

		virtual void InvokeOnCreateComponent_S(void* component, Entity& entity)override
		{
			for (auto& observer : CreateObservers)
				observer->OnCreateComponent(*reinterpret_cast<ComponentType*>(component), entity);
		}

		virtual void InvokeOnDestroyComponent_S(void* component, Entity& entity)override
		{
			for (auto& observer : DestroyObservers)
				observer->OnDestroyComponent(*reinterpret_cast<ComponentType*>(component), entity);
		}

		void InvokeOnCreateComponent(ComponentType& component, Entity& entity)
		{
			for (auto& observer : CreateObservers)
				observer->OnCreateComponent(component, entity);
		}

		void InvokeOnDestroyComponent(ComponentType& component, Entity& entity)
		{
			for (auto& observer : DestroyObservers)
				observer->OnDestroyComponent(component, entity);
		}

		ComponentContextBase* CreateOwnEmptyCopy() override
		{
			return new ComponentContext<ComponentType>(Allocator.GetChunkCapacity());
		}
	};

}