#pragma once
#include "decs\Core.h"
#include "decs\Type.h"
#include "decs\Entity.h"
#include "decs\Container.h"

#include "IterationCore.h"

namespace decs
{
	class MultiQueryBase
	{
	public:
		virtual ~MultiQueryBase()
		{

		}

		virtual bool AddContainer(Container* container, bool bIsEnabled = true) = 0;
		virtual bool RemoveContainer(Container* container) = 0;
		virtual void SetContainerEnabled(Container* container, bool isEnabled) = 0;
	};

	template<typename... ComponentsTypes>
	class MultiQuery : public MultiQueryBase
	{
	private:
		using ArchetypeContextType = IterationArchetypeContext<sizeof...(ComponentsTypes)>;
		using ContainerContextType = IterationContainerContext<ArchetypeContextType, ComponentsTypes...>;

		template<typename TComponent>
		using PackedContainerType = StablePackedContainer<TComponent>*;

	public:
		MultiQuery()
		{

		}

		inline uint64_t GetMinComponentsCount() const
		{
			uint64_t includesCount = sizeof...(ComponentsTypes);
			if (m_WithAnyOf.size() > 0) includesCount += 1;
			return sizeof...(ComponentsTypes) + m_WithAll.size();
		}

		template<typename... ComponentsTypes>
		MultiQuery& Without()
		{
			m_IsDirty = true;
			if constexpr (sizeof...(ComponentsTypes) == 0)
			{
				m_Without.clear();
			}
			else
			{
				m_Without.resize(sizeof...(ComponentsTypes));
				find_type_ids<ComponentsTypes...>(m_Without.data());
			}
			return *this;
		}

		template<typename... ComponentsTypes>
		MultiQuery& WithAnyFrom()
		{
			m_IsDirty = true;
			if constexpr (sizeof...(ComponentsTypes) == 0)
			{
				m_WithAnyOf.clear();
			}
			else
			{
				m_WithAnyOf.resize(sizeof...(ComponentsTypes));
				find_type_ids<ComponentsTypes...>(m_WithAnyOf.data());
			}
			return *this;
		}

