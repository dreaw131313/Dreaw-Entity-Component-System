#pragma once
#include "decs\Core.h"
#include "decs\Type.h"
#include "decs\Entity.h"
#include "decs\Container.h"

#include "IterationCore.h"

#include "decs/Component/Component.h"

#include <type_traits>

namespace decs
{
	template<TComponentConcept... ComponentsTypes>
	class Query
	{
		static_assert(!decs::contain_tags_v<ComponentsTypes...>, "Query must not use tags in as ComponentTypes!");

	private:
		using ArchetypeContextType = IterationArchetypeContext<drop_const_t<ComponentsTypes>...>;
		using ContainersTupleType = ArchetypeContextType::ContainersTuple;

		template<typename TComponent>
		using PackedContainerType = StablePackedContainer<TComponent>*;

	public:
		Query()
		{

		}

		Query(Container* container):
			m_Container(container)
		{

		}

		~Query()
		{

		}

		inline void SetContainer(Container* container)
		{
			if (container != m_Container)
			{
				m_IsDirty = true;
				m_Container = container;
				Invalidate();
			}
		}

		[[nodiscard]] inline Container* GetContainer() const { return m_Container; }

		[[nodiscard]] inline bool IsValid()const { return m_Container != nullptr; }

		[[nodiscard]] inline uint64_t GetEntityCount()
		{
			if (IsValid())
			{
				Fetch();

				uint64_t entityCount = 0;
				for (auto& archetypeCtx : m_ArchetypesContexts)
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
			if constexpr (sizeof...(WithoutTypes) == 0)
			{
				m_Without.clear();
			}
			else
			{
				m_Without.resize(sizeof...(WithoutTypes));
				find_type_ids<drop_const_t<WithoutTypes>...>(m_Without.data());
			}
			return *this;
		}

		template<TComponentOrTagConcept... WithAnyTypes>
		Query& WithAny()
		{
			m_IsDirty = true;
			if constexpr (sizeof...(WithAnyTypes) == 0)
			{
				m_WithAnyOf.clear();
			}
			else
			{
				m_WithAnyOf.resize(sizeof...(WithAnyTypes));
				find_type_ids<drop_const_t<WithAnyTypes>...>(m_WithAnyOf.data());
			}
			return *this;
		}

		template<TComponentOrTagConcept... WithTypes>
		Query& With()
		{
			m_IsDirty = true;
			if constexpr (sizeof...(WithTypes) == 0)
			{
				m_WithAll.clear();
			}
			else
			{
				m_WithAll.resize(sizeof...(WithTypes));
				find_type_ids<drop_const_t<WithTypes>...>(m_WithAll.data());
			}
			return *this;
		}

		void ClearFilters()
		{
			if (m_WithAll.size() > 0)
			{
				m_IsDirty = true;
				m_WithAll.clear();
			}
			if (m_WithAnyOf.size() > 0)
			{
				m_IsDirty = true;
				m_WithAnyOf.clear();
			}
			if (m_Without.size() > 0)
			{
				m_IsDirty = true;
				m_Without.clear();
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

			Entity entityBuffer = {};
			if constexpr (is_invocable_with_entity_v<Callable, ComponentsTypes...>)
			{
				entityBuffer.SetLifeTimeData_Internal(m_Container->GetLifeTimeData());
			}

			const uint64_t contextCount = m_ArchetypesContexts.size();
			for (uint64_t contextIndex = 0; contextIndex < contextCount; contextIndex++)
			{
				const ArchetypeContextType& ctx = m_ArchetypesContexts[contextIndex];
				uint64_t ctxEntityCount = ctx.GetEntityCount();
				if (ctxEntityCount == 0) continue;

				const auto& containersTuple = ctx.GetContainersTuple();
				const std::vector<ArchetypeEntityData>& entitiesData = ctx.GetArchetype()->m_EntitiesData;

				for (uint64_t idx = 0; idx < ctxEntityCount; idx++)
				{
					const auto& entityData = entitiesData[idx];
					if (entityData.IsActive())
					{
						InvokeEntityIteration(func, entityBuffer, *entityData.m_EntityData, idx, containersTuple);
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
			if (!IsValid()) return;
			FetchInternal();

			Entity entityBuffer = {};
			if constexpr (is_invocable_with_entity_v<Callable, ComponentsTypes...>)
			{
				entityBuffer.SetLifeTimeData_Internal(m_Container->GetLifeTimeData());
			}

			const uint64_t contextCount = m_ArchetypesContexts.size();
			for (uint64_t contextIndex = 0; contextIndex < contextCount; contextIndex++)
			{
				const ArchetypeContextType& ctx = m_ArchetypesContexts[contextIndex];
				uint64_t ctxEntityCount = ctx.GetEntityCount();
				if (ctxEntityCount == 0) continue;

				const auto& containersTuple = ctx.GetContainersTuple();
				const std::vector<ArchetypeEntityData>& entitiesData = ctx.GetArchetype()->m_EntitiesData;

				for (uint64_t idx = 0; idx < ctxEntityCount; idx++)
				{
					const auto& entityData = entitiesData[idx];
					if (entityData.m_EntityData != nullptr && entityData.IsActive())
					{
						InvokeEntityIteration(func, entityBuffer, *entityData.m_EntityData, idx, containersTuple);
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
		/// </summary>
		/// <typeparam name="Callable"></typeparam>
		/// <param name="func"></param>
		template<typename Callable>
			requires query_callable<Callable, ComponentsTypes...>
		void ForEachBackward(Callable&& func) noexcept
		{
			if (!IsValid()) return;
			FetchInternal();

			Entity entityBuffer = {};
			if constexpr (is_invocable_with_entity_v<Callable, ComponentsTypes...>)
			{
				entityBuffer.SetLifeTimeData_Internal(m_Container->GetLifeTimeData());
			}

			const uint64_t contextCount = m_ArchetypesContexts.size();
			for (uint64_t contextIndex = 0; contextIndex < contextCount; contextIndex++)
			{
				const ArchetypeContextType& ctx = m_ArchetypesContexts[contextIndex];
				uint64_t ctxEntityCount = ctx.GetEntityCount();
				if (ctxEntityCount == 0) continue;

				const auto& containersTuple = ctx.GetContainersTuple();
				const std::vector<ArchetypeEntityData>& entitiesData = ctx.GetArchetype()->m_EntitiesData;

				int64_t idx = ctxEntityCount - 1;

				for (; idx > -1; idx--)
				{
					const auto& entityData = entitiesData[idx];
					if (entityData.IsActive())
					{
						InvokeEntityIteration(func, entityBuffer, *entityData.m_EntityData, idx, containersTuple);
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
			if (!IsValid()) return;
			FetchInternal();

			Entity entityBuffer = {};
			if constexpr ( is_invocable_with_entity_v<Callable, ComponentsTypes...>)
			{
				entityBuffer.SetLifeTimeData_Internal(m_Container->GetLifeTimeData());
			}

			const uint64_t contextCount = m_ArchetypesContexts.size();
			for (uint64_t contextIndex = 0; contextIndex < contextCount; contextIndex++)
			{
				const ArchetypeContextType& ctx = m_ArchetypesContexts[contextIndex];
				uint64_t ctxEntityCount = ctx.GetEntityCount();
				if (ctxEntityCount == 0) continue;

				const auto& containersTuple = ctx.GetContainersTuple();
				const std::vector<ArchetypeEntityData>& entitiesData = ctx.GetArchetype()->m_EntitiesData;

				int64_t idx = ctxEntityCount - 1;

				for (; idx > -1; idx--)
				{
					const auto& entityData = entitiesData[idx];
					if (entityData.m_EntityData != nullptr && entityData.IsActive())
					{
						InvokeEntityIteration(func, entityBuffer, *entityData.m_EntityData, idx, containersTuple);
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
			if (!IsValid()) return;
			FetchInternal();

			Entity entityBuffer = {};
			if constexpr (is_invocable_with_entity_v<Callable, ComponentsTypes...>)
			{
				entityBuffer.SetLifeTimeData_Internal(m_Container->GetLifeTimeData());
			}

			const uint64_t contextCount = m_ArchetypesContexts.size();
			for (uint64_t contextIndex = 0; contextIndex < contextCount; contextIndex++)
			{
				const ArchetypeContextType& ctx = m_ArchetypesContexts[contextIndex];
				uint64_t ctxEntityCount = ctx.GetEntityCount();
				if (ctxEntityCount == 0) continue;

				const auto& containersTuple = ctx.GetContainersTuple();
				const std::vector<ArchetypeEntityData>& entitiesData = ctx.GetArchetype()->m_EntitiesData;

				for (uint64_t idx = 0; idx < ctxEntityCount; idx++)
				{
					const auto& entityData = entitiesData[idx];
					if (entityData.m_EntityData != nullptr)
					{
						InvokeEntityIteration(func, entityBuffer, *entityData.m_EntityData, idx, containersTuple);
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
				return m_ContainedArchetypes.find(entity.GetArchetype()) != m_ContainedArchetypes.end();
			}
			return false;
		}

	private:
		ecsSet<const Archetype*> m_ContainedArchetypes;
		TypeGroup<drop_const_t<ComponentsTypes>...> m_Includes = {};
		std::vector<TypeID> m_Without;
		std::vector<TypeID> m_WithAnyOf;
		std::vector<TypeID> m_WithAll;
		std::vector<ArchetypeContextType> m_ArchetypesContexts;

		// cache value to check if query should be updated:
		uint64_t m_ArchetypesCountDirty = 0;

		Container* m_Container = nullptr;
		bool m_IsDirty = true;

	private:
		void FetchInternal()
		{
			if (m_IsDirty)
			{
				m_IsDirty = false;
				Invalidate();
			}

			uint64_t containerArchetypesCount = m_Container->m_ArchetypesMap.ArchetypesCount();
			if (m_ArchetypesCountDirty != containerArchetypesCount)
			{
				uint64_t newArchetypesCount = containerArchetypesCount - m_ArchetypesCountDirty;
				uint64_t minComponentsCountInArchetype = GetMinComponentsCount();

				const ArchetypesMap& map = m_Container->m_ArchetypesMap;
				uint64_t maxComponentsInArchetype = map.MaxNumberOfTypesInArchetype();
				if (maxComponentsInArchetype < minComponentsCountInArchetype) return;

				if (newArchetypesCount > m_ArchetypesContexts.size())
				{
					// performing normal finding of archetypes
					auto group = GetBestArchetypesGroup();
					FetchArchetypesFromArchetypesGroup(group);
				}
				else
				{
					// checking only new archetypes:
					AddingArchetypesWithCheckingOnlyNewArchetypes(map, m_ArchetypesCountDirty, minComponentsCountInArchetype);
				}

				m_ArchetypesCountDirty = containerArchetypesCount;
			}
		}

		void CollectArchetypesEntityCount()
		{
			const uint64_t ctxCount = m_ArchetypesContexts.size();
			for (uint64_t ctxIdx = 0; ctxIdx < ctxCount; ctxIdx++)
			{
				m_ArchetypesContexts[ctxIdx].ValidateCachedEntityCount();
			}
		}

		inline uint64_t GetMinComponentsCount() const
		{
			uint64_t includesCount = sizeof...(ComponentsTypes);
			if (m_WithAnyOf.size() > 0) includesCount += 1;
			return sizeof...(ComponentsTypes) + m_WithAll.size();
		}

		void Invalidate()
		{
			m_ArchetypesContexts.clear();
			m_ContainedArchetypes.clear();
			m_ArchetypesCountDirty = 0;
		}

		inline bool ContainArchetype(Archetype* arch) const { return m_ContainedArchetypes.find(arch) != m_ContainedArchetypes.end(); }

		// FETCHING ARCHETYPE

		ArchetypesGroupByOneType* GetBestArchetypesGroup()
		{
			auto& groupsMap = m_Container->m_ArchetypesMap.m_ArchetypesGroupedByOneType;

			uint64_t bestArchetypesCount = std::numeric_limits<uint64_t>::max();
			ArchetypesGroupByOneType* bestGroup = nullptr;

			for (uint64_t i = 0; i < m_Includes.Size(); i++)
			{
				auto it = groupsMap.find(m_Includes[i]);
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

		void TryAddArchetypeFromGroup(Archetype& archetype)
		{
			if (!ContainArchetype(&archetype) && archetype.GetComponentAndTagCount())
			{
				// without test
				{
					uint64_t excludeCount = m_Without.size();
					for (int i = 0; i < excludeCount; i++)
					{
						if (archetype.ContainType(m_Without[i]))
						{
							return;
						}
					}
				}

				// with any test
				{
					uint64_t requiredAnyCount = m_WithAnyOf.size();
					bool containRequiredAny = requiredAnyCount == 0;

					for (int i = 0; i < requiredAnyCount; i++)
					{
						if (archetype.ContainType(m_WithAnyOf[i]))
						{
							containRequiredAny = true;
							break;
						}
					}
					if (!containRequiredAny) return;
				}

				// required all test
				{
					uint64_t requiredAllCount = m_WithAll.size();

					for (int i = 0; i < requiredAllCount; i++)
					{
						if (!archetype.ContainType(m_WithAll[i]))
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

		void FetchArchetypesFromArchetypesGroup(ArchetypesGroupByOneType* group)
		{
			if (group == nullptr) return;
			uint64_t minComponentsCount = GetMinComponentsCount();
			uint64_t maxComponentCountsInGroup = group->MaxComponentsCount();

			for (uint64_t i = minComponentsCount; i <= maxComponentCountsInGroup; i++)
			{
				auto archetypesToCheckPtr = group->GetArchetypesWithComponentsCount(i);
				if (archetypesToCheckPtr != nullptr)
				{
					for (auto& archetype : *archetypesToCheckPtr)
					{
						TryAddArchetypeFromGroup(*archetype);
					}
				}
			}
		}

		void AddingArchetypesWithCheckingOnlyNewArchetypes(
			const ArchetypesMap& map,
			uint64_t startArchetypesIndex,
			uint64_t minRequiredComponentsCount
		)
		{
			auto& archetypes = map.m_Archetypes;
			uint64_t archetypesCount = map.m_Archetypes.Size();
			for (uint64_t i = startArchetypesIndex; i < archetypesCount; i++)
			{
				Archetype& arch = archetypes[i];
				if (arch.GetComponentAndTagCount() >= minRequiredComponentsCount)
				{
					TryAddArchetypeFromGroup(arch);
				}
			}
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

	#pragma region BATCH ITERATOR
	public:
		class BatchIterator
		{
			using QueryType = Query<ComponentsTypes...>;

			template<TComponentConcept... Types>
			friend class Query;
		public:
			BatchIterator() {}

			BatchIterator(
				QueryType* query,
				const uint64_t& firstArchetypeIndex,
				const uint64_t& firstIterationIndex,
				const uint64_t& entitiesCount
			):
				m_IsValid(true),
				m_Query(query),
				m_FirstArchetypeIndex(firstArchetypeIndex),
				m_FirstIterationIndex(firstIterationIndex),
				m_EntitiesCount(entitiesCount)
			{
			}

			~BatchIterator() {}

			inline bool IsValid() { return m_IsValid; }

			template<typename Callable>
				requires query_callable<Callable, ComponentsTypes...>
			inline void ForEach(Callable&& func) const
			{
				if (!m_IsValid) return;

				Container* container = m_Query->m_Container;
				Entity entityBuffer = {};
				if constexpr (is_invocable_with_entity_v<Callable, ComponentsTypes...>)
				{
					entityBuffer.SetLifeTimeData_Internal(container->GetLifeTimeData());
				}

				uint64_t contextIndex = m_FirstArchetypeIndex;
				uint64_t contextCount = m_Query->m_ArchetypesContexts.size();
				ArchetypeContextType* archetypeContexts = m_Query->m_ArchetypesContexts.data();

				uint64_t leftEntitiesToIterate = m_EntitiesCount;

				for (; contextIndex < contextCount; contextIndex++)
				{
					const ArchetypeContextType& ctx = archetypeContexts[contextIndex];
					uint64_t ctxEntityCount = ctx.GetEntityCount();
					if (ctxEntityCount == 0) continue;

					const auto& containersTuple = ctx.GetContainersTuple();
					const std::vector<ArchetypeEntityData>& entitiesData = ctx.GetArchetype()->m_EntitiesData;

					uint64_t iterationIndex;
					uint64_t iterationsCount;

					if (contextIndex == m_FirstArchetypeIndex)
					{
						iterationIndex = m_FirstIterationIndex;
					}
					else
					{
						iterationIndex = 0;
					}

					uint64_t leftEntitiesInContext = ctxEntityCount - iterationIndex;
					if (leftEntitiesToIterate <= leftEntitiesInContext)
					{
						iterationsCount = iterationIndex + leftEntitiesToIterate;
						leftEntitiesToIterate = 0;
					}
					else
					{
						iterationsCount = iterationIndex + leftEntitiesInContext;
						leftEntitiesToIterate -= leftEntitiesInContext;
					}

					uint64_t idx = iterationIndex;
					for (; idx < iterationsCount; idx++)
					{
						const auto& entityData = entitiesData[idx];
						if (entityData.IsActive())
						{
							InvokeEntityIteration(func, entityBuffer, *entityData.m_EntityData, idx, containersTuple);
						}
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
				if (!m_IsValid) return;

				Container* container = m_Query->m_Container;
				Entity entityBuffer = {};
				if constexpr (is_invocable_with_entity_v<Callable, ComponentsTypes...>)
				{
					entityBuffer.SetLifeTimeData_Internal(container->GetLifeTimeData());
				}

				uint64_t contextIndex = m_FirstArchetypeIndex;
				uint64_t contextCount = m_Query->m_ArchetypesContexts.size();
				ArchetypeContextType* archetypeContexts = m_Query->m_ArchetypesContexts.data();

				uint64_t leftEntitiesToIterate = m_EntitiesCount;

				for (; contextIndex < contextCount; contextIndex++)
				{
					const ArchetypeContextType& ctx = archetypeContexts[contextIndex];
					uint64_t ctxEntityCount = ctx.GetEntityCount();
					if (ctxEntityCount == 0) continue;

					const auto& containersTuple = ctx.GetContainersTuple();
					const std::vector<ArchetypeEntityData>& entitiesData = ctx.GetArchetype()->m_EntitiesData;

					uint64_t iterationIndex;
					uint64_t iterationsCount;

					if (contextIndex == m_FirstArchetypeIndex)
					{
						iterationIndex = m_FirstIterationIndex;
					}
					else
					{
						iterationIndex = 0;
					}

					uint64_t leftEntitiesInContext = ctxEntityCount - iterationIndex;
					if (leftEntitiesToIterate <= leftEntitiesInContext)
					{
						iterationsCount = iterationIndex + leftEntitiesToIterate;
						leftEntitiesToIterate = 0;
					}
					else
					{
						iterationsCount = iterationIndex + leftEntitiesInContext;
						leftEntitiesToIterate -= leftEntitiesInContext;
					}

					uint64_t idx = iterationIndex;
					for (; idx < iterationsCount; idx++)
					{
						const auto& entityData = entitiesData[idx];
						// Add safety check since the entity can sometimes be null, meaning the record is invalid.
						// When iterating with entity state checks, an explicit validity check is unnecessary—
						// if the entity data is nullptr, the archetype's "is active" flag will already be false.
						if (entityData.m_EntityData != nullptr)
						{
							InvokeEntityIteration(func, entityBuffer, *entityData.m_EntityData, idx, containersTuple);
						}
					}

					if (leftEntitiesToIterate == 0)
					{
						return;
					}
				}
			}

		private:
			QueryType* m_Query = nullptr;
			bool m_IsValid = false;

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

			uint64_t entitiesCount = 0;
			for (ArchetypeContextType& archContext : m_ArchetypesContexts)
			{
				entitiesCount += archContext.GetEntityCount();
			}

			uint64_t realDesiredBatchSize = std::llround(std::ceil((float)entitiesCount / (float)desiredBatchesCount));
			uint64_t finalBatchSize;

			if (realDesiredBatchSize < minBatchSize)
				finalBatchSize = minBatchSize;
			else
				finalBatchSize = realDesiredBatchSize;


			BatchIterator* currentIterator = nullptr;

			uint64_t contextsCount = m_ArchetypesContexts.size();
			for (uint64_t contextIndex = 0; contextIndex < contextsCount; contextIndex++)
			{
				const ArchetypeContextType& ctx = m_ArchetypesContexts[contextIndex];
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