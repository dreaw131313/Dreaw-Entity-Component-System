#pragma once
#include "decs/Core/Type.h"
#include "decs/Light/Archetypes/LArchetypesMap.h"
#include "decs/Light/LContainer.h"
#include "decs/Light/LEntity.h"
#include "LIterationTratis.h"

namespace decs::light
{

	namespace iteration::trait
	{
	}

	class Iteration
	{
	public:
		template<typename Callable, typename... QueryDataContainerType>
		inline static void InvokeEntityIteration(
			Callable&& func,
			uint64_t entityIndexInArchetype,
			const std::tuple<QueryDataContainerType*...>& containersTuple
		)
		{
			func(std::get<QueryDataContainerType*>(containersTuple)->GetAsRef(entityIndexInArchetype)...);
		}


		template<typename Callable, typename... QueryDataContainerType>
		inline static void InvokeEntityIteration(
			Callable&& func,
			Entity& entityBuffer,
			EntityData& entityData,
			uint64_t entityIndexInArchetype,
			const std::tuple<QueryDataContainerType*...>& containersTuple
		)
		{
			entityBuffer.Set_Internal(entityData);
			func(
				entityBuffer,
				std::get<QueryDataContainerType*>(containersTuple)->GetAsRef(entityIndexInArchetype)...
			);
		}
	};

	template<light_component_or_filter_concept... ComponentsTypes>
	struct QueryFiltersConfig
	{
	public:
		using TypeGroupType = TypeGroup<pure_type_t<ComponentsTypes>...>;

	public:
		const TypeGroupType& GetIncludes() const
		{
			return m_Includes;
		}

		const ecsVector<TypeID>& GetWithoutTypes() const noexcept
		{
			return m_Without;
		}

		const ecsVector<TypeID>& GetWithAnyTypes() const noexcept
		{
			return 	m_WithAnyOf;
		}

		const ecsVector<TypeID>& GetWithAllTypes() const noexcept
		{
			return m_WithAll;
		}

		inline const IFilterDataTupleHandle& GetFilterDataTuple() const noexcept
		{
			return m_FilterDataTuple;
		}

		inline uint64_t GetMinComponentFilterCount() const
		{
			uint64_t includesCount = sizeof...(ComponentsTypes);
			if (m_WithAnyOf.size() > 0) includesCount += 1;
			return sizeof...(ComponentsTypes) + m_WithAll.size();
		}

		template<light_component_or_tag_or_filter_concept... WithoutTypes>
		void Without()
		{
			if constexpr (sizeof...(WithoutTypes) == 0)
			{
				m_Without.clear();
			}
			else
			{
				m_Without.reserve(sizeof...(WithoutTypes));
				(m_Without.push_back(Type<drop_const_t<ligth_component_or_tag_or_filter_t<WithoutTypes>>>::ID()), ...);
			}
		}

		template<light_component_or_tag_or_filter_concept... WithAnyTypes>
		void WithAny()
		{
			if constexpr (sizeof...(WithAnyTypes) == 0)
			{
				m_WithAnyOf.clear();
			}
			else
			{
				m_WithAnyOf.reserve(sizeof...(WithAnyTypes));
				(m_WithAnyOf.push_back(Type<drop_const_t<ligth_component_or_tag_or_filter_t<WithAnyTypes>>>::ID()), ...);
			}
		}

		template<light_component_or_tag_or_filter_concept... WithTypes>
		void With()
		{
			if constexpr (sizeof...(WithTypes) == 0)
			{
				m_WithAll.clear();
			}
			else
			{
				m_WithAll.reserve(sizeof...(WithTypes));
				(m_WithAll.push_back(Type<drop_const_t<ligth_component_or_tag_or_filter_t<WithTypes>>>::ID()), ...);
			}
		}

		template<filter_concept... FilterTypes>
		void WithFilterData(FilterTypes&&... filterData)
		{
			m_FilterDataTuple = TFilterDataTupleHandle<FilterTypes...>::Create(std::forward<FilterTypes>(filterData)...).Cast<IFilterDataTuple>();
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
		ecsVector<TypeID> m_Without{};
		ecsVector<TypeID> m_WithAnyOf{};
		ecsVector<TypeID> m_WithAll{};
		TypeGroupType m_Includes = {};
		IFilterDataTupleHandle m_FilterDataTuple{};
	};

