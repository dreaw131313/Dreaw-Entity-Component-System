#pragma once
#include "decs/Normal/Container.h"
#include "decs/Normal/Entity.h"

namespace decs
{
	/// <summary>
	/// Class for iterating over container in the same order like ContainerSerializer.
	/// </summary>
	class ContainerIterator final
	{
	public:
		template<container_iterator_entity_func TCallable>
		void Foreach(Container& container, TCallable&& entityFunc) const
		{
			auto& archetypesMap = container.m_ArchetypesMap;
			auto& archetypesVector = container.m_ArchetypesMap.m_ArchetypeAllocator.GetCreatedArchetypesVector();

			decs::Entity entityBuffer = {};

			for (uint32_t i = 0; i < container.m_EmptyEntities.size(); i++)
			{
				entityBuffer.Set_Internal(container, *container.m_EmptyEntities[i]);
				entityFunc(entityBuffer);
			}

			for (size_t archIdx = 0; archIdx < archetypesVector.size(); archIdx++)
			{
				Archetype& archetype = *archetypesVector[archIdx];
				const auto& entityStorage = archetype.GetEntityStorage();
				uint64_t entitesCount = archetype.EntityCount();
				if (entitesCount > 0)
				{
					for (uint64_t entityIdx = 0; entityIdx < entitesCount; entityIdx++)
					{
						auto entityData = entityStorage.GetEntity(entityIdx);
						if (entityData != nullptr)
						{
							entityBuffer.Set_Internal(container, *entityData);
							entityFunc(entityBuffer);
						}
					}
				}
			}
		}

		/// <summary>
		/// Iterates over entities, and archetypes.First calls ArchetypeFunc with nullptr archetype (for entitieis without archetype). Then iterate over entities without archetype. Then iterate over entities in each archetype. Before iterating over entities in each archetype, calls ArchetypeFunc.
		/// </summary>
		/// <typeparam name="EntityFunc"></typeparam>
		/// <typeparam name="ArchetypeFunc"></typeparam>
		template<
			container_iterator_entity_func EntityFunc,
			container_iterator_archetype_func ArchetypeFunc
		>
		void ForEach(Container& container, EntityFunc&& entityFunc, ArchetypeFunc&& archetypeFunc)
		{
			auto& archetypesMap = container.m_ArchetypesMap;
			auto& archetypesVector = container.m_ArchetypesMap.m_ArchetypeAllocator.GetCreatedArchetypesVector();

			archetypeFunc(nullptr);
			decs::Entity entityBuffer = {};

			for (uint32_t i = 0; i < container.m_EmptyEntities.size(); i++)
			{
				entityBuffer.Set_Internal(container, *container.m_EmptyEntities[i]);
				entityFunc(entityBuffer);
			}

			for (size_t archIdx = 0; archIdx < archetypesVector.size(); archIdx++)
			{
				Archetype& archetype = archetypesVector[archIdx];
				const auto& entityStorage = archetype.GetEntityStorage();
				archetypeFunc(&archetype);

				uint64_t entitesCount = archetype.EntityCount();
				if (entitesCount > 0)
				{
					for (uint64_t entityIdx = 0; entityIdx < entitesCount; entityIdx++)
					{
						auto entityData = entityStorage.GetEntity(entityIdx);
						if (entityData != nullptr)
						{
							entityBuffer.Set_Internal(container, *entityData);
							entityFunc(entityBuffer);
						}
					}
				}
			}
		}

	};
}
