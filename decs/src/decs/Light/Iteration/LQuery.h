#pragma once
#include "LIterationCore.h"

#include "LQueryManager.h"

namespace decs::light
{
	template<light_component_or_filter_concept... ComponentsTypes>
	class Query : public IQuery
	{
	private:
		using ArchetypeContextType = IterationArchetypeContext<drop_const_t<ComponentsTypes>...>;
		using ContainersTupleType = ArchetypeContextType::ContainersTuple;
		using QueryFilterConfigType = QueryFiltersConfig<drop_const_t<ComponentsTypes>...>;
		using ContainerContextType = IterationContainerContext<drop_const_t<ComponentsTypes>...>;

	public:
		Query() = default;

		Query(Container* container):
			m_ContainerContext(container)
		{
			AddToContainer();
		}

		~Query()
		{
			RemoveFromContainer();
		}

		template<light_component_or_tag_or_filter_concept... WithoutTypes>
		Query& Without()
		{
			MakeDirty();
			m_FilterConfig.Without<WithoutTypes...>();
			return *this;
		}

		template<light_component_or_tag_or_filter_concept... WithAnyTypes>
		Query& WithAny()
		{
			MakeDirty();
			m_FilterConfig.WithAny<WithAnyTypes...>();
			return *this;
		}

		template<light_component_or_tag_or_filter_concept... WithTypes>
		Query& With()
		{
			MakeDirty();
			m_FilterConfig.With<WithTypes...>();
			return *this;
		}

		template<filter_concept... FilterTypes>
		void WithFilterData(FilterTypes&&... filterData)
		{
			MakeDirty();
			m_FilterConfig.WithFilterData(std::forward<FilterTypes>(filterData)...);
		}

		void ClearFilters()
		{
			if (m_FilterConfig.Clear())
			{
				MakeDirty();
			}
		}

		inline void SetContainer(Container* container)
		{
			if (m_ContainerContext.GetContainer() != container)
			{
				RemoveFromContainer();
				m_ContainerContext.SetContainer(container);
				AddToContainer();

				m_IsDirty = true;
			}
		}

		[[nodiscard]] inline Container* GetContainer() const
		{
			return m_ContainerContext.GetContainer();
		}

		[[nodiscard]] inline bool IsValid()const
		{
			return m_ContainerContext.IsValid();
		}

		[[nodiscard]] inline uint64_t GetEntityCount()
		{
			if (IsValid())
			{
				Fetch();

				const auto& archetypeContexts = m_ContainerContext.GetArchetypeContexts();
				uint64_t entityCount = 0;
				for (auto& archetypeCtx : archetypeContexts)
				{
					entityCount += archetypeCtx.GetEntityCount();
				}
			}

			return 0;
		}

		/// <summary>
		/// Iterates over entities in archetypes from first to last. During iteration with this method creating, destroying and adding or removing component is forbidden on all entities, because it can cause undefined behavior. 
		/// Destroying entites and adding or removing component to any entity, can cause that iteration index will go out of bound. 
		/// Creating new entities will not cause index out of bound but if created entity has components which satisfys this query, it is undefined if that entity will be iterated or not in this function. If created entity will be placed in archetype that is not valid for this query it is safe to create it.
		/// </summary>
		/// <typeparam name="Callable"></typeparam>
		/// <param name="func"></param>
		template<typename Callable>
			requires light_query_callable<Callable, ComponentsTypes...>
		inline void ForEach(Callable&& func) noexcept
		{
			if (!IsValid()) return;
			Fetch();

			Container* container = m_ContainerContext.GetContainer();
			auto& archetypeContexts = m_ContainerContext.GetArchetypeContexts();
			const uint64_t contextCount = archetypeContexts.size();

			if constexpr (is_invocable_with_light_entity_v<Callable, ComponentsTypes...>)
			{
				Entity entityBuffer = {};
				for (const auto& ctx : archetypeContexts)
				{
					ctx.ForEach_WithEntity(func, entityBuffer);
				}
			}
			else
			{
				for (const auto& ctx : archetypeContexts)
				{
					ctx.ForEach(func);
				}
			}
		}

