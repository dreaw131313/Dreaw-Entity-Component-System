#pragma once
#include "decs/Core/Core.h"
#include "decs/Core/trait.h"

#include "decs/Normal/Component/Component.h"

namespace decs
{
	struct Entity;

	template<typename Observer, typename ComponentType>
	concept component_create_observer_concept = requires
	{
		{ static_cast<void(Observer::*)(const Entity&, ComponentType&)>(&Observer::OnCreateComponent) };
	};

	template<typename Observer, typename ComponentType>
	concept component_destroy_observer_concept = requires
	{
		{ static_cast<void(Observer::*)(const Entity&, ComponentType&)>(&Observer::OnDestroyComponent) };
	};

	template<typename Observer, typename ComponentType>
	concept component_enable_observer_concept = requires
	{
		{ static_cast<void(Observer::*)(const Entity&, ComponentType&)>(&Observer::OnEnableComponent) };
	};

	template<typename Observer, typename ComponentType>
	concept component_disable_observer_concept = requires
	{
		{ static_cast<void(Observer::*)(const Entity&, ComponentType&)>(&Observer::OnDisableComponent) };
	};

}