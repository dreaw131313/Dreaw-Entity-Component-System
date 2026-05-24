#pragma once
#include "IterationCore.h"

namespace decs
{
	class IMultiQuery
	{
	public:
		virtual ~IMultiQuery()
		{

		}

		virtual bool AddContainer(Container* container, bool bIsEnabled = true) = 0;
		virtual bool RemoveContainer(Container* container) = 0;
		virtual void SetContainerEnabled(Container* container, bool isEnabled) = 0;
	};

	template<ComponentConcept... ComponentsTypes>
	class MultiQuery : public IMultiQuery
	{
		static_assert(!decs::contain_tags_v<ComponentsTypes...>, "MultiQuery must not use tags in as ComponentTypes!");

	private:
		using ContainerContextType = IterationContainerContext<drop_const_t<ComponentsTypes>...>;
		using ArchetypeContextType = ContainerContextType::ArchetypeContextType;
		using ContainersTupleType = ArchetypeContextType::ContainersTuple;
		using QueryFilterConfigType = QueryFiltersConfig<drop_const_t<ComponentsTypes>...>;

		template<typename TComponent>
		using PackedContainerType = PackedStableComponentContainer<TComponent>*;

	public:
		MultiQuery() = default;

		[[nodiscard]] uint64_t GetEntityCount()
		{
			Fetch();

			uint64_t entityCount = 0;


			return entityCount;
		}

		template<TComponentOrTagConcept... WithoutTypes>
		MultiQuery& Without()
		{
			m_IsDirty = true;
			m_FilterConfig.Without<WithoutTypes...>();
			return *this;
		}

		template<TComponentOrTagConcept... WithAnyTypes>
		MultiQuery& WithAny()
		{
			m_IsDirty = true;
			m_FilterConfig.WithAny<WithAnyTypes...>();
			return *this;
		}

		template<TComponentOrTagConcept... WithTypes>
		MultiQuery& With()
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
			Fetch();

