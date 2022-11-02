#pragma once
#include "decspch.h"

#include "Core.h"


namespace decs
{
	class Entity;

	class CreateEntityObserver
	{
	public:
		virtual void OnCreateEntity(const EntityID& entity, Container& container) = 0;
	};

	class DestroyEntityObserver
	{
	public:
		virtual void OnDestroyEntity(const EntityID& entity, Container& container) = 0;
	};

	class ActivateEntityObserver
	{
		virtual void OnSetEntityActive(Entity& entity) = 0;
	};

	class DeactivateEntityObserver
	{
		virtual void OnSetEntityInActive(Entity& entity) = 0;;
	};

	template<typename ComponentType>
	class CreateComponentObserver
	{
	public:
		virtual void OnCreateComponent(ComponentType& component, const EntityID& entity, Container& container) = 0;
	};

	template<typename ComponentType>
	class DestroyComponentObserver
	{
	public:
		virtual void OnDestroyComponent(ComponentType& component, const EntityID& entity, Container& container) = 0;
	};


}