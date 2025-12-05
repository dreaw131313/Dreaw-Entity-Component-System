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
	struct QueryFiltersConfig
	{
	public:
		using TypeGroupType = TypeGroup<drop_const_t<ComponentsTypes>...>;

	public:
		const TypeGroupType& GetIncludes() const
		{
			return m_Includes;
		}

		const std::vector<TypeID>& GetWithoutFilter() const noexcept
		{
			return m_Without;
		}

		const std::vector<TypeID>& GetWithAnyFilter() const noexcept
		{
			return 	m_WithAnyOf;
		}

		const std::vector<TypeID>& GetWithAllFilter() const noexcept
		{
			return m_WithAll;
		}

		inline uint64_t GetMinComponentsCount() const
		{
			uint64_t includesCount = sizeof...(ComponentsTypes);
			if (m_WithAnyOf.size() > 0) includesCount += 1;
			return sizeof...(ComponentsTypes) + m_WithAll.size();
		}

		template<TComponentOrTagConcept... WithoutTypes>
		void Without()
		{
			if constexpr (sizeof...(WithoutTypes) == 0)
			{
				m_Without.clear();
			}
			else
			{
				m_Without.resize(sizeof...(WithoutTypes));
				find_type_ids<drop_const_t<WithoutTypes>...>(m_Without.data());
			}
		}

		template<TComponentOrTagConcept... WithAnyTypes>
		void WithAny()
		{
			if constexpr (sizeof...(WithAnyTypes) == 0)
			{
				m_WithAnyOf.clear();
			}
			else
			{
				m_WithAnyOf.resize(sizeof...(WithAnyTypes));
				find_type_ids<drop_const_t<WithAnyTypes>...>(m_WithAnyOf.data());
			}
		}

		template<TComponentOrTagConcept... WithTypes>
		void With()
		{
			if constexpr (sizeof...(WithTypes) == 0)
			{
				m_WithAll.clear();
			}
			else
			{
				m_WithAll.resize(sizeof...(WithTypes));
				find_type_ids<drop_const_t<WithTypes>...>(m_WithAll.data());
			}
		}

		[[nodiscard]] bool Clear()
		{
			bool bResult = false;
			if (m_WithAll.size() > 0)
			{
				bResult = true;
				m_WithAll.clear();
			}
			if (m_WithAnyOf.size() > 0)
			{
				bResult = true;
				m_WithAnyOf.clear();
			}
			if (m_Without.size() > 0)
			{
				bResult = true;
				m_Without.clear();
			}

			return bResult;
		}

	private:
		TypeGroupType m_Includes = {};
		std::vector<TypeID> m_Without{};
		std::vector<TypeID> m_WithAnyOf{};
		std::vector<TypeID> m_WithAll{};
	};

	template<TComponentConcept... ComponentsTypes>
	class IterationArchetypeContext
	{
	public:
		template<typename TComponent>
		using TPackedContainer = StablePackedContainer<drop_const_t<TComponent>>;
		using ContainersTuple = std::tuple<TPackedContainer<ComponentsTypes>*...>;

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

	private:
		const Archetype* m_Archetype = nullptr;
		ContainersTuple m_ContainersTuple{};

	private:
		template<typename T = void, typename... Args>
		bool CreatePackedContainersTuple()
		{
			constexpr uint64_t compIdx = sizeof...(ComponentsTypes) - sizeof...(Args) - 1;

			TPackedContainer<drop_const_t<T>>* componentContainer = m_Archetype->GetTypePackedContainer<drop_const_t<T>>();
			if (componentContainer == nullptr)
			{
				return false;
			}

			std::get<TPackedContainer<T>*>(m_ContainersTuple) = componentContainer;

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
		using QueryFilterConfigType = QueryFiltersConfig<drop_const_t<ComponentsTypes>...>;

	public:
		std::vector<ArchetypeContextType> m_ArchetypesContexts{};
		ecsSet<const Archetype*> m_ContainedArchetypes{};
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

		inline Container* GetContainer() const
		{
			return m_Container;
		}

		const std::vector<ArchetypeContextType>& GetArchetypeContexts() const noexcept
		{
			return m_ArchetypesContexts;
		}

		const ecsSet<const Archetype*>& GetArchetypes() const
		{
			return m_ContainedArchetypes;
		}

		void Clear()
		{
			m_ArchetypesContexts.clear();
			m_ContainedArchetypes.clear();
			m_ArchetypesCountDirty = 0;
		}

		void SetContainer(Container* container)
		{
			Clear();
			m_Container = container;
		}

		void Fetch(const QueryFilterConfigType& filter)
		{
			uint64_t minComponentsCount = filter.GetMinComponentsCount();

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
					auto group = GetBestArchetypesGroup(filter.GetIncludes());
					FetchArchetypesFromArchetypesGroup(group, filter);
				}
				else
				{
					// checking only new archetypes:
					AddingArchetypesWithCheckingOnlyNewArchetypes(map, m_ArchetypesCountDirty, filter);
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

		void TryAddArchetypeFromGroup(Archetype& archetype, const QueryFilterConfigType& filter)
		{
			if (!ContainArchetype(&archetype) && archetype.GetComponentAndTagCount())
			{
				// without test
				{
					auto& without = filter.GetWithoutFilter();

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
					auto& withAnyOf = filter.GetWithAnyFilter();

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
					auto& withAll = filter.GetWithAllFilter();
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
					ArchetypeContextType context{};
					if (context.Initialize(&archetype))
					{
						m_ContainedArchetypes.insert(&archetype);
						m_ArchetypesContexts.push_back(context);
					}
				}
			}
		}

		void FetchArchetypesFromArchetypesGroup(ArchetypesGroupByOneType* group, const QueryFilterConfigType& filter)
		{
			if (group == nullptr) return;
			uint64_t maxComponentCountsInGroup = group->MaxComponentsCount();

			for (uint64_t i = filter.GetMinComponentsCount(); i <= maxComponentCountsInGroup; i++)
			{
				auto archetypesToCheckPtr = group->GetArchetypesWithComponentsCount(i);
				if (archetypesToCheckPtr != nullptr)
				{
					for (auto archetype : *archetypesToCheckPtr)
					{
						TryAddArchetypeFromGroup(*archetype, filter);
					}
				}
			}
		}

		void AddingArchetypesWithCheckingOnlyNewArchetypes(ArchetypesMap& map, uint64_t startArchetypesIndex, const QueryFilterConfigType& filter)
		{
			auto& archetypes = map.m_Archetypes;
			uint64_t archetypesCount = map.m_Archetypes.Size();
			uint64_t minRequiredComponentsCount = filter.GetMinComponentsCount();

			for (uint64_t i = startArchetypesIndex; i < archetypesCount; i++)
			{
				Archetype& arch = archetypes[i];
				if (arch.GetComponentAndTagCount() >= minRequiredComponentsCount)
				{
					TryAddArchetypeFromGroup(arch, filter);
				}
			}
		}
	};
}