#pragma once
#include "decs/Core/Type.h"
#include "decs/Light/Archetypes/LArchetypesMap.h"
#include "decs/Light/LContainer.h"
#include "decs/Light/LEntity.h"

namespace decs::light
{

	template<typename T>
	struct query_data_container final
	{
	public:
		using data_type = pure_type_t<T>;
		using container_type = PackedLightComponentContainer<data_type>;
		inline static constexpr bool is_filter = false;
	};

	template<typename T>
	struct query_data_container<filter<T>> final
	{
	public:
		using container_type = FilterContainer<filter<T>>;
		inline static constexpr bool is_filter = true;
	};

	template<typename T>
	using query_data_container_t = query_data_container<T>::container_type;


	class Iteration
	{
	public:

		template<typename Desired, typename TupleType>
		inline static Desired* get_optional_data_from_tuple(
			uint64_t entityIndexInArchetype,
			const TupleType& containersTuple
		)
		{
			if constexpr (tuple_has_type_v<query_data_container_t<Desired>*, TupleType>)
			{
				return std::get<query_data_container_t<Desired>*>(containersTuple)->GetAsPtr(entityIndexInArchetype);
			}
			else
			{
				return nullptr;
			}
		}

		template<typename Callable, typename TupleType, typename... ComponentTypes>
		inline static void InvokeEntityIteration_WithOptional(
			Callable&& func,
			uint64_t entityIndexInArchetype,
			const TupleType& containersTuple
		)
		{
			if constexpr (sizeof...(ComponentTypes) != 0)
			{
				func(*get_optional_data_from_tuple<ComponentTypes, TupleType>(entityIndexInArchetype, containersTuple)...);
			}
			else
			{
				func();
			}
		}

		template<typename Callable, typename... ComponentTypes>
		inline static void InvokeEntityIteration(
			Callable&& func,
			uint64_t entityIndexInArchetype,
			const std::tuple<query_data_container_t<ComponentTypes>*...>& containersTuple
		)
		{
			func(std::get<query_data_container_t<ComponentTypes>*>(containersTuple)->GetAsRef(entityIndexInArchetype)...);
		}

