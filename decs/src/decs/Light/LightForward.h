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
	template<TLightComponentConcept... ComponentsTypes>
	class Query;
	template<TLightComponentConcept... ComponentsTypes>
	class MultiQuery;
	template<TLightComponentConcept... ComponentsTypes>
	class IterationArchetypeContext;
	template<TLightComponentConcept...>
	class IterationContainerContext;
	class ContainerIterator;

}