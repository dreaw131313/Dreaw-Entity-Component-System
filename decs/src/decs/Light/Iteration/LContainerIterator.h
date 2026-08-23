#pragma once
#include "decs/Light/LContainer.h"
#include "decs/Light/LEntity.h"

namespace decs::light
{
	/// <summary>
	/// Class for iterating over container in the same order like ContainerSerializer.
	/// </summary>
	class ContainerIterator final
	{
	public:
		template<typename Callable>
		void Foreach(Container& container, Callable&& callable) const
		{
			auto& archetypesMap = container.m_ArchetypesMap;

			Entity entityBuffer = {};

			for (uint32_t i = 0; i < container.m_EmptyEntities.size(); i++)
			{
				entityBuffer.Set_Internal(*container.m_EmptyEntities[i]);
				callable(entityBuffer);
			}

			auto archetypesSpan = container.m_ArchetypesMap.m_ArchetypeAllocator.GetCreatedArchetypes();
			for (auto archetype : archetypesSpan)
			{
				const auto& entities = archetype->GetEntities();
				uint64_t entitesCount = archetype->EntityCount();
				for (uint64_t entityIdx = 0; entityIdx < entitesCount; entityIdx++)
				{
					auto archetypeEntityData = entities.Get(entityIdx);
					if (archetypeEntityData != nullptr)
					{
						entityBuffer.Set_Internal(*archetypeEntityData);
						callable(entityBuffer);
					}
				}
			}

		}
	};
}
