#pragma once
#include "decs/Core/Core.h"

#include "decs/Normal/Component/Component.h"

namespace decs
{
	struct Entity;

	class CreateEntityObserver
	{
	public:
		virtual ~CreateEntityObserver() = default;

		/// <summary>
		/// In this method modifying or changing component setup of any entity (except entity passed to this function) is forbidden because it lead to undefined behavior. New entities can be created. Existing entities can be destroyed.
		/// </summary>
		/// <param name="entity"></param>
		virtual void OnCreateEntity(const Entity& entity) = 0;
	};

	class DestroyEntityObserver
	{
	public:
		virtual ~DestroyEntityObserver() = default;

		virtual void OnDestroyEntity(const Entity& entity) = 0;
	};

	class EnableEntityObserver
	{
	public:
		virtual ~EnableEntityObserver() = default;

		virtual void OnEnableEntity(const Entity& entity) = 0;
	};

	class DisableEntityObserver
	{
	public:
		virtual ~DisableEntityObserver() = default;

		virtual void OnDisableEntity(const Entity& entity) = 0;;
	};

	template<component_concept ComponentType>
	class CreateComponentObserver
	{
	public:
		virtual ~CreateComponentObserver() = default;

		/// <summary>
		/// In this method setup of components of existing entites (except entity passed to this function) should not be changed, because it can lead to undefined behavior when this method is called in function "Container::InvokeEntitesOnCreateListeners". New entites can be created and it component setup can be changed.
		/// </summary>
		/// <param name="component"></param>
		/// <param name="entity"></param>
		virtual void OnCreateComponent(ComponentType& component, const Entity& entity) = 0;
	};

	template<component_concept ComponentType>
	class DestroyComponentObserver
	{
	public:
		virtual ~DestroyComponentObserver() = default;

		virtual void OnDestroyComponent(ComponentType& component, const Entity& entity) = 0;
	};

	template<component_concept ComponentType>
	class EnableComponentObserver
	{
	public:
		virtual ~EnableComponentObserver() = default;

		virtual void OnEnableComponent(ComponentType& component, const Entity& entity) = 0;
	};

	template<component_concept ComponentType>
	class DisableComponentObserver
	{
	public:
		virtual ~DisableComponentObserver() = default;

		virtual void OnDisableComponent(ComponentType& component, const Entity& entity) = 0;
	};

	template<component_concept ComponentType>
	struct ComponentObserversGroup
	{
	public:
		CreateComponentObserver<ComponentType>* m_CreateObserver = nullptr;
		DestroyComponentObserver<ComponentType>* m_DestroyObserver = nullptr;
		EnableComponentObserver<ComponentType>* m_EnableObserver = nullptr;
		DisableComponentObserver<ComponentType>* m_DisableObserver = nullptr;
	};

}