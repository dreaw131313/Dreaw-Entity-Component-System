#pragma once
#include "decs\Core.h"

namespace decs
{
	class Entity;

	class CreateEntityObserver
	{
	public:
		virtual void OnCreateEntity(Entity& entity) = 0;
	};

	class DestroyEntityObserver
	{
	public:
		virtual void OnDestroyEntity(Entity& entity) = 0;
	};

	class ActivateEntityObserver
	{
	public:
		virtual void OnSetEntityActive(Entity& entity) = 0;
	};

	class DeactivateEntityObserver
	{
	public:
		virtual void OnSetEntityInactive(Entity& entity) = 0;;
	};

	template<typename ComponentType>
	class CreateComponentObserver
	{
	public:
		virtual void OnCreateComponent(ComponentType& component, Entity& entity) = 0;
	};

	template<typename ComponentType>
	class DestroyComponentObserver
	{
	public:
		virtual void OnDestroyComponent(ComponentType& component, Entity& entity) = 0;
	};


	template<typename ComponentType>
	class CreateComponentObserver<stable<ComponentType>>
	{
	public:
		virtual void OnCreateComponent(ComponentType& component, Entity& entity) = 0;
	};

	template<typename ComponentType>
	class DestroyComponentObserver<stable<ComponentType>>
	{
	public:
		virtual void OnDestroyComponent(ComponentType& component, Entity& entity) = 0;
	};
}