		template<typename... ComponentsTypes>
		MultiQuery& With()
		{
			m_IsDirty = true;
			if constexpr (sizeof...(ComponentsTypes) == 0)
			{
				m_WithAll.clear();
			}
			else
			{
				m_WithAll.resize(sizeof...(ComponentsTypes));
				find_type_ids<ComponentsTypes...>(m_WithAll.data());
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
		inline void ForEach(Callable&& func) noexcept
		{
			Fetch();

			decs::Entity entityBuffor = {};
			std::tuple<PackedContainerType<ComponentsTypes>...> containersTuple = {};

			uint64_t contextSize = m_ContainerContexts.size();
			for (uint64_t containerContextIndex = 0; containerContextIndex < contextSize; containerContextIndex++)
			{
				ContainerContextType& containerContext = m_ContainerContexts[containerContextIndex];
				if (!containerContext.m_bIsEnabled)
				{
					continue; // Skip if container context is disabled
				}

				auto archetypesContexts = containerContext.m_ArchetypesContexts.data();
				const uint64_t archetypesContextsCount = containerContext.m_ArchetypesContexts.size();

				for (uint64_t archetypeContextIdx = 0; archetypeContextIdx < archetypesContextsCount; archetypeContextIdx++)
				{
					ArchetypeContextType& ctx = archetypesContexts[archetypeContextIdx];
					uint64_t ctxEntityCount = ctx.GetEntityCount();
					if (ctxEntityCount == 0) continue;

					std::vector<ArchetypeEntityData>& entitiesData = ctx.Arch->m_EntitiesData;
					CreatePackedContainersTuple<ComponentsTypes...>(containersTuple, ctx);

					for (uint64_t idx = 0; idx < ctxEntityCount; idx++)
					{
						const auto& entityData = entitiesData[idx];
						if (entityData.IsActive())
						{
							if constexpr (std::is_invocable<Callable, Entity&, ComponentsTypes&...>())
							{
								entityBuffor.Set(entityData.m_EntityData, containerContext.m_Container);
								func(
									entityBuffor,
									std::get<PackedContainerType<ComponentsTypes>>(containersTuple)->GetAsRef(idx)...
								);
							}
							else
							{
								func(std::get<PackedContainerType<ComponentsTypes>>(containersTuple)->GetAsRef(idx)...);
							}
						}
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
		void ForEachBackward(Callable&& func) noexcept
		{
			Fetch();

			decs::Entity entityBuffor = {};
			std::tuple<PackedContainerType<ComponentsTypes>...> containersTuple = {};

			uint64_t contextSize = m_ContainerContexts.size();
			for (uint64_t containerContextIndex = 0; containerContextIndex < contextSize; containerContextIndex++)
			{
				ContainerContextType& containerContext = m_ContainerContexts[containerContextIndex];
				if (!containerContext.m_bIsEnabled)
				{
					continue; // Skip if container context is disabled
				}

				auto archetypesContexts = containerContext.m_ArchetypesContexts.data();
				const uint64_t archetypesContextsCount = containerContext.m_ArchetypesContexts.size();

				for (uint64_t archetypeContextIdx = 0; archetypeContextIdx < archetypesContextsCount; archetypeContextIdx++)
				{
					ArchetypeContextType& ctx = archetypesContexts[archetypeContextIdx];
					uint64_t ctxEntityCount = ctx.GetEntityCount();
					if (ctxEntityCount == 0) continue;

					std::vector<ArchetypeEntityData>& entitiesData = ctx.Arch->m_EntitiesData;
					CreatePackedContainersTuple<ComponentsTypes...>(containersTuple, ctx);

					for (int64_t idx = (int64_t)ctxEntityCount - 1; idx > -1; idx--)
					{
						const auto& entityData = entitiesData[idx];
						if (entityData.IsActive())
						{
							if constexpr (std::is_invocable<Callable, Entity&, ComponentsTypes&...>())
							{
								entityBuffor.Set(entityData.m_EntityData, containerContext.m_Container);
								func(
									entityBuffor,
									std::get<PackedContainerType<ComponentsTypes>>(containersTuple)->GetAsRef(idx)...
								);
							}
							else
							{
								func(std::get<PackedContainerType<ComponentsTypes>>(containersTuple)->GetAsRef(idx)...);
							}
						}
					}
				}
			}
		}

		/// <summary>
		/// Iterates over entities in archetypes from last to first. 
		/// During iteration with this method:
		/// Is forbiden to:
		///		- Destroying entities different than currently iterated entity. It can cause index out of bound
		///		- Adding or removing components in entities different than currently iterated entity. It can cause index out of bound
		/// It is safe to:
		///		- Destroy currently iterated entity
		///		- Add or remove components in curently iterated entity
		///		- Create new entities
		/// </summary>
		/// <typeparam name="Callable"></typeparam>
		/// <param name="func"></param>
		template<typename Callable>
		void ForEachSafe(Callable&& func) noexcept
		{
			Fetch();
			CollectArchetypesEntityCount();

			decs::Entity entityBuffor = {};
			std::tuple<PackedContainerType<ComponentsTypes>...> containersTuple = {};

			uint64_t contextSize = m_ContainerContexts.size();
			for (uint64_t containerContextIndex = 0; containerContextIndex < contextSize; containerContextIndex++)
			{
				ContainerContextType& containerContext = m_ContainerContexts[containerContextIndex];
				if (!containerContext.m_bIsEnabled)
				{
					continue; // Skip if container context is disabled
				}

				auto archetypesContexts = containerContext.m_ArchetypesContexts.data();
				const uint64_t archetypesContextsCount = containerContext.m_ArchetypesContexts.size();

				for (uint64_t archetypeContextIdx = 0; archetypeContextIdx < archetypesContextsCount; archetypeContextIdx++)
				{
					ArchetypeContextType& ctx = archetypesContexts[archetypeContextIdx];
					uint64_t ctxEntityCount = ctx.GetCachedEntityCount();
					if (ctxEntityCount == 0) continue;

					std::vector<ArchetypeEntityData>& entitiesData = ctx.Arch->m_EntitiesData;
					CreatePackedContainersTuple<ComponentsTypes...>(containersTuple, ctx);

					for (int64_t idx = (int64_t)ctxEntityCount - 1; idx > -1; idx--)
					{
						const auto& entityData = entitiesData[idx];
						if (entityData.IsActive())
						{
							if constexpr (std::is_invocable<Callable, Entity&, ComponentsTypes&...>())
							{
								entityBuffor.Set(entityData.m_EntityData, containerContext.m_Container);
								func(
									entityBuffor,
									std::get<PackedContainerType<ComponentsTypes>>(containersTuple)->GetAsRef(idx)...
								);
							}
							else
							{
								func(std::get<PackedContainerType<ComponentsTypes>>(containersTuple)->GetAsRef(idx)...);
							}
						}
					}
				}
			}
		}

		virtual bool AddContainer(Container* container, bool bIsEnabled = true) override
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

		virtual bool RemoveContainer(Container* container) override
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

		virtual void SetContainerEnabled(Container* container, bool isEnabled) override
		{
			auto it = m_ContainerContextsIndexes.find(container);
			if (it != m_ContainerContextsIndexes.end())
			{
				ContainerContextType& context = m_ContainerContexts[it->second];
				context.m_bIsEnabled = isEnabled;
			}
		}

		bool Contain(const decs::Entity& entity)
		{
			if (entity.IsValid())
			{
				auto containerCtxIdxIt = m_ContainerContextsIndexes.find(entity.GetContainer());
				if (containerCtxIdxIt != m_ContainerContextsIndexes.end())
				{
					auto& ctx = m_ContainerContexts[containerCtxIdxIt->second];
					ctx.Fetch(
						m_Includes,
						m_Without,
						m_WithAnyOf,
						m_WithAll,
						GetMinComponentsCount()
					);
					return ctx.Contain(entity);
				}
			}
			return false;
		}

		void Fetch()
		{
			uint64_t containerContextsSize = m_ContainerContexts.size();
			uint64_t minComponentsCount = GetMinComponentsCount();
			if (m_IsDirty)
			{
				m_IsDirty = false;
				for (uint64_t i = 0; i < containerContextsSize; i++)
				{
					ContainerContextType& containerContext = m_ContainerContexts[i];
					containerContext.Invalidate();
					containerContext.Fetch(
						m_Includes,
						m_Without,
						m_WithAnyOf,
						m_WithAll,
						minComponentsCount
					);
				}
			}
			else
			{
				for (uint64_t i = 0; i < containerContextsSize; i++)
				{
					ContainerContextType& containerContext = m_ContainerContexts[i];
					m_ContainerContexts[i].Fetch(
						m_Includes,
						m_Without,
						m_WithAnyOf,
						m_WithAll,
						minComponentsCount
					);
				}
			}
		}
	private:
		ecsMap<Container*, uint64_t> m_ContainerContextsIndexes;
		TypeGroup<ComponentsTypes...> m_Includes = {};
		std::vector<TypeID> m_Without;
		std::vector<TypeID> m_WithAnyOf;
		std::vector<TypeID> m_WithAll;

		std::vector<ContainerContextType> m_ContainerContexts = {};

		bool m_IsDirty = true;

	private:

		void CollectArchetypesEntityCount()
		{
			const uint64_t containerCtxCount = m_ContainerContexts.size();
			for (uint64_t containerCtxIdx = 0; containerCtxIdx < containerCtxCount; containerCtxIdx++)
			{
				m_ContainerContexts[containerCtxIdx].ValidateCachedEntityCount();
			}
		}

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

		template<typename T = void, typename... Args>
		void CreatePackedContainersTuple(
			std::tuple<PackedContainerType<ComponentsTypes>...>& containersTuple,
			const ArchetypeContextType& context
		) const noexcept
		{
			constexpr uint64_t compIdx = sizeof...(ComponentsTypes) - sizeof...(Args) - 1;
			std::get<PackedContainerType<T>>(containersTuple) = (static_cast<PackedContainerType<T>>(context.m_Containers[compIdx]));

			if constexpr (sizeof...(Args) == 0) return;

			CreatePackedContainersTuple<Args...>(
				containersTuple,
				context
			);
		}

		template<>
		void CreatePackedContainersTuple<void>(
			std::tuple<PackedContainerType<ComponentsTypes>...>& containersTuple,
			const ArchetypeContextType& context
		) const noexcept
		{

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
			) :
				m_Query(query),
				m_StartContainerElementIndex(startContainerElementIndex),
				m_StartArchetypeIndex(startArchetypeIndex),
				m_StartEntityIndex(startEntityIndex),
				m_EntitiesCount(entitiesCount)
			{

			}

			template<typename Callable>
			void ForEach(Callable&& func) noexcept
			{
				auto& containerContexts = m_Query->m_ContainerContexts;
				decs::Entity entityBuffor = {};
				std::tuple<PackedContainerType<ComponentsTypes>...> containersTuple = {};

				uint64_t leftEntitiesToIterate = m_EntitiesCount;

				uint64_t contextSize = containerContexts.size();
				for (uint64_t containerContextIndex = 0; containerContextIndex < contextSize; containerContextIndex++)
				{
					ContainerContextType& containerContext = containerContexts[containerContextIndex];
					if (!containerContext.m_bIsEnabled)
					{
						continue; // Skip if container context is disabled
					}

					auto archetypesContexts = containerContext.m_ArchetypesContexts.data();
					const uint64_t archetypesContextsCount = containerContext.m_ArchetypesContexts.size();

					uint64_t archetypeContextIdx;
					uint64_t startEntitiyIndex;
					if (containerContextIndex == m_StartContainerElementIndex)
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
						ArchetypeContextType& ctx = archetypesContexts[archetypeContextIdx];
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

						std::vector<ArchetypeEntityData>& entitiesData = ctx.Arch->m_EntitiesData;
						CreatePackedContainersTuple<ComponentsTypes...>(containersTuple, ctx);

						for (uint64_t idx = startEntitiyIndex; idx < entitiesCount; idx++)
						{
							const auto& entityData = entitiesData[idx];
							if (entityData.IsActive())
							{
								if constexpr (std::is_invocable<Callable, Entity&, ComponentsTypes&...>())
								{
									entityBuffor.Set(entityData.m_EntityData, containerContext.m_Container);
									func(
										entityBuffor,
										std::get<PackedContainerType<ComponentsTypes>>(containersTuple)->GetAsRef(idx)...
									);
								}
								else
								{
									func(std::get<PackedContainerType<ComponentsTypes>>(containersTuple)->GetAsRef(idx)...);
								}
							}
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
			uint64_t m_StartContainerElementIndex = 0;
			uint64_t m_StartArchetypeIndex = 0;
			uint64_t m_StartEntityIndex = 0;
			uint64_t m_EntitiesCount = 0;

		private:
			template<typename T = void, typename... Args>
			void CreatePackedContainersTuple(
				std::tuple<PackedContainerType<ComponentsTypes>...>& containersTuple,
				const ArchetypeContextType& context
			) const noexcept
			{
				constexpr uint64_t compIdx = sizeof...(ComponentsTypes) - sizeof...(Args) - 1;
				std::get<PackedContainerType<T>>(containersTuple) = (static_cast<PackedContainerType<T>>(context.m_Containers[compIdx]));

				if constexpr (sizeof...(Args) == 0) return;
				CreatePackedContainersTuple<Args...>(
					containersTuple,
					context
				);
			}

			template<>
			void CreatePackedContainersTuple<void>(
				std::tuple<PackedContainerType<ComponentsTypes>...>& containersTuple,
				const ArchetypeContextType& context
			) const noexcept
			{

			}
		};

	public:
		void CreateBatchIterators(
			std::vector<BatchIterator>& iterators,
			uint64_t desiredBatchesCount,
			uint64_t minBatchSize
		)
		{
			Fetch();
			uint64_t entitiesCount = CalculateEntityCount();

			uint64_t realDesiredBatchSize = std::llround(std::ceil((float)entitiesCount / (float)desiredBatchesCount));
			uint64_t finalBatchSize;
			if (realDesiredBatchSize < minBatchSize)
				finalBatchSize = minBatchSize;
			else
				finalBatchSize = realDesiredBatchSize;


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
	};
}