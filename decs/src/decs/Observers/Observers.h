#pragma once
#include "decs\Core.h"

namespace decs
{
	class Entity;

	class CreateEntityObserver
	{
	public:
		virtual ~CreateEntityObserver() = default;

		virtual void OnCreateEntity(Entity& entity) = 0;
	};

	class DestroyEntityObserver
	{
	public:
		virtual ~DestroyEntityObserver() = default;

		virtual void OnDestroyEntity(Entity& entity) = 0;
	};

	class ActivateEntityObserver
	{
	public:
		virtual ~ActivateEntityObserver() = default;

		virtual void OnSetEntityActive(Entity& entity) = 0;
	};

	class DeactivateEntityObserver
	{
	public:
		virtual ~DeactivateEntityObserver() = default;

		virtual void OnSetEntityInactive(Entity& entity) = 0;;
	};

	template<typename ComponentType>
	class CreateComponentObserver
	{
	public:
		virtual ~CreateComponentObserver() = default;

		virtual void OnCreateComponent(component_type<ComponentType>::Type& component, Entity& entity) = 0;
	};

	template<typename ComponentType>
	class DestroyComponentObserver
	{
	public:
		virtual ~DestroyComponentObserver() = default;

		virtual void OnDestroyComponent(component_type<ComponentType>::Type& component, Entity& entity) = 0;
	};

	template<typename ComponentType>
	class ActivateEntityComponentObserver
	{
	public:
		virtual ~ActivateEntityComponentObserver() = default;

		virtual void OnSetEntityActive(component_type<ComponentType>::Type& component, Entity& entity) = 0;
	};

	template<typename ComponentType>
	class EntityDisableComponentObserver
	{
	public:
		virtual ~EntityDisableComponentObserver() = default;

		virtual void OnDisableEntity(component_type<ComponentType>::Type& component, Entity& entity) = 0;
	};

}