	template<typename... ComponentContainerType>
	struct SimpleArchetypeIterator;

	template<typename... ComponentContainerType>
	struct SimpleArchetypeIterator<std::tuple<ComponentContainerType*...>>
	{
		using ComponentContainersTuple = std::tuple<ComponentContainerType*... >;

	public:
		SimpleArchetypeIterator(const Archetype& archetype, const ComponentContainersTuple& components) :
			m_Archetype(archetype),
			m_ComponentsTuple(components)
		{

		}

		template<typename Func>
			requires std::is_invocable_v<Func, typename ComponentContainerType::component_type&...>
		|| std::is_invocable_v<Func, const Entity&, typename ComponentContainerType::component_type&...>
			void ForEach(Func&& func) const
		{
			uint64_t entityCount = m_Archetype.EntityCount();
			if (entityCount == 0)
			{
				return;
			}

			Entity entityBuffer{};

			auto entityDataList = m_Archetype.GetEntities().Data();
			for (uint64_t idx = 0; idx < entityCount; idx++)
			{
				auto& archetypeEntityData = entityDataList[idx];

				if constexpr (std::is_invocable_v<Func, const Entity&, typename ComponentContainerType::component_type&...>)
				{
					Iteration::InvokeEntityIteration(func, entityBuffer, *archetypeEntityData, idx, m_ComponentsTuple);
				}
				else
				{
					Iteration::InvokeEntityIteration(func, idx, m_ComponentsTuple);
				}
			}
		}

	private:
		const Archetype& m_Archetype;
		const ComponentContainersTuple& m_ComponentsTuple;
	};


	template<light_component_or_filter_concept... ComponentsTypes>
	class IterationArchetypeContext
	{
	public:
		using ContainersTuple = std::tuple<iteration::trait::query_data_container_t<ComponentsTypes>*...>;
		using FiltersOnlyTuple = iteration::trait::create_filter_container_only_tuple<iteration::trait::query_data_container_t<ComponentsTypes>...>;
		using ComponentsOnlyTuple = iteration::trait::create_packed_container_only_tuple<iteration::trait::query_data_container_t<ComponentsTypes>...>;
		using ComponentOnlyIterator = SimpleArchetypeIterator<ComponentsOnlyTuple>;

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

			//m_ContainersTuple = { m_Archetype->GetTypePackedContainer<pure_type_t<ComponentsTypes>>()... };
			m_ContainersTuple = { GetArchetypeDataContainer<ComponentsTypes>()... };

			return ((std::get<iteration::trait::query_data_container_t<ComponentsTypes>*>(m_ContainersTuple) != nullptr) && ...);
		}

		template<typename T>
		iteration::trait::query_data_container_t<T>* GetArchetypeDataContainer()
		{
			if constexpr (iteration::trait::query_data_container<T>::is_filter)
			{
				return m_Archetype->GetFilterData<filter_type_t<T>>().m_FilterContainer;
			}
			else
			{
				return m_Archetype->GetTypePackedContainer<typename iteration::trait::query_data_container<T>::data_type>();
			}
		}

	#pragma region FOREACH
	public:
		template<typename Callable>
		void ForEach(Callable&& func) const
		{
			uint64_t ctxEntityCount = this->GetEntityCount();
			if (ctxEntityCount == 0)
			{
				return;
			}

			const auto& containersTuple = this->GetContainersTuple();

			for (uint64_t idx = 0; idx < ctxEntityCount; idx++)
			{
				Iteration::InvokeEntityIteration(func, idx, containersTuple);
			}
		}

		template<typename Callable>
		void ForEach_WithEntity(Callable&& func, Entity& entityBuffer) const
		{
			uint64_t ctxEntityCount = this->GetEntityCount();
			if (ctxEntityCount == 0)
			{
				return;
			}

			const auto& containersTuple = this->GetContainersTuple();
			auto& entities = this->GetArchetype()->GetEntities();

			for (uint64_t idx = 0; idx < ctxEntityCount; idx++)
			{
				Iteration::InvokeEntityIteration(func, entityBuffer, *entities.Get(static_cast<size_t>(idx)), idx, containersTuple);
			}
		}

