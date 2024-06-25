#pragma once
#include "decs\Core.h"

namespace decs
{
	class Entity;

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

		virtual void OnEnableComponent(const Entity& entity) = 0;
	};

	class DisableEntityObserver
	{
	public:
		virtual ~DisableEntityObserver() = default;

		virtual void OnDisableComponent(const Entity& entity) = 0;;
	};

	template<typename TComponent>
	class CreateComponentObserver
	{
	public:
		virtual ~CreateComponentObserver() = default;

		/// <summary>
		/// In this method setup of components of existing entites (except entity passed to this function) should not be changed, because it can lead to undefined behavior when this method is called in function "Container::InvokeEntitesOnCreateListeners". New entites can be created and it component setup can be changed.
		/// </summary>
		/// <param name="component"></param>
		/// <param name="entity"></param>
		virtual void OnCreateComponent(TComponent& component, const Entity& entity) = 0;
	};

	template<typename TComponent>
	class DestroyComponentObserver
	{
	public:
		virtual ~DestroyComponentObserver() = default;

		virtual void OnDestroyComponent(TComponent& component, const Entity& entity) = 0;
	};

	template<typename TComponent>
	class EnableComponentObserver
	{
	public:
		virtual ~EnableComponentObserver() = default;

		virtual void OnEnableComponent(TComponent& component, const Entity& entity) = 0;
	};

	template<typename TComponent>
	class DisableComponentObserver
	{
	public:
		virtual ~DisableComponentObserver() = default;

		virtual void OnDisableComponent(TComponent& component, const Entity& entity) = 0;
	};


	template<typename TComponent>
	struct ComponentObserversGroup
	{
	public:
		CreateComponentObserver<TComponent>* m_CreateObserver = nullptr;
		DestroyComponentObserver<TComponent>* m_DestroyObserver = nullptr;
		EnableComponentObserver<TComponent>* m_EnableObserver = nullptr;
		DisableComponentObserver<TComponent>* m_DisableObserver = nullptr;
	};

}