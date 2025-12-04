#pragma once
#include "decs\Core.h"
#include "decs\ComponentContainers\PackedContainer.h"
#include "decs\Archetypes\Archetype.h"
#include "decs\Archetypes\ArchetypesMap.h"

namespace decs
{
	template<typename T>
	concept TComponentOrTagConcept = TComponentConcept<T> || TTagConcept<T>;


	template<TComponentConcept... ComponentsTypes>
	class IterationArchetypeContext
	{
	public:
		template<typename TComponent>
		using TPackedContainer = StablePackedContainer<drop_const_t<TComponent>>*;
		using ContainersTuple = std::tuple<TPackedContainer<ComponentsTypes>...>;

	public:
		inline static constexpr uint64_t s_ComponentCount = sizeof...(ComponentsTypes);

	public:
		inline const Archetype* GetArchetype() const noexcept
		{
			return m_Archetype;
		}

		inline uint64_t GetEntityCount() const
		{
			return m_Archetype->EntityCount();
		}

		inline int64_t GetCachedEntityCount() const
		{
			return m_CachedEntityCount;
		}

		inline const ContainersTuple& GetContainersTuple() const noexcept
		{
			return m_ContainersTuple;
		}

		bool Initialize(const Archetype* archetype)
		{
			DECS_ASSERT(archetype != nullptr, "Archetype must not be nullptr!");

			m_Archetype = archetype;

			return CreatePackedContainersTuple<ComponentsTypes...>();
		}

		inline void ValidateCachedEntityCount()
		{
			m_CachedEntityCount = m_Archetype->EntityCount();
		}

	private:
		const Archetype* m_Archetype = nullptr;
		ContainersTuple m_ContainersTuple{};
		int64_t m_CachedEntityCount = 0;

	private:
		template<typename T = void, typename... Args>
		bool CreatePackedContainersTuple()
		{
			constexpr uint64_t compIdx = sizeof...(ComponentsTypes) - sizeof...(Args) - 1;

			TPackedContainer<drop_const_t<T>> componentContainer = m_Archetype->GetTypePackedContainer<drop_const_t<T>>();
			if (componentContainer == nullptr)
			{
				return false;
			}

			std::get<TPackedContainer<T>>(m_ContainersTuple) = componentContainer;

			if constexpr (sizeof...(Args) == 0) return true;

			return CreatePackedContainersTuple<Args...>();
		}

		template<>
		bool CreatePackedContainersTuple<void>()
		{
			return true;
		}
	};


	template<TComponentConcept... ComponentsTypes>
	class IterationContainerContext
	{
	public:
		using ArchetypeContextType = IterationArchetypeContext<ComponentsTypes...>;

	public:
		std::vector<ArchetypeContextType> m_ArchetypesContexts;
		ecsSet<const Archetype*> m_ContainedArchetypes;
		Container* m_Container = nullptr;
		uint64_t m_ArchetypesCountDirty = 0;
		bool m_bIsEnabled = true;

	public:
		IterationContainerContext()
		{

		}

		IterationContainerContext(Container* container, bool bIsEnabled = true):
			m_Container(container),
			m_bIsEnabled(bIsEnabled)
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
		}

		bool Contain(const decs::Entity& entity)
		{
			return m_ContainedArchetypes.find(entity.GetArchetype()) != m_ContainedArchetypes.end();
		}

		void ValidateCachedEntityCount()
		{
			const uint64_t ctxCount = m_ArchetypesContexts.size();
			for (uint64_t i = 0; i < ctxCount; i++)
			{
				m_ArchetypesContexts[i].ValidateCachedEntityCount();
			}
		}

		uint64_t GetEntityCount()
		{
			uint64_t entityCount = 0;

			for (const auto& archetypeCtx : m_ArchetypesContexts)
			{
				entityCount += archetypeCtx.GetEntityCount();
			}

			return entityCount;
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
			if (!ContainArchetype(&archetype) && archetype.GetComponentAndTagCount())
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

					if (context.Initialize(&archetype))
					{
						m_ContainedArchetypes.insert(&archetype);
					}
					else
					{
						m_ArchetypesContexts.pop_back();
					}
				}
			}
		}

		void FetchArchetypesFromArchetypesGroup(
			ArchetypesGroupByOneType* group,
			const TypeGroup<ComponentsTypes...>& includes,
			const std::vector<TypeID>& without,
			const std::vector<TypeID>& withAnyOf,
			const std::vector<TypeID>& withAll,
			uint64_t minComponentsCount
		)
		{
			if (group == nullptr) return;
			uint64_t maxComponentCountsInGroup = group->MaxComponentsCount();

			for (uint64_t i = minComponentsCount; i <= maxComponentCountsInGroup; i++)
			{
				auto archetypesToCheckPtr = group->GetArchetypesWithComponentsCount(i);
				if (archetypesToCheckPtr != nullptr)
				{
					for (auto archetype : *archetypesToCheckPtr)
					{
						TryAddArchetypeFromGroup(*archetype, includes, without, withAnyOf, withAll);
					}
				}
			}
		}

		void AddingArchetypesWithCheckingOnlyNewArchetypes(
			ArchetypesMap& map,
			uint64_t startArchetypesIndex,
			uint64_t minRequiredComponentsCount,
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
				if (arch.GetComponentAndTagCount() >= minRequiredComponentsCount)
				{
					TryAddArchetypeFromGroup(arch, includes, without, withAnyOf, withAll);
				}
			}
		}
	};
}