	#pragma endregion

	#pragma region FOREACH SAFE
	public:
		template<typename Callable>
		void ForEach_Safe(Callable&& func) const
		{
			uint64_t ctxEntityCount = this->GetEntityCount();
			if (ctxEntityCount == 0)
			{
				return;
			}

			const auto& containersTuple = this->GetContainersTuple();
			auto& entities = this->GetArchetype()->GetEntities();

			for (uint64_t idx = 0; idx < ctxEntityCount; idx++)
			{
				auto entityData = entities.Get(idx);
				if (entityData != nullptr)
				{
					Iteration::InvokeEntityIteration(func, idx, containersTuple);
				}
			}
		}

		template<typename Callable>
		void ForEach_WithEntity_Safe(Callable&& func, Entity& entityBuffer) const
		{
			uint64_t ctxEntityCount = this->GetEntityCount();
			if (ctxEntityCount == 0)
			{
				return;
			}

			const auto& containersTuple = this->GetContainersTuple();
			auto& entities = this->GetArchetype()->GetEntities();

			for (uint64_t idx = 0; idx < ctxEntityCount; idx++)
			{
				auto entityData = entities.Get(idx);
				if (entityData != nullptr)
				{
					Iteration::InvokeEntityIteration(func, entityBuffer, *entityData, idx, containersTuple);
				}
			}
		}

	#pragma endregion

	#pragma region FOREACH BACKWARD
	public:
		template<typename Callable>
		void ForEachBackward(Callable&& func) const
		{
			uint64_t ctxEntityCount = this->GetEntityCount();
			if (ctxEntityCount == 0)
			{
				return;
			}

			const auto& containersTuple = this->GetContainersTuple();

			int64_t idx = ctxEntityCount - 1;
			for (; idx > -1; idx--)
			{
				Iteration::InvokeEntityIteration(func, idx, containersTuple);
			}
		}

		template<typename Callable>
		void ForEachBackward_WithEntity(Callable&& func, Entity& entityBuffer) const
		{
			uint64_t ctxEntityCount = this->GetEntityCount();
			if (ctxEntityCount == 0)
			{
				return;
			}

			const auto& containersTuple = this->GetContainersTuple();
			auto& entities = this->GetArchetype()->GetEntities();

			int64_t idx = ctxEntityCount - 1;
			for (; idx > -1; idx--)
			{
				auto entityData = entities.Get(idx);
				Iteration::InvokeEntityIteration(func, entityBuffer, *entityData, idx, containersTuple);
			}
		}

	#pragma endregion

	#pragma region FOREACH BACKWARD SAFE
	public:
		template<typename Callable>
		void ForEachBackward_Safe(Callable&& func) const
		{
			uint64_t ctxEntityCount = this->GetEntityCount();
			if (ctxEntityCount == 0)
			{
				return;
			}

			const auto& containersTuple = this->GetContainersTuple();
			auto& entities = this->GetArchetype()->GetEntities();

			int64_t idx = ctxEntityCount - 1;
			for (; idx > -1; idx--)
			{
				auto entityData = entities.Get(idx);
				if (entityData != nullptr)
				{
					Iteration::InvokeEntityIteration(func, idx, containersTuple);
				}
			}
		}

		template<typename Callable>
		void ForEachBackward_WithEntity_Safe(Callable&& func, Entity& entityBuffer) const
		{
			uint64_t ctxEntityCount = this->GetEntityCount();
			if (ctxEntityCount == 0)
			{
				return;
			}

			const auto& containersTuple = this->GetContainersTuple();
			auto& entities = this->GetArchetype()->GetEntities();

			int64_t idx = ctxEntityCount - 1;
			for (; idx > -1; idx--)
			{
				auto entityData = entities.Get(idx);
				if (entityData != nullptr)
				{
					Iteration::InvokeEntityIteration(func, entityBuffer, *entityData, idx, containersTuple);
				}
			}
		}