			uint64_t contextSize = m_ContainerContexts.size();
			for (uint64_t containerContextIndex = 0; containerContextIndex < contextSize; containerContextIndex++)
			{
				ContainerContextType& containerContext = m_ContainerContexts[containerContextIndex];
				if (!containerContext.IsValidAndEnabled())
				{
					continue; // Skip if container context is disabled
				}

				if constexpr (is_invocable_with_entity_v<Callable, ComponentsTypes...>)
				{
					decs::Entity entityBuffer = {};
					entityBuffer.SetLifeTimeData_Internal(containerContext.m_Container->GetLifeTimeData());

					for (const auto& ctx : containerContext.m_ArchetypesContexts)
					{
						ctx.ForEach_WithEntity(func, entityBuffer);
					}
				}
				else
				{
					for (const auto& ctx : containerContext.m_ArchetypesContexts)
					{
						ctx.ForEach(func);
					}
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
			Fetch();

			uint64_t contextSize = m_ContainerContexts.size();
			for (uint64_t containerContextIndex = 0; containerContextIndex < contextSize; containerContextIndex++)
			{
				ContainerContextType& containerContext = m_ContainerContexts[containerContextIndex];
				if (!containerContext.IsValidAndEnabled())
				{
					continue; // Skip if container context is disabled
				}

				if constexpr (is_invocable_with_entity_v<Callable, ComponentsTypes...>)
				{
					decs::Entity entityBuffer = {};
					entityBuffer.SetLifeTimeData_Internal(containerContext.m_Container->GetLifeTimeData());

					for (const auto& ctx : containerContext.m_ArchetypesContexts)
					{
						ctx.ForEach_WithEntity_Safe(func, entityBuffer);
					}
				}
				else
				{
					for (const auto& ctx : containerContext.m_ArchetypesContexts)
					{
						ctx.ForEach_Safe(func);
					}
				}
			}
		}

		/// <summary>
		/// Iterates over entities in archetypes from last to first. During iteration with this method only destroying current entity is not forbidden. Any other operation on all entities are undefined behaviors. 
		/// Creating new entities will not cause index out of bound but if created entity has components which satisfys this query, it is undefined if that entity will be iterated or not in this function. 
		/// Destroying entities other than currently iterated and removing or adding component from them can cause index out of bound.
		/// Creating new entities will not cause index out of bound, but if created entity has components which satisfys this query, it is undefined if that entity will be iterated or not in this function. If created entity will be placed in archetype that is not valid for this query it is safe to create it.
		/// Adding or removing components from currnet iterated entity will not cause index out of bound, but it can cause that this entity will be iterated again. If after add or remove component, entity will be moved to archetype which is not valid for this query it is known that entity will not be iterated again.
		/// Desrtoying 
		/// </summary>
		/// <typeparam name="Callable"></typeparam>
		/// <param name="func"></param>
		template<typename Callable>
			requires query_callable<Callable, ComponentsTypes...>
		void ForEachBackward(Callable&& func) noexcept
		{
			Fetch();

			uint64_t contextSize = m_ContainerContexts.size();
			for (uint64_t containerContextIndex = 0; containerContextIndex < contextSize; containerContextIndex++)
			{
				ContainerContextType& containerContext = m_ContainerContexts[containerContextIndex];
				if (!containerContext.IsValidAndEnabled())
				{
					continue; // Skip if container context is disabled
				}

				if constexpr (is_invocable_with_entity_v<Callable, ComponentsTypes...>)
				{
					decs::Entity entityBuffer = {};
					entityBuffer.SetLifeTimeData_Internal(containerContext.m_Container->GetLifeTimeData());

					for (const auto& ctx : containerContext.m_ArchetypesContexts)
					{
						ctx.ForEachBackward_WithEntity(func, entityBuffer);
					}
				}
				else
				{
					for (const auto& ctx : containerContext.m_ArchetypesContexts)
					{
						ctx.ForEachBackward(func);
					}
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
			Fetch();

			uint64_t contextSize = m_ContainerContexts.size();
			for (uint64_t containerContextIndex = 0; containerContextIndex < contextSize; containerContextIndex++)
			{
				ContainerContextType& containerContext = m_ContainerContexts[containerContextIndex];
				if (!containerContext.IsValidAndEnabled())
				{
					continue; // Skip if container context is disabled
				}

				if constexpr (is_invocable_with_entity_v<Callable, ComponentsTypes...>)
				{
					decs::Entity entityBuffer = {};
					entityBuffer.SetLifeTimeData_Internal(containerContext.m_Container->GetLifeTimeData());

					for (const auto& ctx : containerContext.m_ArchetypesContexts)
					{
						ctx.ForEachBackward_WithEntity_Safe(func, entityBuffer);
					}
				}
				else
				{
					for (const auto& ctx : containerContext.m_ArchetypesContexts)
					{
						ctx.ForEachBackward_Safe(func);
					}
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
			Fetch();

			uint64_t contextSize = m_ContainerContexts.size();
			for (uint64_t containerContextIndex = 0; containerContextIndex < contextSize; containerContextIndex++)
			{
				ContainerContextType& containerContext = m_ContainerContexts[containerContextIndex];
				if (!containerContext.IsValidAndEnabled())
				{
					continue; // Skip if container context is disabled
				}

				if constexpr (is_invocable_with_entity_v<Callable, ComponentsTypes...>)
				{
					decs::Entity entityBuffer = {};
					entityBuffer.SetLifeTimeData_Internal(containerContext.m_Container->GetLifeTimeData());

					for (const auto& ctx : containerContext.m_ArchetypesContexts)
					{
						ctx.ForEach_IngoreEntityActiveState_WithEntity(func, entityBuffer);
					}
				}
				else
				{
					for (const auto& ctx : containerContext.m_ArchetypesContexts)
					{
						ctx.ForEach_IngoreEntityActiveState(func);
					}
				}
			}
		}

		bool AddContainer(Container* container, bool bIsEnabled = true) override
		{
			auto& contextIndex = m_ContainerContextsIndexes[container];
			if (contextIndex >= m_ContainerContexts.size() || m_ContainerContexts[contextIndex].m_Container != container)
			{
				contextIndex = m_ContainerContexts.size();
				m_ContainerContexts.emplace_back(container, bIsEnabled);
				return true;
			}
			return false;
		}

		bool RemoveContainer(Container* container) override
		{
			auto it = m_ContainerContextsIndexes.find(container);
			if (it != m_ContainerContextsIndexes.end())
			{
				const uint64_t index = it->second;
				m_ContainerContextsIndexes.erase(it);

				if (index < (m_ContainerContexts.size() - 1))
				{
					auto& lastContext = m_ContainerContexts.back();
					m_ContainerContextsIndexes[lastContext.m_Container] = index;
					m_ContainerContexts[index] = lastContext;
				}
				m_ContainerContexts.pop_back();

				return true;
			}
			return false;
		}

		void SetContainerEnabled(Container* container, bool bIsEnabled) override
		{
			auto it = m_ContainerContextsIndexes.find(container);
			if (it != m_ContainerContextsIndexes.end())
			{
				ContainerContextType& context = m_ContainerContexts[it->second];
				context.m_bIsEnabled = bIsEnabled;
			}
		}

		[[nodiscard]] bool Contain(const decs::Entity& entity)
		{
			if (entity.IsValid())
			{
				auto containerCtxIdxIt = m_ContainerContextsIndexes.find(entity.GetContainer());
				if (containerCtxIdxIt != m_ContainerContextsIndexes.end())
				{
					auto& ctx = m_ContainerContexts[containerCtxIdxIt->second];
					ctx.Fetch(m_FilterConfig);
					return ctx.Contain(entity);
				}
			}
			return false;
		}

		void Fetch()
		{
			uint64_t containerContextsSize = m_ContainerContexts.size();
			if (m_IsDirty)
			{
				m_IsDirty = false;
				for (uint64_t i = 0; i < containerContextsSize; i++)
				{
					ContainerContextType& containerContext = m_ContainerContexts[i];
					containerContext.Clear();
					containerContext.Fetch(m_FilterConfig);
				}
			}
			else
			{
				for (uint64_t i = 0; i < containerContextsSize; i++)
				{
					ContainerContextType& containerContext = m_ContainerContexts[i];
					m_ContainerContexts[i].Fetch(m_FilterConfig);
				}
			}
		}
	private:
		ecsMap<Container*, uint64_t> m_ContainerContextsIndexes;
		QueryFilterConfigType m_FilterConfig{};

		ecsVector<ContainerContextType> m_ContainerContexts = {};

		bool m_IsDirty = true;

	private:
		uint64_t CalculateEntityCount()
		{
			uint64_t entitiesCount = 0;
			for (uint64_t i = 0; i < m_ContainerContexts.size(); i++)
			{
				ContainerContextType& containerCtx = m_ContainerContexts[i];
				uint64_t archetypesCtxCount = containerCtx.m_ArchetypesContexts.size();
				for (uint64_t j = 0; j < archetypesCtxCount; j++)
				{
					ArchetypeContextType& archetypeCtx = containerCtx.m_ArchetypesContexts[j];
					entitiesCount += archetypeCtx.GetEntityCount();
				}
			}
			return entitiesCount;
		}

	private:
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
				entityBuffer.SetWithoutLifeTimeDataInvalidation_Internal(entityData);
				func(
					entityBuffer,
					std::get<PackedContainerType<drop_const_t<ComponentsTypes>>>(containersTuple)->GetAsRef(entityIndexInArchetype)...
				);
			}
			else
			{
				func(std::get<PackedContainerType<drop_const_t<ComponentsTypes>>>(containersTuple)->GetAsRef(entityIndexInArchetype)...);
			}
		}

	public:
		struct BatchIterator
		{
			using QueryType = typename MultiQuery<ComponentsTypes...>;

			friend class QueryType;
		public:
			BatchIterator()
			{

			}

			BatchIterator(
				QueryType* query,
				uint64_t startContainerElementIndex,
				uint64_t startArchetypeIndex,
				uint64_t startEntityIndex,
				uint64_t entitiesCount
			):
				m_Query(query),
				m_StartContainerContextIndex(startContainerElementIndex),
				m_StartArchetypeIndex(startArchetypeIndex),
				m_StartEntityIndex(startEntityIndex),
				m_EntitiesCount(entitiesCount)
			{

			}

			template<typename Callable>
				requires query_callable<Callable, ComponentsTypes...>
			void ForEach(Callable&& func) noexcept
			{
				auto& containerContexts = m_Query->m_ContainerContexts;
				decs::Entity entityBuffer = {};

				uint64_t leftEntitiesToIterate = m_EntitiesCount;

				uint64_t contextSize = containerContexts.size();
				for (uint64_t containerContextIndex = m_StartContainerContextIndex; containerContextIndex < contextSize; containerContextIndex++)
				{
					ContainerContextType& containerContext = containerContexts[containerContextIndex];
					if (!containerContext.m_bIsEnabled)
					{
						continue; // Skip if container context is disabled
					}

					if constexpr (is_invocable_with_entity_v<Callable, ComponentsTypes...>)
					{
						entityBuffer.SetLifeTimeData_Internal(containerContext.m_Container->GetLifeTimeData());
					}

					auto archetypesContexts = containerContext.m_ArchetypesContexts.data();
					const uint64_t archetypesContextsCount = containerContext.m_ArchetypesContexts.size();

					uint64_t archetypeContextIdx;
					uint64_t startEntitiyIndex;
					if (containerContextIndex == m_StartContainerContextIndex)
					{
						archetypeContextIdx = m_StartArchetypeIndex;
						startEntitiyIndex = m_StartEntityIndex;
					}
					else
					{
						archetypeContextIdx = 0;
						startEntitiyIndex = 0;
					}

					for (; archetypeContextIdx < archetypesContextsCount; archetypeContextIdx++)
					{
						const ArchetypeContextType& ctx = archetypesContexts[archetypeContextIdx];
						uint64_t ctxEntityCount = ctx.GetEntityCount();
						if (ctxEntityCount == 0) continue;

						uint64_t leftEntitiesInArchetypeToIterate = ctxEntityCount - startEntitiyIndex;
						uint64_t entitiesCount;

						if (leftEntitiesToIterate <= leftEntitiesInArchetypeToIterate)
						{
							entitiesCount = leftEntitiesToIterate + startEntitiyIndex;
							leftEntitiesToIterate = 0;
						}
						else
						{
							entitiesCount = leftEntitiesInArchetypeToIterate + startEntitiyIndex;
							leftEntitiesToIterate -= leftEntitiesInArchetypeToIterate;
						}

						if constexpr (is_invocable_with_entity_v<Callable, ComponentsTypes...>)
						{
							ctx.ForEachFromTo_WithEntity(func, entityBuffer, startEntitiyIndex, entitiesCount);
						}
						else
						{
							ctx.ForEachFromTo(func, startEntitiyIndex, entitiesCount);
						}

						if (leftEntitiesToIterate == 0)
						{
							return;
						}
					}
				}
			}

			template<typename Callable>
				requires query_callable<Callable, ComponentsTypes...>
			void ForEach_IngoreEntityActiveState(Callable&& func)
			{
				auto& containerContexts = m_Query->m_ContainerContexts;
				decs::Entity entityBuffer = {};

				uint64_t leftEntitiesToIterate = m_EntitiesCount;

				uint64_t contextSize = containerContexts.size();
				for (uint64_t containerContextIndex = m_StartContainerContextIndex; containerContextIndex < contextSize; containerContextIndex++)
				{
					ContainerContextType& containerContext = containerContexts[containerContextIndex];
					if (!containerContext.m_bIsEnabled)
					{
						continue; // Skip if container context is disabled
					}

					if constexpr (is_invocable_with_entity_v<Callable, ComponentsTypes...>)
					{
						entityBuffer.SetLifeTimeData_Internal(containerContext.m_Container->GetLifeTimeData());
					}

					auto archetypesContexts = containerContext.m_ArchetypesContexts.data();
					const uint64_t archetypesContextsCount = containerContext.m_ArchetypesContexts.size();

					uint64_t archetypeContextIdx;
					uint64_t startEntitiyIndex;
					if (containerContextIndex == m_StartContainerContextIndex)
					{
						archetypeContextIdx = m_StartArchetypeIndex;
						startEntitiyIndex = m_StartEntityIndex;
					}
					else
					{
						archetypeContextIdx = 0;
						startEntitiyIndex = 0;
					}

					for (; archetypeContextIdx < archetypesContextsCount; archetypeContextIdx++)
					{
						const ArchetypeContextType& ctx = archetypesContexts[archetypeContextIdx];
						uint64_t ctxEntityCount = ctx.GetEntityCount();
						if (ctxEntityCount == 0) continue;

						uint64_t leftEntitiesInArchetypeToIterate = ctxEntityCount - startEntitiyIndex;
						uint64_t entitiesCount;

						if (leftEntitiesToIterate <= leftEntitiesInArchetypeToIterate)
						{
							entitiesCount = leftEntitiesToIterate + startEntitiyIndex;
							leftEntitiesToIterate = 0;
						}
						else
						{
							entitiesCount = leftEntitiesInArchetypeToIterate + startEntitiyIndex;
							leftEntitiesToIterate -= leftEntitiesInArchetypeToIterate;
						}

						if constexpr (is_invocable_with_entity_v<Callable, ComponentsTypes...>)
						{
							ctx.ForEachFromTo_IgnoreActiveState_WithEntity(func, entityBuffer, startEntitiyIndex, entitiesCount);
						}
						else
						{
							ctx.ForEachFromTo_IgnoreActiveState(func, startEntitiyIndex, entitiesCount);
						}

						if (leftEntitiesToIterate == 0)
						{
							return;
						}
					}
				}
			}

		private:
			QueryType* m_Query = nullptr;
			uint64_t m_StartContainerContextIndex = 0;
			uint64_t m_StartArchetypeIndex = 0;
			uint64_t m_StartEntityIndex = 0;
			uint64_t m_EntitiesCount = 0;

		};

	public:
		void CreateBatchIterators(
			ecsVector<BatchIterator>& iterators,
			uint64_t desiredBatchesCount,
			uint64_t minBatchSize
		)
		{
			Fetch();
			uint64_t entitiesCount = CalculateEntityCount();

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

			BatchIterator* iterator = nullptr;

			uint64_t contextSize = m_ContainerContexts.size();
			for (uint64_t containerContextIndex = 0; containerContextIndex < contextSize; containerContextIndex++)
			{
				ContainerContextType& containerContext = m_ContainerContexts[containerContextIndex];
				auto archetypesContexts = containerContext.m_ArchetypesContexts.data();
				const uint64_t archetypesContextsCount = containerContext.m_ArchetypesContexts.size();

				for (uint64_t archetypeContextIdx = 0; archetypeContextIdx < archetypesContextsCount; archetypeContextIdx++)
				{
					ArchetypeContextType& ctx = archetypesContexts[archetypeContextIdx];
					uint64_t ctxEntitiesCount = ctx.GetEntityCount();
					if (ctxEntitiesCount == 0) continue;

					uint64_t currentEntityIndex = 0;

					while (ctxEntitiesCount > 0)
					{
						if (iterator == nullptr)
						{
							iterator = &iterators.emplace_back(this, containerContextIndex, archetypeContextIdx, currentEntityIndex, 0);
						}

						uint64_t neededEntitiesCount = finalBatchSize - iterator->m_EntitiesCount;

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
							iterator->m_EntitiesCount += entitiesCountToAddToIterator;
							currentEntityIndex += entitiesCountToAddToIterator;

							if (iterator->m_EntitiesCount == finalBatchSize)
							{
								iterator = nullptr;
							}
						}
						else
						{
							iterator = nullptr;
						}
					}
				}
			}
		}

		void CreateBatchIteratorsWithMaxNumberPerBatch(
			ecsVector<BatchIterator>& iterators,
			uint32_t maxBatchSize
		)
		{
			Fetch();
			BatchIterator* currentIterator = nullptr;

			uint64_t contextSize = m_ContainerContexts.size();
			for (uint64_t containerContextIndex = 0; containerContextIndex < contextSize; containerContextIndex++)
			{
				ContainerContextType& containerContext = m_ContainerContexts[containerContextIndex];
				auto archetypesContexts = containerContext.m_ArchetypesContexts.data();
				const uint64_t archetypesContextsCount = containerContext.m_ArchetypesContexts.size();

				for (uint64_t archetypeContextIdx = 0; archetypeContextIdx < archetypesContextsCount; archetypeContextIdx++)
				{
					ArchetypeContextType& ctx = archetypesContexts[archetypeContextIdx];
					const uint32_t ctxEntityCount = static_cast<uint32_t>(ctx.GetEntityCount());
					uint32_t currentEntityIndex = 0;

					while (currentEntityIndex < ctxEntityCount)
					{
						if (currentIterator == nullptr)
						{
							currentIterator = &iterators.emplace_back(this, containerContextIndex, archetypeContextIdx, currentEntityIndex, 0);
						}

						uint32_t iteratorEntityCount = static_cast<uint32_t>(currentIterator->m_EntitiesCount);
						uint32_t availableArchetypeEntities = ctxEntityCount - currentEntityIndex;

						uint32_t iteratorNeededEntities = maxBatchSize - iteratorEntityCount;

						if (availableArchetypeEntities >= iteratorNeededEntities)
						{
							currentEntityIndex += iteratorNeededEntities;
							currentIterator->m_EntitiesCount += iteratorNeededEntities;
							currentIterator = nullptr;
						}
						else
						{
							currentIterator->m_EntitiesCount += availableArchetypeEntities;
							currentEntityIndex += availableArchetypeEntities;
						}
					}
				}
			}
		}
	};
}