		/// <summary>
		/// Iterates over entities in archetypes from last to first. During iteration with this method only destroying current entity is not forbidden. Any other operation on all entities are undefined behaviors. 
		/// Creating new entities will not cause index out of bound but if created entity has components which satisfys this query, it is undefined if that entity will be iterated or not in this function. 
		/// Destroying entities other than currently iterated and removing or adding component from them can cause index out of bound.
		/// Creating new entities will not cause index out of bound, but if created entity has components which satisfys this query, it is undefined if that entity will be iterated or not in this function. If created entity will be placed in archetype that is not valid for this query it is safe to create it.
		/// Adding or removing components from currnet iterated entity will not cause index out of bound, but it can cause that this entity will be iterated again. If after add or remove component, entity will be moved to archetype which is not valid for this query it is known that entity will not be iterated again.
		/// </summary>
		/// <typeparam name="Callable"></typeparam>
		/// <param name="func"></param>
		template<typename Callable>
			requires light_query_callable<Callable, ComponentsTypes...>
		void ForEachBackward(Callable&& func) noexcept
		{
			if (!IsValid()) return;
			Fetch();

			Container* container = m_ContainerContext.GetContainer();
			auto& archetypeContexts = m_ContainerContext.GetArchetypeContexts();
			const uint64_t contextCount = archetypeContexts.size();

			if constexpr (is_invocable_with_light_entity_v<Callable, ComponentsTypes...>)
			{
				Entity entityBuffer = {};

				for (const auto& ctx : archetypeContexts)
				{
					ctx.ForEachBackward_WithEntity(func, entityBuffer);
				}
			}
			else
			{
				for (const auto& ctx : archetypeContexts)
				{
					ctx.ForEachBackward(func);
				}
			}
		}

		template<typename TCallable>
			requires light_query_iterate_container_callable<TCallable, ComponentsTypes...>
		void ForEachArchetype(TCallable&& func)
		{
			if (!IsValid()) return;
			Fetch();

			m_ContainerContext.ForEachContainer(func);
		}

		/// <summary>
		/// Checks if entity belong to this query.
		/// </summary>
		/// <param name="entity"></param>
		/// <returns></returns>
		[[nodiscard]] bool Contain(const Entity& entity)
		{
			if (entity.IsValid())
			{
				Fetch();
				return m_ContainerContext.ContainsArchetype(entity.GetArchetype());
			}
			return false;
		}

		void Fetch()
		{
			if (m_IsDirty)
			{
				m_IsDirty = false;
				m_ContainerContext.Clear();
				m_ContainerContext.Fetch(m_FilterConfig);
			}
		}

	private:
		QueryFilterConfigType m_FilterConfig{};
		ContainerContextType m_ContainerContext{};
		bool m_IsDirty = true;

	private:
		void MakeDirty()
		{
			m_IsDirty = true;
		}

		template<typename Callable>
		inline static void InvokeEntityIteration(
			Callable&& func,
			Entity& entityBuffer,
			EntityData& entityData,
			uint64_t entityIndexInArchetype,
			const ContainersTupleType& containersTuple
		)
		{
			if constexpr (is_invocable_with_light_entity_v<Callable, ComponentsTypes...>)
			{
				Iteration::InvokeEntityIteration<Callable, ComponentsTypes...>(
					func,
					entityBuffer,
					entityData,
					entityIndexInArchetype,
					containersTuple
				);
			}
			else
			{
				Iteration::InvokeEntityIteration<Callable, ComponentsTypes...>(
					func,
					entityIndexInArchetype,
					containersTuple
				);
			}
		}

	#pragma region ILightQuery implementation 
	private:
		void AddToContainer()
		{
			Container* container = m_ContainerContext.GetContainer();
			if (container != nullptr)
			{
				container->AddQuery(this);
			}
		}

		void RemoveFromContainer()
		{
			Container* container = m_ContainerContext.GetContainer();
			if (container != nullptr)
			{
				container->RemoveQuery(this);
			}
		}

		void TryAddArchetype(const Archetype& archetype) override
		{
			m_ContainerContext.TryAddArchetype(archetype, m_FilterConfig);
		}

		void TryRemoveArchetpye(const Archetype& archetype) override
		{
			m_ContainerContext.TryRemoveArchetype(archetype);
		}

		void OnQueryManagerDestroy() override
		{
			m_ContainerContext.SetContainer(nullptr);
		}
	#pragma endregion

	#pragma region BATCH ITERATOR
	public:
		class BatchIterator
		{
			using QueryType = Query<ComponentsTypes...>;

			template<light_component_or_filter_concept... Types>
			friend class Query;
		public:
			BatchIterator() {}

			BatchIterator(
				QueryType* query,
				uint64_t firstArchetypeIndex,
				uint64_t firstIterationIndex,
				uint64_t entitiesCount
			):
				m_Query(query),
				m_FirstArchetypeIndex(firstArchetypeIndex),
				m_FirstIterationIndex(firstIterationIndex),
				m_EntitiesCount(entitiesCount)
			{
			}

			~BatchIterator() {}

