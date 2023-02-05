#pragma once
#include "Core.h"
#include "ComponentContainers\PackedContainer.h"
#include "Archetypes\Archetype.h"
#include "Archetypes\ArchetypesMap.h"

namespace decs
{
	template<uint64_t elementsCount>
	class ArchetypeContext
	{
	public:
		Archetype* Arch = nullptr;
		uint64_t m_EntitiesCount = 0;
		PackedContainerBase* m_Containers[elementsCount] = { nullptr };

	public:
		inline void ValidateEntitiesCount() { m_EntitiesCount = Arch->EntitiesCount(); }
	};


	template<typename ArchetypeContextType, typename... ComponentsTypes>
	class ContainerContext
	{
	public:
		Container* m_Container = nullptr;
		std::vector<ArchetypeContextType> m_ArchetypesContexts;
		ecsSet<Archetype*> m_ContainedArchetypes;
		uint64_t m_ArchetypesCountDirty = 0;

	public:
		ContainerContext()
		{

		}

		ContainerContext(Container* container) :
			m_Container(container)
		{

		}

		void Invalidate()
		{
			m_ArchetypesContexts.clear();
			m_ContainedArchetypes.clear();
			m_ArchetypesCountDirty = 0;
		}

		void Fetch(
			const TypeGroup<ComponentsTypes...>& includes,
			const std::vector<TypeID>& without,
			const std::vector<TypeID>& withAnyOf,
			const std::vector<TypeID>& withAll,
			const uint64_t& minComponentsCount
		)
		{
			uint64_t containerArchetypesCount = m_Container->m_ArchetypesMap.ArchetypesCount();
			if (m_ArchetypesCountDirty != containerArchetypesCount)
			{
				uint64_t newArchetypesCount = containerArchetypesCount - m_ArchetypesCountDirty;

				ArchetypesMap& map = m_Container->m_ArchetypesMap;
				uint64_t maxComponentsInArchetype = map.MaxNumberOfTypesInArchetype();
				if (maxComponentsInArchetype < minComponentsCount) return;

				if (newArchetypesCount > m_ArchetypesContexts.size())
				{
					// performing normal finding of archetypes
					auto group = GetBestArchetypesGroup(includes);
					FetchArchetypesFromArchetypesGroup(group, includes, without, withAnyOf, withAll, minComponentsCount);
				}
				else
				{
					// checking only new archetypes:
					AddingArchetypesWithCheckingOnlyNewArchetypes(map, m_ArchetypesCountDirty, minComponentsCount, includes, without, withAnyOf, withAll);
				}

				m_ArchetypesCountDirty = containerArchetypesCount;
			}

			uint64_t archetypesCount = m_ArchetypesContexts.size();
			for (uint64_t i = 0; i < archetypesCount; i++)
			{
				m_ArchetypesContexts[i].ValidateEntitiesCount();
			}
		}

	private:

		inline bool ContainArchetype(Archetype* arch) const { return m_ContainedArchetypes.find(arch) != m_ContainedArchetypes.end(); }

		ArchetypesGroupByOneType* GetBestArchetypesGroup(const TypeGroup<ComponentsTypes...>& includes)
		{
			auto& groupsMap = m_Container->m_ArchetypesMap.m_ArchetypesGroupedByOneType;

			uint64_t bestArchetypesCount = std::numeric_limits<uint64_t>::max();
			ArchetypesGroupByOneType* bestGroup = nullptr;

			for (uint64_t i = 0; i < includes.Size(); i++)
			{
				auto it = groupsMap.find(includes[i]);
				if (it != groupsMap.end())
				{
					uint64_t bufforGroupArchetypesCount = it->second->ArchetypesCount();
					if (bufforGroupArchetypesCount < bestArchetypesCount)
					{
						bestArchetypesCount = bufforGroupArchetypesCount;
						bestGroup = it->second;
					}
				}
			}

			return bestGroup;
		}

		void TryAddArchetypeFromGroup(
			Archetype& archetype,
			const TypeGroup<ComponentsTypes...>& includes,
			const std::vector<TypeID>& without,
			const std::vector<TypeID>& withAnyOf,
			const std::vector<TypeID>& withAll
		)
		{
			if (!ContainArchetype(&archetype) && archetype.ComponentsCount())
			{
				// without test
				{
					uint64_t excludeCount = without.size();
					for (int i = 0; i < excludeCount; i++)
					{
						if (archetype.ContainType(without[i]))
						{
							return;
						}
					}
				}

				// with any test
				{
					uint64_t requiredAnyCount = withAnyOf.size();
					bool containRequiredAny = requiredAnyCount == 0;

					for (int i = 0; i < requiredAnyCount; i++)
					{
						if (archetype.ContainType(withAnyOf[i]))
						{
							containRequiredAny = true;
							break;
						}
					}
					if (!containRequiredAny) return;
				}

				// required all test
				{
					uint64_t requiredAllCount = withAll.size();

					for (int i = 0; i < requiredAllCount; i++)
					{
						if (!archetype.ContainType(withAll[i]))
						{
							return;
						}
					}
				}

				// includes
				{
					ArchetypeContextType& context = m_ArchetypesContexts.emplace_back();

					for (uint32_t typeIdx = 0; typeIdx < includes.Size(); typeIdx++)
					{
						auto typeIDIndex = archetype.FindTypeIndex(includes.IDs()[typeIdx]);
						if (typeIDIndex == Limits::MaxComponentCount)
						{
							m_ArchetypesContexts.pop_back();
							return;
						}

						auto& packedContainer = archetype.m_TypeData[typeIDIndex].m_PackedContainer;
						context.m_Containers[typeIdx] = packedContainer;
					}
					m_ContainedArchetypes.insert(&archetype);
					context.Arch = &archetype;
				}
			}
		}

		void FetchArchetypesFromArchetypesGroup(
			ArchetypesGroupByOneType* group,
			const TypeGroup<ComponentsTypes...>& includes,
			const std::vector<TypeID>& without,
			const std::vector<TypeID>& withAnyOf,
			const std::vector<TypeID>& withAll,
			const uint64_t& minComponentsCount
		)
		{
			if (group == nullptr) return;
			uint64_t maxComponentCountsInGroup = group->MaxComponentsCount();

			for (uint64_t i = minComponentsCount; i <= maxComponentCountsInGroup; i++)
			{
				std::vector<Archetype*>& archetypesToCheck = group->GetArchetypesWithComponentsCount(i);
				for (auto& archetype : archetypesToCheck)
				{
					TryAddArchetypeFromGroup(*archetype, includes, without, withAnyOf, withAll);
				}
			}
		}

		void AddingArchetypesWithCheckingOnlyNewArchetypes(
			ArchetypesMap& map,
			const uint64_t& startArchetypesIndex,
			const uint64_t& minRequiredComponentsCount,
			const TypeGroup<ComponentsTypes...>& includes,
			const std::vector<TypeID>& without,
			const std::vector<TypeID>& withAnyOf,
			const std::vector<TypeID>& withAll
		)
		{
			auto& archetypes = map.m_Archetypes;
			uint64_t archetypesCount = map.m_Archetypes.Size();
			for (uint64_t i = startArchetypesIndex; i < archetypesCount; i++)
			{
				Archetype& arch = archetypes[i];
				if (arch.ComponentsCount() < minRequiredComponentsCount)
				{
					TryAddArchetypeFromGroup(arch, includes, without, withAnyOf, withAll);
				}
			}
		}

	};
}