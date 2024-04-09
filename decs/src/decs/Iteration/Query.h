#pragma once
#include "decs\Core.h"
#include "decs\Type.h"
#include "decs\Entity.h"
#include "decs\Container.h"

#include "IterationCore.h"

namespace decs
{
	template<typename... ComponentsTypes>
	class Query
	{
	private:
		using ArchetypeContextType = IterationArchetypeContext<sizeof...(ComponentsTypes)>;
	public:
		Query()
		{

		}

		Query(Container& container) :
			m_Container(&container)
		{

		}

		~Query()
		{

		}

		inline void SetContainer(Container& container)
		{
			m_IsDirty = &container != m_Container;
			m_Container = &container;
		}

		inline Container* GetContainer() const { return m_Container; }

		inline bool IsValid()const { return m_Container != nullptr; }

		template<typename... ComponentsTypes>
		Query& Without()
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
		Query& WithAnyFrom()
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
		Query& With()
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
			if (!IsValid()) return;
			Fetch();

			Entity entityBuffor = {};
			std::tuple<PackedContainer<ComponentsTypes>*...> containersTuple = {};
			const uint64_t contextCount = m_ArchetypesContexts.size();
			for (uint64_t contextIndex = 0; contextIndex < contextCount; contextIndex++)
			{
				const ArchetypeContextType& ctx = m_ArchetypesContexts[contextIndex];
				uint64_t ctxEntityCount = ctx.GetEntityCount();
				if (ctxEntityCount == 0) continue;

				std::vector<ArchetypeEntityData>& entitiesData = ctx.Arch->m_EntitiesData;
				CreatePackedContainersTuple<ComponentsTypes...>(containersTuple, ctx);

				for (uint64_t idx = 0; idx < ctxEntityCount; idx++)
				{
					const auto& entityData = entitiesData[idx];
					if (entityData.IsActive())
					{
						if constexpr (std::is_invocable<Callable, Entity&, typename component_type<ComponentsTypes>::Type&...>())
						{
							entityBuffor.Set(entityData.m_EntityData, this->m_Container);
							func(entityBuffor, std::get<PackedContainer<ComponentsTypes>*>(containersTuple)->GetAsRef(idx)...);
						}
						else
						{
							func(std::get<PackedContainer<ComponentsTypes>*>(containersTuple)->GetAsRef(idx)...);
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
			if (!IsValid()) return;
			Fetch();

			Entity entityBuffor = {};
			std::tuple<PackedContainer<ComponentsTypes>*...> containersTuple = {};
			const uint64_t contextCount = m_ArchetypesContexts.size();
			for (uint64_t contextIndex = 0; contextIndex < contextCount; contextIndex++)
			{
				const ArchetypeContextType& ctx = m_ArchetypesContexts[contextIndex];
				uint64_t ctxEntityCount = ctx.GetEntityCount();
				if (ctxEntityCount == 0) continue;

				std::vector<ArchetypeEntityData>& entitiesData = ctx.Arch->m_EntitiesData;
				CreatePackedContainersTuple<ComponentsTypes...>(containersTuple, ctx);
				int64_t idx = ctxEntityCount - 1;

				for (; idx > -1; idx--)
				{
					const auto& entityData = entitiesData[idx];
					if (entityData.IsActive())
					{
						if constexpr (std::is_invocable<Callable, Entity&, typename component_type<ComponentsTypes>::Type&...>())
						{
							entityBuffor.Set(entityData.m_EntityData, this->m_Container);
							func(entityBuffor, std::get<PackedContainer<ComponentsTypes>*>(containersTuple)->GetAsRef(idx)...);
						}
						else
						{
							func(std::get<PackedContainer<ComponentsTypes>*>(containersTuple)->GetAsRef(idx)...);
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
			if (!IsValid()) return;
			Fetch();
			CollectArchetypesEntityCount();

			Entity entityBuffor = {};
			std::tuple<PackedContainer<ComponentsTypes>*...> containersTuple = {};
			const uint64_t contextCount = m_ArchetypesContexts.size();
			for (uint64_t contextIndex = 0; contextIndex < contextCount; contextIndex++)
			{
				const ArchetypeContextType& ctx = m_ArchetypesContexts[contextIndex];
				uint64_t ctxEntityCount = ctx.GetCachedEntityCount();
				if (ctxEntityCount == 0) continue;

				std::vector<ArchetypeEntityData>& entitiesData = ctx.Arch->m_EntitiesData;
				CreatePackedContainersTuple<ComponentsTypes...>(containersTuple, ctx);
				int64_t idx = ctxEntityCount - 1;

				for (; idx > -1; idx--)
				{
					const auto& entityData = entitiesData[idx];
					if (entityData.IsActive())
					{
						if constexpr (std::is_invocable<Callable, Entity&, typename component_type<ComponentsTypes>::Type&...>())
						{
							entityBuffor.Set(entityData.m_EntityData, this->m_Container);
							func(entityBuffor, std::get<PackedContainer<ComponentsTypes>*>(containersTuple)->GetAsRef(idx)...);
						}
						else
						{
							func(std::get<PackedContainer<ComponentsTypes>*>(containersTuple)->GetAsRef(idx)...);
						}
					}
				}
			}
		}

		void Fetch()
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

				ArchetypesMap& map = m_Container->m_ArchetypesMap;
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

		/// <summary>
		/// Checks if entity belong to this query.
		/// </summary>
		/// <param name="entity"></param>
		/// <returns></returns>
		bool Contain(const decs::Entity& entity)
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
		TypeGroup<ComponentsTypes...> m_Includes = {};
		std::vector<TypeID> m_Without;
		std::vector<TypeID> m_WithAnyOf;
		std::vector<TypeID> m_WithAll;
		std::vector<ArchetypeContextType> m_ArchetypesContexts;

		// cache value to check if query should be updated:
		uint64_t m_ArchetypesCountDirty = 0;

		Container* m_Container = nullptr;
		bool m_IsDirty = true;

	private:
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
			if (!ContainArchetype(&archetype) && archetype.ComponentCount())
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
					ArchetypeContextType& context = m_ArchetypesContexts.emplace_back();

					for (uint32_t typeIdx = 0; typeIdx < m_Includes.Size(); typeIdx++)
					{
						auto typeIDIndex = archetype.FindTypeIndex(m_Includes.IDs()[typeIdx]);
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

		void FetchArchetypesFromArchetypesGroup(ArchetypesGroupByOneType* group)
		{
			if (group == nullptr) return;
			uint64_t minComponentsCount = GetMinComponentsCount();
			uint64_t maxComponentCountsInGroup = group->MaxComponentsCount();

			for (uint64_t i = minComponentsCount; i <= maxComponentCountsInGroup; i++)
			{
				std::vector<Archetype*>& archetypesToCheck = group->GetArchetypesWithComponentsCount(i);
				for (auto& archetype : archetypesToCheck)
				{
					TryAddArchetypeFromGroup(*archetype);
				}
			}
		}

		void AddingArchetypesWithCheckingOnlyNewArchetypes(
			ArchetypesMap& map,
			uint64_t startArchetypesIndex,
			uint64_t minRequiredComponentsCount
		)
		{
			auto& archetypes = map.m_Archetypes;
			uint64_t archetypesCount = map.m_Archetypes.Size();
			for (uint64_t i = startArchetypesIndex; i < archetypesCount; i++)
			{
				Archetype& arch = archetypes[i];
				if (arch.ComponentCount() < minRequiredComponentsCount)
				{
					TryAddArchetypeFromGroup(arch);
				}
			}
		}

		template<typename T = void, typename... Args>
		void CreatePackedContainersTuple(
			std::tuple<PackedContainer<ComponentsTypes>*...>& containersTuple,
			const ArchetypeContextType& context
		) const noexcept
		{
			constexpr uint64_t compIdx = sizeof...(ComponentsTypes) - sizeof...(Args) - 1;
			std::get<PackedContainer<T>*>(containersTuple) = (static_cast<PackedContainer<T>*>(context.m_Containers[compIdx]));

			if constexpr (sizeof...(Args) == 0) return;

			CreatePackedContainersTuple<Args...>(
				containersTuple,
				context
			);
		}

		template<>
		void CreatePackedContainersTuple<void>(
			std::tuple<PackedContainer<ComponentsTypes>*...>& containersTuple,
			const ArchetypeContextType& context
		) const noexcept
		{

		}

#pragma region BATCH ITERATOR
	public:
		class BatchIterator
		{
			using QueryType = Query<ComponentsTypes...>;

			template<typename... Types>
			friend class Query;
		public:
			BatchIterator() {}

			BatchIterator(
				QueryType* query,
				const uint64_t& firstArchetypeIndex,
				const uint64_t& firstIterationIndex,
				const uint64_t& entitiesCount
			) :
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
			inline void ForEach(Callable&& func) const
			{
				if (!m_IsValid) return;

				Entity entityBuffor = {};
				std::tuple<PackedContainer<ComponentsTypes>*...> containersTuple = {};

				uint64_t contextIndex = m_FirstArchetypeIndex;
				uint64_t contextCount = m_Query->m_ArchetypesContexts.size();
				ArchetypeContextType* archetypeContexts = m_Query->m_ArchetypesContexts.data();
				Container* container = m_Query->m_Container;

				uint64_t leftEntitiesToIterate = m_EntitiesCount;

				for (; contextIndex < contextCount; contextIndex++)
				{
					const ArchetypeContextType& ctx = archetypeContexts[contextIndex];
					uint64_t ctxEntityCount = ctx.GetEntityCount();
					if (ctxEntityCount == 0) continue;

					std::vector<ArchetypeEntityData>& entitiesData = ctx.Arch->m_EntitiesData;
					CreatePackedContainersTuple<ComponentsTypes...>(containersTuple, ctx);

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
							if constexpr (std::is_invocable<Callable, Entity&, typename component_type<ComponentsTypes>::Type&...>())
							{
								entityBuffor.Set(entityData.m_EntityData, container);
								func(entityBuffor, std::get<PackedContainer<ComponentsTypes>*>(containersTuple)->GetAsRef(idx)...);
							}
							else
							{
								func(std::get<PackedContainer<ComponentsTypes>*>(containersTuple)->GetAsRef(idx)...);
							}
						}
					}

					if (leftEntitiesToIterate == 0)
					{
						return;
					}
				}
			}

		private:
			template<typename T = void, typename... Args>
			void CreatePackedContainersTuple(
				std::tuple<PackedContainer<ComponentsTypes>*...>& containersTuple,
				const ArchetypeContextType& context
			) const noexcept
			{
				constexpr uint64_t compIdx = sizeof...(ComponentsTypes) - sizeof...(Args) - 1;
				std::get<PackedContainer<T>*>(containersTuple) = (static_cast<PackedContainer<T>*>(context.m_Containers[compIdx]));

				if constexpr (sizeof...(Args) == 0) return;

				CreatePackedContainersTuple<Args...>(
					containersTuple,
					context
				);
			}

			template<>
			void CreatePackedContainersTuple<void>(
				std::tuple<PackedContainer<ComponentsTypes>*...>& containersTuple,
				const ArchetypeContextType& context
			) const noexcept
			{

			}

		protected:
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