			inline bool IsValid()const noexcept
			{
				return m_Query != nullptr && m_Query->IsValid();
			}

			template<typename Callable>
				requires light_query_callable<Callable, ComponentsTypes...>
			inline void ForEach(Callable&& func) const
			{
				if (!IsValid())
				{
					return;
				}

				Container* container = m_Query->GetContainer();
				auto& archetypeContexts = m_Query->m_ContainerContext.GetArchetypeContexts();

				Entity entityBuffer = {};

				uint64_t contextIndex = m_FirstArchetypeIndex;
				uint64_t contextCount = archetypeContexts.size();

				uint64_t leftEntitiesToIterate = m_EntitiesCount;

				for (; contextIndex < contextCount; contextIndex++)
				{
					const ArchetypeContextType& ctx = archetypeContexts[contextIndex];
					uint64_t ctxEntityCount = ctx.GetEntityCount();
					if (ctxEntityCount == 0) continue;

					const uint64_t startEntityIdx = contextIndex == m_FirstArchetypeIndex ? m_FirstIterationIndex : 0;
					uint64_t iterationsCount;

					uint64_t leftEntitiesInContext = ctxEntityCount - startEntityIdx;
					if (leftEntitiesToIterate <= leftEntitiesInContext)
					{
						iterationsCount = startEntityIdx + leftEntitiesToIterate;
						leftEntitiesToIterate = 0;
					}
					else
					{
						iterationsCount = startEntityIdx + leftEntitiesInContext;
						leftEntitiesToIterate -= leftEntitiesInContext;
					}

					if constexpr (is_invocable_with_light_entity_v<Callable, ComponentsTypes...>)
					{
						ctx.ForEachFromTo_WithEntity(func, entityBuffer, startEntityIdx, iterationsCount);
					}
					else
					{
						ctx.ForEachFromTo(func, startEntityIdx, iterationsCount);
					}

					if (leftEntitiesToIterate == 0)
					{
						return;
					}
				}
			}

		private:
			QueryType* m_Query = nullptr;
			uint64_t m_FirstArchetypeIndex = 0;
			uint64_t m_FirstIterationIndex = 0;
			uint64_t m_EntitiesCount = 0;
		};

	#pragma endregion

	public:
		void CreateBatchIterators(
			std::vector<BatchIterator>& iterators,
			uint64_t desiredBatchesCount,
			uint64_t minBatchSize
		)
		{
			if (!IsValid())
			{
				return;
			}

			Fetch();

			const auto& archetypeContexts = m_ContainerContext.GetArchetypeContexts();

			uint64_t entitiesCount = 0;
			for (const ArchetypeContextType& archContext : archetypeContexts)
			{
				entitiesCount += archContext.GetEntityCount();
			}

			uint64_t realDesiredBatchSize = std::llround(std::ceil((float)entitiesCount / (float)desiredBatchesCount));
			uint64_t finalBatchSize;

			if (realDesiredBatchSize < minBatchSize)
			{
				finalBatchSize = minBatchSize;
			}
			else
			{
				finalBatchSize = realDesiredBatchSize;
			}

			BatchIterator* currentIterator = nullptr;

			uint64_t contextsCount = archetypeContexts.size();
			for (uint64_t contextIndex = 0; contextIndex < contextsCount; contextIndex++)
			{
				const ArchetypeContextType& ctx = archetypeContexts[contextIndex];
				uint64_t ctxEntitiesCount = ctx.GetEntityCount();
				if (ctxEntitiesCount == 0) continue;

				uint64_t currentEntityIndex = 0;

				while (ctxEntitiesCount > 0)
				{
					if (currentIterator == nullptr)
					{
						currentIterator = &iterators.emplace_back(this, contextIndex, currentEntityIndex, 0);
					}

					uint64_t neededEntitiesCount = finalBatchSize - currentIterator->m_EntitiesCount;

					if (neededEntitiesCount > 0)
					{
						uint64_t entitiesCountToAddToIterator;
						if (neededEntitiesCount <= ctxEntitiesCount)
						{
							entitiesCountToAddToIterator = neededEntitiesCount;
							ctxEntitiesCount -= entitiesCountToAddToIterator;
						}
						else
						{
							entitiesCountToAddToIterator = ctxEntitiesCount;
							ctxEntitiesCount = 0;
						}
						currentIterator->m_EntitiesCount += entitiesCountToAddToIterator;
						currentEntityIndex += entitiesCountToAddToIterator;

						if (currentIterator->m_EntitiesCount == finalBatchSize)
						{
							currentIterator = nullptr;
						}
					}
					else
					{
						currentIterator = nullptr;
					}
				}
			}

		}
	};

}