	#pragma endregion

	#pragma region FOREACH FROM TO

		template<typename Callable>
		void ForEachFromTo(Callable&& func, uint64_t fromIdx, uint64_t toIdx) const
		{
			const auto& containersTuple = this->GetContainersTuple();

			for (uint64_t idx = fromIdx; idx < toIdx; idx++)
			{
				Iteration::InvokeEntityIteration(func, idx, containersTuple);
			}
		}

		template<typename Callable>
		void ForEachFromTo_WithEntity(Callable&& func, Entity& entityBuffer, uint64_t fromIdx, uint64_t toIdx) const
		{
			const auto& containersTuple = this->GetContainersTuple();
			auto& entities = this->GetArchetype()->GetEntities();

			for (uint64_t idx = fromIdx; idx < toIdx; idx++)
			{
				Iteration::InvokeEntityIteration(func, entityBuffer, *entities.Get(idx), idx, containersTuple);
			}
		}

	#pragma endregion

	#pragma region FOREACH CONTAINER
	public:
		template<typename Callable>
			requires iteration::trait::light_query_iterate_container_callable<Callable, ComponentsTypes...>
		void  InvokeForEachComponentContainer(Callable&& func) const
		{
			if (GetEntityCount() > 0)
			{
				func(std::get<iteration::trait::query_data_container_t<ComponentsTypes>*>(m_ContainersTuple)->GetAsSpan()...);
			}
		}

	#pragma endregion

	#pragma region FOREACH FILTER -> FOREACH ENTITY
	public:
		template<typename Func>
			requires iteration::trait::is_invocable_with_only_filters_v<Func, const ComponentOnlyIterator&, FiltersOnlyTuple>
		void ForEachFilter(Func&& func) const
		{
			if (m_Archetype->IsEmpty())
			{
				return;
			}

			FiltersOnlyTuple filterContainers{};
			ComponentsOnlyTuple componentContainers{};

			FillTuple(m_ContainersTuple, filterContainers);
			FillTuple(m_ContainersTuple, componentContainers);

			ComponentOnlyIterator thisIterator(*m_Archetype, componentContainers);

			InvokeFiltersOnlyCallable(func, filterContainers, thisIterator);
		}

	private:
		template<typename Func, typename... Filters>
		inline static void InvokeFiltersOnlyCallable(Func&& func, const std::tuple<Filters...>& filters, const ComponentOnlyIterator& thisIterator)
		{
			func(std::get<Filters>(filters)->GetAsRef(0)..., thisIterator);
		}

		template<typename... From, typename... To>
		inline static void FillTuple(const std::tuple<From...>& from, std::tuple<To...>& to)
		{
			((std::get<To>(to) = std::get<To>(from)), ...);
		}
	#pragma endregion

	private:
		const Archetype* m_Archetype = nullptr;
		ContainersTuple m_ContainersTuple{};

	public:
		struct Iterator
		{
		public:
			Iterator(IterationArchetypeContext& ctx) :
				m_Ctx(ctx)
			{

			}
			template<typename Func>
			void ForEach(Func&& func) const
			{
				if constexpr (iteration::trait::is_invocable_with_light_entity_v<Func, ComponentsTypes...>)
				{
					Entity e{};
					m_Ctx.ForEachBackward_WithEntity(func);
				}
				else
				{
					m_Ctx.ForEach(func);
				}
			}

		private:
			const IterationArchetypeContext& m_Ctx;
		};


	};

	template<light_component_or_filter_concept... ComponentsTypes>
	class IterationContainerContext
	{
	public:
		using ArchetypeContextType = IterationArchetypeContext<ComponentsTypes...>;
		using QueryFilterConfigType = QueryFiltersConfig<drop_const_t<ComponentsTypes>...>;

	public:
		ecsVector<ArchetypeContextType> m_ArchetypesContexts{};
		ecsHashMap<const Archetype*, size_t> m_ArchetypeIndices{};
		Container* m_Container = nullptr;
		bool m_bIsEnabled = true;
		bool m_bIsDirty = true;

