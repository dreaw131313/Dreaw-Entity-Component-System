#pragma once

#include "decs/Core/trait.h"

namespace decs::light
{
	class EntityData;
	class EntityManager;
	class Archetype;
	class ArchetypesMap;
	class Container;
	class Entity;
	class Iteration;
	template<light_component_or_filter_concept... ComponentsTypes>
	class Query;
	template<light_component_or_filter_concept... ComponentsTypes>
	class MultiQuery;
	template<light_component_or_filter_concept... ComponentsTypes>
	class IterationArchetypeContext;
	template<light_component_or_filter_concept...>
	class IterationContainerContext;
	class ContainerIterator;

}