		template<typename Callable, typename... ComponentTypes>
		inline static void InvokeEntityIteration(
			Callable&& func,
			Entity& entityBuffer,
			EntityData& entityData,
			uint64_t entityIndexInArchetype,
			const std::tuple<query_data_container_t<ComponentTypes>*...>& containersTuple
		)
		{
			entityBuffer.Set_Internal(entityData);
			func(
				entityBuffer,
				std::get<query_data_container_t<ComponentTypes>*>(containersTuple)->GetAsRef(entityIndexInArchetype)...
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

		const std::vector<TypeID>& GetWithoutTypes() const noexcept
		{
			return m_Without;
		}

		const std::vector<TypeID>& GetWithAnyTypes() const noexcept
		{
			return 	m_WithAnyOf;
		}

		const std::vector<TypeID>& GetWithAllTypes() const noexcept
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
		std::vector<TypeID> m_Without{};
		std::vector<TypeID> m_WithAnyOf{};
		std::vector<TypeID> m_WithAll{};
		TypeGroupType m_Includes = {};
		IFilterDataTupleHandle m_FilterDataTuple{};
	};

	template<light_component_or_filter_concept... ComponentsTypes>
	class IterationArchetypeContext
	{
	public:
		using ContainersTuple = std::tuple<query_data_container_t<ComponentsTypes>*...>;

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

			return ((std::get<query_data_container_t<ComponentsTypes>*>(m_ContainersTuple) != nullptr) && ...);
		}

		template<typename T>
		query_data_container_t<T>* GetArchetypeDataContainer()
		{
			if constexpr (query_data_container<T>::is_filter)
			{
				return m_Archetype->GetFilterData<filter_type_t<T>>().m_FilterContainer;
			}
			else
			{
				return m_Archetype->GetTypePackedContainer<typename query_data_container<T>::data_type>();
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
				Iteration::InvokeEntityIteration<Callable, ComponentsTypes...>(func, idx, containersTuple);
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
				Iteration::InvokeEntityIteration<Callable, ComponentsTypes...>(func, entityBuffer, *entities.Get(static_cast<size_t>(idx)), idx, containersTuple);
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
					Iteration::InvokeEntityIteration<Callable, ComponentsTypes...>(func, idx, containersTuple);
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
					Iteration::InvokeEntityIteration<Callable, ComponentsTypes...>(func, entityBuffer, *entityData, idx, containersTuple);
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
				Iteration::InvokeEntityIteration<Callable, ComponentsTypes...>(func, idx, containersTuple);
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
				Iteration::InvokeEntityIteration<Callable, ComponentsTypes...>(func, entityBuffer, *entityData, idx, containersTuple);
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
					Iteration::InvokeEntityIteration<Callable, ComponentsTypes...>(func, idx, containersTuple);
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
					Iteration::InvokeEntityIteration<Callable, ComponentsTypes...>(func, entityBuffer, *entityData, idx, containersTuple);
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
				Iteration::InvokeEntityIteration<Callable, ComponentsTypes...>(func, idx, containersTuple);
			}
		}

		template<typename Callable>
		void ForEachFromTo_WithEntity(Callable&& func, Entity& entityBuffer, uint64_t fromIdx, uint64_t toIdx) const
		{
			const auto& containersTuple = this->GetContainersTuple();
			auto& entities = this->GetArchetype()->GetEntities();

			for (uint64_t idx = fromIdx; idx < toIdx; idx++)
			{
				Iteration::InvokeEntityIteration<Callable, ComponentsTypes...>(func, entityBuffer, *entities.Get(idx), idx, containersTuple);
			}
		}

	#pragma endregion

	#pragma region FOREACH CONTAINER
	public:
		template<typename TCallable>
			requires light_query_iterate_container_callable<TCallable, ComponentsTypes...>
		void InvokeForEachComponentContainer(TCallable&& func) const
		{
			if (GetEntityCount() > 0)
			{
				func(std::get<query_data_container_t<ComponentsTypes>*>(m_ContainersTuple)->GetAsSpan()...);
			}
		}

	#pragma endregion

	private:
		const Archetype* m_Archetype = nullptr;
		ContainersTuple m_ContainersTuple{};
	};

	template<light_component_or_filter_concept... ComponentsTypes>
	class IterationContainerContext
	{
	public:
		using ArchetypeContextType = IterationArchetypeContext<ComponentsTypes...>;
		using QueryFilterConfigType = QueryFiltersConfig<drop_const_t<ComponentsTypes>...>;

	public:
		std::vector<ArchetypeContextType> m_ArchetypesContexts{};
		ecsMap<const Archetype*, size_t> m_ArchetypeIndices{};
		Container* m_Container = nullptr;
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

		inline bool IsValid() const noexcept
		{
			return m_Container != nullptr;
		}

		inline bool IsEnabled() const noexcept
		{
			return m_bIsEnabled;
		}

		inline bool IsValidAndEnabled() const noexcept
		{
			return IsValid() && IsEnabled();
		}

		inline Container* GetContainer() const
		{
			return m_Container;
		}

		const std::vector<ArchetypeContextType>& GetArchetypeContexts() const noexcept
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
		}

		void SetContainer(Container* container)
		{
			Clear();
			m_Container = container;
		}

		void Fetch(const QueryFilterConfigType& filter)
		{
			if (!IsValid())
			{
				return;
			}

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

			for (const auto& archetypeCtx : m_ArchetypesContexts)
			{
				entityCount += archetypeCtx.GetEntityCount();
			}

			return entityCount;
		}

		template<typename TCallable>
			requires light_query_iterate_container_callable<TCallable, ComponentsTypes...>
		void ForEachContainer(TCallable&& func) const
		{
			for (const auto& archetypeContext : m_ArchetypesContexts)
			{
				archetypeContext.InvokeForEachComponentContainer(func);
			}
		}

		inline bool ContainArchetype(const Archetype* arch) const
		{
			return m_ArchetypeIndices.contains(arch);
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
			if (!ContainArchetype(&archetype) && archetype.GetComponentTagFilterCount())
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
				std::span<Archetype*> archetypes = group->GetArchetypesWithComponentTagFilterCount(i);
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