	public:
		IterationContainerContext()
		{

		}

		IterationContainerContext(Container* container, bool bIsEnabled = true) :
			m_Container(container),
			m_bIsEnabled(bIsEnabled)
		{

		}

		inline bool IsValid() const noexcept
		{
			return m_Container != nullptr;
		}

		inline bool IsEnabled() const noexcept
		{
			return m_bIsEnabled;
		}

		inline void SetDirty()
		{
			m_bIsDirty = true;
		}

		inline bool IsDirty() const noexcept
		{
			return m_bIsDirty;
		}

		inline bool IsValidAndEnabled() const noexcept
		{
			return IsValid() && IsEnabled();
		}

		inline Container* GetContainer() const
		{
			return m_Container;
		}

		const ecsVector<ArchetypeContextType>& GetArchetypeContexts() const noexcept
		{
			return m_ArchetypesContexts;
		}

		inline bool ContainsArchetype(const Archetype* archetype) const
		{
			return m_ArchetypeIndices.contains(archetype);
		}

		void Clear()
		{
			m_ArchetypesContexts.clear();
			m_ArchetypeIndices.clear();
			SetDirty();
		}

		void SetContainer(Container* container)
		{
			Clear();
			m_Container = container;
		}

		void Fetch(const QueryFilterConfigType& filter)
		{
			if (!IsValid() || !IsDirty())
			{
				return;
			}

			m_bIsDirty = false;

			uint64_t minComponentFilterCount = filter.GetMinComponentFilterCount();

			ArchetypesMap& map = m_Container->m_ArchetypesMap;
			uint64_t maxComponentsInArchetype = map.GetMaxComponentTagFilterCount();
			if (maxComponentsInArchetype >= minComponentFilterCount)
			{
				if (filter.GetIncludes().Size() > 0)
				{
					auto group = GetBestArchetypesGroup(filter);
					FetchArchetypesFromArchetypesGroup(group, filter);
				}
				else
				{
					AddAllArchetypesToQuery(map, filter);
				}
			}
		}

		bool Contain(const Entity& entity)
		{
			return m_ArchetypeIndices.find(entity.GetArchetype()) != m_ArchetypeIndices.end();
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

			for (const ArchetypeContextType& archetypeCtx : m_ArchetypesContexts)
			{
				entityCount += archetypeCtx.GetEntityCount();
			}

			return entityCount;
		}

		template<typename Callable>
			requires iteration::trait::light_query_iterate_container_callable<Callable, ComponentsTypes...>
		void ForEachContainer(Callable&& func) const
		{
			for (const ArchetypeContextType& archetypeContext : m_ArchetypesContexts)
			{
				archetypeContext.InvokeForEachComponentContainer(func);
			}
		}

		const ArchetypesGroupByOneType* GetBestArchetypesGroup(const QueryFilterConfigType& filter)
		{
			auto& groupsMap = m_Container->m_ArchetypesMap.m_ArchetypesGroupedByOneType;

			uint64_t bestArchetypesCount = std::numeric_limits<uint64_t>::max();
			const ArchetypesGroupByOneType* bestGroup = nullptr;

			if (const IFilterDataTupleHandle& filterDataTuple = filter.GetFilterDataTuple())
			{
				bestGroup = filterDataTuple->GetBestArchetypeGroup(m_Container->m_ArchetypesMap, bestArchetypesCount);
				if (bestGroup != nullptr)
				{
					bestArchetypesCount = bestGroup->GetArchetypesCount();
				}
			}

			const auto& includes = filter.GetIncludes();
			for (uint64_t i = 0; i < includes.Size(); i++)
			{
				auto it = groupsMap.find(includes[i]);
				if (it != groupsMap.end())
				{
					uint64_t bufforGroupArchetypesCount = it->second->GetArchetypesCount();
					if (bufforGroupArchetypesCount < bestArchetypesCount)
					{
						bestArchetypesCount = bufforGroupArchetypesCount;
						bestGroup = it->second;
					}
				}
			}

			return bestGroup;
		}

