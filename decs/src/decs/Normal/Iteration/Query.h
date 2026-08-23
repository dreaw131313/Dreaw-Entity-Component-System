#pragma once

#include "IterationCore.h"
#include "QueryManager.h"

namespace decs
{
	template<component_concept... ComponentsTypes>
	class Query : public IQuery
	{
		static_assert(!decs::contain_tags_v<ComponentsTypes...>, "Query must not use tags in as ComponentTypes!");

	private:
		using ArchetypeContextType = IterationArchetypeContext<drop_const_t<ComponentsTypes>...>;
		using ContainersTupleType = ArchetypeContextType::ContainersTuple;
		using QueryFilterConfigType = QueryFiltersConfig<drop_const_t<ComponentsTypes>...>;

		template<typename ComponentType>
		using PackedContainerType = PackedStableComponentContainer<ComponentType>*;

	public:
		Query() = default;

		Query(Container* container) :
			m_ContainerContext(container, true)
		{
			AddToContainer();
		}

		~Query()
		{
			RemoveFromContainer();
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


		Query(const Query& other) :
			m_ContainerContext(other.m_ContainerContext),
			m_FilterConfig(other.m_FilterConfig)
		{
			AddToContainer();
		}

		Query& operator=(const Query& other)
		{
			if (this != &other)
			{
				RemoveFromContainer();

				m_FilterConfig = other.m_FilterConfig;
				SetContainer(other.GetContainer());
				AddToContainer();
			}

			return *this;
		}

		Query(Query&& other) noexcept :
			m_ContainerContext(std::move(other.m_ContainerContext)),
			m_FilterConfig(std::move(other.m_FilterConfig))
		{
			other.SetContainer(nullptr);
			other.m_FilterConfig.Clear();
			AddToContainer();
		}

		Query& operator=(Query&& other) noexcept
		{
			if (this != &other)
			{
				RemoveFromContainer();

				m_FilterConfig = std::move(other.m_FilterConfig);
				m_ContainerContext = std::move(other.m_ContainerContext);
				other.SetContainer(nullptr);

				AddToContainer();
			}

			return *this;
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

		template<TComponentOrTagConcept... WithoutTypes>
		Query& Without()
		{
			m_IsDirty = true;
			m_FilterConfig.Without<WithoutTypes...>();
			return *this;
		}

		template<TComponentOrTagConcept... WithAnyTypes>
		Query& WithAny()
		{
			m_IsDirty = true;
			m_FilterConfig.WithAny<WithAnyTypes...>();
			return *this;
		}

		template<TComponentOrTagConcept... WithTypes>
		Query& With()
		{
			m_IsDirty = true;
			m_FilterConfig.With<WithTypes...>();
			return *this;
		}

		void ClearFilters()
		{
			if (m_FilterConfig.Clear())
			{
				m_IsDirty = true;
			}
		}

		/// <summary>
		/// Iterates over entities in archetypes from first to last. During iteration with this method creating, destroying and adding or removing component is forbidden on all entities, because it can cause undefined behavior. 
		/// Destroying entites and adding or removing component to any entity, can cause that iteration index will go out of bound. 
		/// Creating new entities will not cause index out of bound but if created entity has components which satisfys this query, it is undefined if that entity will be iterated or not in this function. If created entity will be placed in archetype that is not valid for this query it is safe to create it.
		/// </summary>
		/// <typeparam name="Callable"></typeparam>
		/// <param name="func"></param>
		template<typename Callable>
			requires query_callable<Callable, ComponentsTypes...>
		inline void ForEach(Callable&& func) noexcept
		{
			if (!IsValid()) return;
			FetchInternal();

			Container* container = m_ContainerContext.GetContainer();
			auto& archetypeContexts = m_ContainerContext.GetArchetypeContexts();
			const uint64_t contextCount = archetypeContexts.size();

			if constexpr (is_invocable_with_entity_v<Callable, ComponentsTypes...>)
			{
				Entity entityBuffer = {};
				entityBuffer.SetLifeTimeData_Internal(container->GetLifeTimeData());

				for (const ArchetypeContextType& ctx : archetypeContexts)
				{
					ctx.ForEach_WithEntity(func, entityBuffer);
				}
			}
			else
			{
				for (const ArchetypeContextType& ctx : archetypeContexts)
				{
					ctx.ForEach(func);
				}
			}
		}

		/// <summary>
		/// Works exacly like ForEach.
		/// There may be need to iterate over entities during certian component creattion or enable callbacks. In such cases destruction of component or entity can be deffered if functions like "Container::InvokeEntitesOnCreateListeners" are used. At that moment entities are not removed from archetype, but their records are invalidated. This function checks during iteration whether entity record is valid. It is not default behavior for iteration methods, as they are optimized for maximum performance.
		/// </summary>
		/// <typeparam name="Callable"></typeparam>
		/// <param name="func"></param>
		template<typename Callable>
			requires query_callable<Callable, ComponentsTypes...>
		inline void ForEach_Safe(Callable&& func) noexcept
		{
			if (!IsValid()) return;
			FetchInternal();

			Container* container = m_ContainerContext.GetContainer();
			auto& archetypeContexts = m_ContainerContext.GetArchetypeContexts();
			const uint64_t contextCount = archetypeContexts.size();

			if constexpr (is_invocable_with_entity_v<Callable, ComponentsTypes...>)
			{
				Entity entityBuffer = {};
				entityBuffer.SetLifeTimeData_Internal(container->GetLifeTimeData());

				for (const ArchetypeContextType& ctx : archetypeContexts)
				{
					ctx.ForEach_WithEntity_Safe(func, entityBuffer);
				}
			}
			else
			{
				for (const ArchetypeContextType& ctx : archetypeContexts)
				{
					ctx.ForEach_Safe(func);
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
			requires query_callable<Callable, ComponentsTypes...>
		void ForEachBackward(Callable&& func) noexcept
		{
			if (!IsValid()) return;
			FetchInternal();

			Container* container = m_ContainerContext.GetContainer();
			auto& archetypeContexts = m_ContainerContext.GetArchetypeContexts();
			const uint64_t contextCount = archetypeContexts.size();

			if constexpr (is_invocable_with_entity_v<Callable, ComponentsTypes...>)
			{
				Entity entityBuffer = {};
				entityBuffer.SetLifeTimeData_Internal(container->GetLifeTimeData());

				for (const ArchetypeContextType& ctx : archetypeContexts)
				{
					ctx.ForEachBackward_WithEntity(func, entityBuffer);
				}
			}
			else
			{
				for (const ArchetypeContextType& ctx : archetypeContexts)
				{
					ctx.ForEachBackward(func);
				}
			}
		}

		/// <summary>
		/// Works exacly like ForEachBackward.
		/// There may be need to iterate over entities during certian component creattion or enable callbacks. In such cases destruction of component or entity can be deffered if functions like "Container::InvokeEntitesOnCreateListeners" are used. At that moment entities are not removed from archetype, but their records are invalidated. This function checks during iteration whether entity record is valid. It is not default behavior for iteration methods, as they are optimized for maximum performance.
		/// </summary>
		/// <typeparam name="Callable"></typeparam>
		/// <param name="func"></param>
		template<typename Callable>
			requires query_callable<Callable, ComponentsTypes...>
		void ForEachBackward_Safe(Callable&& func) noexcept
		{
			if (!IsValid()) return;
			FetchInternal();

			Container* container = m_ContainerContext.GetContainer();
			auto& archetypeContexts = m_ContainerContext.GetArchetypeContexts();
			const uint64_t contextCount = archetypeContexts.size();

			if constexpr (is_invocable_with_entity_v<Callable, ComponentsTypes...>)
			{
				Entity entityBuffer = {};
				entityBuffer.SetLifeTimeData_Internal(container->GetLifeTimeData());

				for (const ArchetypeContextType& ctx : archetypeContexts)
				{
					ctx.ForEachBackward_WithEntity_Safe(func, entityBuffer);
				}
			}
			else
			{
				for (const ArchetypeContextType& ctx : archetypeContexts)
				{
					ctx.ForEachBackward_Safe(func);
				}
			}
		}

		/// <summary>
		/// Same rules apply like in Foreach methods. But here iteration is for every entity even if entity is not active
		/// </summary>
		/// <typeparam name="Callable"></typeparam>
		/// <param name="func"></param>
		template<typename Callable>
			requires query_callable<Callable, ComponentsTypes...>
		void ForEach_IngoreEntityActiveState(Callable&& func)
		{
			if (!IsValid()) return;
			FetchInternal();

			Container* container = m_ContainerContext.GetContainer();
			auto& archetypeContexts = m_ContainerContext.GetArchetypeContexts();
			const uint64_t contextCount = archetypeContexts.size();

			if constexpr (is_invocable_with_entity_v<Callable, ComponentsTypes...>)
			{
				Entity entityBuffer = {};
				entityBuffer.SetLifeTimeData_Internal(container->GetLifeTimeData());

				for (const ArchetypeContextType& ctx : archetypeContexts)
				{
					ctx.ForEach_IngoreEntityActiveState_WithEntity(func, entityBuffer);
				}
			}
			else
			{
				for (const ArchetypeContextType& ctx : archetypeContexts)
				{
					ctx.ForEach_IngoreEntityActiveState(func);
				}
			}
		}

		/// <summary>
		/// Iterate over all entities (enabled and disabled), if func returns expresion which evaluates to true, iteration is stoped, and function return
		/// </summary>
		/// <typeparam name="Callable"></typeparam>
		/// <param name="func"></param>
		template<typename Callable>
			requires query_find_callable<Callable, ComponentsTypes...>
		void Find(Callable&& func)
		{
			if (!IsValid()) return;
			FetchInternal();

			Container* container = m_ContainerContext.GetContainer();
			auto& archetypeContexts = m_ContainerContext.GetArchetypeContexts();
			const uint64_t contextCount = archetypeContexts.size();

			if constexpr (is_invocable_with_entity_v<Callable, ComponentsTypes...>)
			{
				Entity entityBuffer = {};
				entityBuffer.SetLifeTimeData_Internal(container->GetLifeTimeData());

				for (const ArchetypeContextType& ctx : archetypeContexts)
				{
					if (ctx.Find_WithEntity(func, entityBuffer))
					{
						return;
					}
				}
			}
			else
			{
				for (const ArchetypeContextType& ctx : archetypeContexts)
				{
					if (ctx.Find(func))
					{
						return;
					}
				}
			}
		}

		/// <summary>
		/// Iterate over enabled entities, if func returns expresion which evaluates to true, iteration is stoped, and function return
		/// </summary>
		/// <typeparam name="Callable"></typeparam>
		/// <param name="func"></param>
		template<typename Callable>
			requires query_find_callable<Callable, ComponentsTypes...>
		void FindEnabled(Callable&& func)
		{
			if (!IsValid()) return;
			FetchInternal();

			Container* container = m_ContainerContext.GetContainer();
			auto& archetypeContexts = m_ContainerContext.GetArchetypeContexts();
			const uint64_t contextCount = archetypeContexts.size();

			if constexpr (is_invocable_with_entity_v<Callable, ComponentsTypes...>)
			{
				Entity entityBuffer = {};
				entityBuffer.SetLifeTimeData_Internal(container->GetLifeTimeData());

				for (const ArchetypeContextType& ctx : archetypeContexts)
				{
					if (ctx.FindEnabled_WithEntity(func, entityBuffer))
					{
						return;
					}
				}
			}
			else
			{
				for (const ArchetypeContextType& ctx : archetypeContexts)
				{
					if (ctx.FindEnabled(func))
					{
						return;
					}
				}
			}
		}

		/// <summary>
		/// Iterate over disabled entities, if func returns expresion which evaluates to true, iteration is stoped, and function return
		/// </summary>
		/// <typeparam name="Callable"></typeparam>
		/// <param name="func"></param>
		template<typename Callable>
			requires query_find_callable<Callable, ComponentsTypes...>
		void FindDisabled(Callable&& func)
		{
			if (!IsValid()) return;
			FetchInternal();

			Container* container = m_ContainerContext.GetContainer();
			auto& archetypeContexts = m_ContainerContext.GetArchetypeContexts();
			const uint64_t contextCount = archetypeContexts.size();

			if constexpr (is_invocable_with_entity_v<Callable, ComponentsTypes...>)
			{
				Entity entityBuffer = {};
				entityBuffer.SetLifeTimeData_Internal(container->GetLifeTimeData());

				for (const ArchetypeContextType& ctx : archetypeContexts)
				{
					if (ctx.FindDisabled_WithEntity(func, entityBuffer))
					{
						return;
					}
				}
			}
			else
			{
				for (const ArchetypeContextType& ctx : archetypeContexts)
				{
					if (ctx.FindDisabled(func))
					{
						return;
					}
				}
			}
		}

		inline void Fetch()
		{
			if (!IsValid()) return;
			FetchInternal();
		}

		/// <summary>
		/// Checks if entity belong to this query.
		/// </summary>
		/// <param name="entity"></param>
		/// <returns></returns>
		[[nodiscard]] bool Contain(const decs::Entity& entity)
		{
			if (entity.IsValid())
			{
				Fetch();
				return m_ContainerContext.ContainsArchetype(entity.GetArchetype());
			}
			return false;
		}

	private:
		QueryFilterConfigType m_FilterConfig{};
		IterationContainerContext<drop_const_t<ComponentsTypes>...> m_ContainerContext{};

		bool m_IsDirty = true;

	private:
		void FetchInternal()
		{
			if (m_IsDirty)
			{
				m_IsDirty = false;
				Invalidate();
			}

			m_ContainerContext.Fetch(m_FilterConfig);
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
			if constexpr (is_invocable_with_entity_v<Callable, ComponentsTypes...>)
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

		void Invalidate()
		{
			m_ContainerContext.Clear();
		}


	#pragma region IQuery implementation
	private:
		void TryAddArchetype(const Archetype& archetype) override
		{
			m_ContainerContext.TryAddArchetype(archetype, m_FilterConfig);
		}

		void TryRemoveArchetpye(const Archetype& archetype) override
		{
			m_ContainerContext.TryRemoveArchetype(archetype);
		}

		/// <summary>
		/// Called when query manager destructor is invoked and query manager has queries.
		/// </summary>
		void OnQueryManagerDestroy() override
		{
			m_ContainerContext.SetContainer(nullptr);
		}

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
	#pragma endregion

	#pragma region BATCH ITERATOR
	public:
		class BatchIterator
		{
			using QueryType = Query<ComponentsTypes...>;

			template<component_concept... Types>
			friend class Query;
		public:
			BatchIterator() {}

			BatchIterator(
				QueryType* query,
				uint64_t firstArchetypeIndex,
				uint64_t firstIterationIndex,
				uint64_t entitiesCount
			) :
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
				requires query_callable<Callable, ComponentsTypes...>
			inline void ForEach(Callable&& func) const
			{
				if (!IsValid())
				{
					return;
				}

				Container* container = m_Query->GetContainer();
				auto& archetypeContexts = m_Query->m_ContainerContext.GetArchetypeContexts();

				Entity entityBuffer = {};
				if constexpr (is_invocable_with_entity_v<Callable, ComponentsTypes...>)
				{
					entityBuffer.SetLifeTimeData_Internal(container->GetLifeTimeData());
				}

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

					if constexpr (is_invocable_with_entity_v<Callable, ComponentsTypes...>)
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

			/// <summary>
			/// Same rules apply like in Foreach methods. But here iteration is for every entity even if entity is not active
			/// </summary>
			/// <typeparam name="Callable"></typeparam>
			/// <param name="func"></param>
			template<typename Callable>
				requires query_callable<Callable, ComponentsTypes...>
			inline void ForEach_IngoreEntityActiveState(Callable&& func) const
			{
				if (!IsValid())
				{
					return;
				}

				Container* container = m_Query->GetContainer();
				auto& archetypeContexts = m_Query->m_ContainerContext.GetArchetypeContexts();

				Entity entityBuffer = {};
				if constexpr (is_invocable_with_entity_v<Callable, ComponentsTypes...>)
				{
					entityBuffer.SetLifeTimeData_Internal(container->GetLifeTimeData());
				}

				uint64_t contextIndex = m_FirstArchetypeIndex;
				uint64_t contextCount = archetypeContexts.size();

				uint64_t leftEntitiesToIterate = m_EntitiesCount;

				for (; contextIndex < contextCount; contextIndex++)
				{
					const ArchetypeContextType& ctx = archetypeContexts[contextIndex];
					uint64_t ctxEntityCount = ctx.GetEntityCount();
					if (ctxEntityCount == 0) continue;

					const auto& containersTuple = ctx.GetContainersTuple();

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

					if constexpr (is_invocable_with_entity_v<Callable, ComponentsTypes...>)
					{
						ctx.ForEachFromTo_IgnoreActiveState_WithEntity(func, entityBuffer, startEntityIdx, iterationsCount);
					}
					else
					{
						ctx.ForEachFromTo_IgnoreActiveState(func, startEntityIdx, iterationsCount);
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
			ecsVector<BatchIterator>& iterators,
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