		void TryAddArchetype(const Archetype& archetype, const QueryFilterConfigType& filter)
		{
			if (!ContainsArchetype(&archetype) && archetype.GetComponentTagFilterCount())
			{
				// filter data tuple
				if (auto& filterDataTuple = filter.GetFilterDataTuple())
				{
					if (!filterDataTuple->TestArchetype(archetype))
					{
						return;
					}
				}

				// without test
				{
					auto& without = filter.GetWithoutTypes();

					uint64_t excludeCount = without.size();
					for (int i = 0; i < excludeCount; i++)
					{
						if (archetype.ContainComponentOrTagOrFilterType(without[i]))
						{
							return;
						}
					}
				}

				// with any test
				{
					auto& withAnyOf = filter.GetWithAnyTypes();

					uint64_t requiredAnyCount = withAnyOf.size();
					bool containRequiredAny = requiredAnyCount == 0;

					for (int i = 0; i < requiredAnyCount; i++)
					{
						if (archetype.ContainComponentOrTagOrFilterType(withAnyOf[i]))
						{
							containRequiredAny = true;
							break;
						}
					}
					if (!containRequiredAny) return;
				}

				// required all test
				{
					auto& withAll = filter.GetWithAllTypes();
					uint64_t requiredAllCount = withAll.size();

					for (int i = 0; i < requiredAllCount; i++)
					{
						if (!archetype.ContainComponentOrTagOrFilterType(withAll[i]))
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
						m_ArchetypeIndices[&archetype] = m_ArchetypesContexts.size();
						m_ArchetypesContexts.push_back(context);
					}
				}
			}
		}

		void TryRemoveArchetype(const Archetype& archetype)
		{
			auto it = m_ArchetypeIndices.find(&archetype);
			if (it == m_ArchetypeIndices.end())
			{
				return;
			}

			size_t index = it->second;
			if (index < (m_ArchetypesContexts.size() - 1))
			{
				ArchetypeContextType& lastArchetypeContext = m_ArchetypesContexts.back();
				m_ArchetypesContexts[index] = lastArchetypeContext;
				m_ArchetypeIndices[lastArchetypeContext.GetArchetype()] = index;
			}
			m_ArchetypesContexts.pop_back();
			m_ArchetypeIndices.erase(&archetype);
		}

		void FetchArchetypesFromArchetypesGroup(const ArchetypesGroupByOneType* group, const QueryFilterConfigType& filter)
		{
			if (group == nullptr) return;
			uint64_t maxComponentCountsInGroup = group->GetMaxComponentTagFilterCount();

			for (uint64_t i = filter.GetMinComponentFilterCount(); i <= maxComponentCountsInGroup; i++)
			{
				std::span<const Archetype* const> archetypes = group->GetArchetypesWithComponentTagFilterCount(i);
				for (auto archetype : archetypes)
				{
					TryAddArchetype(*archetype, filter);
				}
			}
		}

		void AddingArchetypesWithCheckingOnlyNewArchetypes(ArchetypesMap& map, uint64_t startArchetypesIndex, const QueryFilterConfigType& filter)
		{
			auto archetypes = map.m_ArchetypeAllocator.GetCreatedArchetypes();
			size_t archetypesCount = archetypes.size();
			size_t minRequiredComponentTagFilterCount = filter.GetMinComponentFilterCount();

			for (size_t i = startArchetypesIndex; i < archetypesCount; i++)
			{
				const Archetype* arch = archetypes[i];
				if (arch->GetComponentTagFilterCount() >= minRequiredComponentTagFilterCount)
				{
					TryAddArchetype(*arch, filter);
				}
			}
		}

		void AddAllArchetypesToQuery(ArchetypesMap& map, const QueryFilterConfigType& filter)
		{
			size_t minRequiredComponentTagFilterCount = filter.GetMinComponentFilterCount();
			for (auto archetype : map.m_ArchetypeAllocator.GetCreatedArchetypes())
			{
				if (archetype->GetComponentTagFilterCount() >= minRequiredComponentTagFilterCount)
				{
					TryAddArchetype(*archetype, filter);
				}
			}
		}
	};
}