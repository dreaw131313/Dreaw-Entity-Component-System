#pragma once
#include "decs\Core.h"
#include "decs\Type.h"
#include "decs\Entity.h"
#include "decs\Container.h"

#include "IterationCore.h"

namespace decs
{
	template<typename... ComponentsTypes>
	class MultiQuery
	{
	private:
		using ArchetypeContextType = ArchetypeContext<sizeof...(ComponentsTypes)>;
		using ContainerContextType = ContainerContext<ArchetypeContextType, ComponentsTypes...>;

	public:
		MultiQuery()
		{

		}

		MultiQuery(const uint64_t& containerContextsChunkSize) :
			m_ContainerContexts(containerContextsChunkSize)
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
			m_Without.clear();
			findIds<ComponentsTypes...>(m_Without);
			return *this;
		}

		template<typename... ComponentsTypes>
		MultiQuery& WithAnyFrom()
		{
			m_IsDirty = true;
			m_WithAnyOf.clear();
			findIds<ComponentsTypes...>(m_WithAnyOf);
			return *this;
		}

		template<typename... ComponentsTypes>
		MultiQuery& With()
		{
			m_IsDirty = true;
			m_WithAll.clear();
			findIds<ComponentsTypes...>(m_WithAll);
			return *this;
		}

		template<typename Callable>
		inline void ForEach(Callable&& func) noexcept
		{
			ForEachBackward(func);
		}

		template<typename Callable>
		void ForEachForward(Callable&& func) noexcept
		{
			Fetch();

			uint64_t containerContextChunksCount = m_ContainerContexts.ChunksCount();
			decs::Entity entityBuffor = {};
			std::tuple<PackedContainer<ComponentsTypes>*...> containersTuple = {};

			for (uint64_t chunkIdx = 0; chunkIdx < containerContextChunksCount; chunkIdx++)
			{
				auto chunk = m_ContainerContexts.GetChunk(chunkIdx);
				const uint64_t chunkSize = m_ContainerContexts.GetChunkSize(chunkIdx);

				for (uint64_t elementIndex = 0; elementIndex < chunkSize; elementIndex++)
				{
					ContainerContextType& containerContext = chunk[elementIndex];
					auto archetypesContexts = containerContext.m_ArchetypesContexts.data();
					const uint64_t archetypesContextsCount = containerContext.m_ArchetypesContexts.size();

					for (uint64_t archetypeContextIdx = 0; archetypeContextIdx < archetypesContextsCount; archetypeContextIdx++)
					{
						ArchetypeContextType& ctx = archetypesContexts[archetypeContextIdx];
						if (ctx.m_EntitiesCount == 0) continue;

						std::vector<ArchetypeEntityData>& entitiesData = ctx.Arch->m_EntitiesData;
						CreatePackedContainersTuple<ComponentsTypes...>(containersTuple, ctx);

						for (uint64_t idx = 0; idx < ctx.m_EntitiesCount; idx++)
						{
							const auto& entityData = entitiesData[idx];
							if (entityData.IsActive())
							{
								if constexpr (std::is_invocable<Callable, Entity&, typename component_type<ComponentsTypes>::Type&...>())
								{
									entityBuffor.Set(entityData.m_ID, containerContext.m_Container);
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
			}
		}

		template<typename Callable>
		void ForEachBackward(Callable&& func) noexcept
		{
			Fetch();

			uint64_t containerContextChunksCount = m_ContainerContexts.ChunksCount();
			decs::Entity entityBuffor = {};
			std::tuple<PackedContainer<ComponentsTypes>*...> containersTuple = {};

			for (uint64_t chunkIdx = 0; chunkIdx < containerContextChunksCount; chunkIdx++)
			{
				auto chunk = m_ContainerContexts.GetChunk(chunkIdx);
				const uint64_t chunkSize = m_ContainerContexts.GetChunkSize(chunkIdx);

				for (uint64_t elementIndex = 0; elementIndex < chunkSize; elementIndex++)
				{
					ContainerContextType& containerContext = chunk[elementIndex];
					auto archetypesContexts = containerContext.m_ArchetypesContexts.data();
					const uint64_t archetypesContextsCount = containerContext.m_ArchetypesContexts.size();

					for (uint64_t archetypeContextIdx = 0; archetypeContextIdx < archetypesContextsCount; archetypeContextIdx++)
					{
						ArchetypeContextType& ctx = archetypesContexts[archetypeContextIdx];
						if (ctx.m_EntitiesCount == 0) continue;

						std::vector<ArchetypeEntityData>& entitiesData = ctx.Arch->m_EntitiesData;
						CreatePackedContainersTuple<ComponentsTypes...>(containersTuple, ctx);

						for (int64_t idx = (int64_t)ctx.m_EntitiesCount - 1; idx > -1; idx--)
						{
							const auto& entityData = entitiesData[idx];
							if (entityData.IsActive())
							{
								if constexpr (std::is_invocable<Callable, Entity&, typename component_type<ComponentsTypes>::Type&...>())
								{
									entityBuffor.Set(entityData.m_ID, containerContext.m_Container);
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
			}
		}

		bool AddContainer(Container* container)
		{
			auto& contextIndex = m_ContainerContextsIndexes[container];
			if (contextIndex >= m_ContainerContexts.Size() || m_ContainerContexts[contextIndex].m_Container != container)
			{
				contextIndex = m_ContainerContexts.Size();
				m_ContainerContexts.EmplaceBack(container);
				return true;
			}
			return false;
		}

		bool RemoveContainer(Container* container)
		{
			auto it = m_ContainerContextsIndexes.find(container);
			if (it != m_ContainerContextsIndexes.end())
			{
				const uint64_t index = it->second;
				m_ContainerContexts.RemoveSwapBack(index);

				if (index < m_ContainerContexts.Size())
				{
					ContainerContextType& context = m_ContainerContexts[index];
					m_ContainerContextsIndexes[context.m_Container] = index;
				}
				m_ContainerContextsIndexes.erase(it);

				return true;
			}
			return false;
		}

	private:
		TypeGroup<ComponentsTypes...> m_Includes = {};
		std::vector<TypeID> m_Without;
		std::vector<TypeID> m_WithAnyOf;
		std::vector<TypeID> m_WithAll;

		bool m_IsDirty = true;

		ChunkedVector<ContainerContextType> m_ContainerContexts = {};
		ecsMap<Container*, uint64_t> m_ContainerContextsIndexes;

		uint64_t m_EntitiesCount = 0;
	private:
		void Fetch()
		{
			uint64_t containerContextsSize = m_ContainerContexts.Size();
			uint64_t minComponentsCount = GetMinComponentsCount();
			m_EntitiesCount = 0;
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
					m_EntitiesCount += containerContext.m_EntitiesCount;
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
					m_EntitiesCount += containerContext.m_EntitiesCount;
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
			std::get<PackedContainer<T>*>(containersTuple) = (reinterpret_cast<PackedContainer<T>*>(context.m_Containers[compIdx]));

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
				const uint64_t& startContainerChunkIndex,
				const uint64_t& startContainerElementIndex,
				const uint64_t& startArchetypeIndex,
				const uint64_t& startEntityIndex,
				const uint64_t& entitiesCount
			) :
				m_Query(query),
				m_StartContainerChunkInex(startContainerChunkIndex),
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
				uint64_t containerContextChunksCount = containerContexts.ChunksCount();
				decs::Entity entityBuffor = {};
				std::tuple<PackedContainer<ComponentsTypes>*...> containersTuple = {};

				uint64_t iteratedEntities = 0;

				for (uint64_t chunkIdx = m_StartContainerChunkInex; chunkIdx < containerContextChunksCount; chunkIdx++)
				{
					auto chunk = containerContexts.GetChunk(chunkIdx);
					const uint64_t chunkSize = containerContexts.GetChunkSize(chunkIdx);

					for (uint64_t elementIndex = m_StartContainerElementIndex; elementIndex < chunkSize; elementIndex++)
					{
						ContainerContextType& containerContext = chunk[elementIndex];
						auto archetypesContexts = containerContext.m_ArchetypesContexts.data();
						const uint64_t archetypesContextsCount = containerContext.m_ArchetypesContexts.size();

						uint64_t archetypeContextIdx;
						bool isStartArchetypeContext;
						if (chunkIdx == m_StartContainerChunkInex && elementIndex == m_StartContainerElementIndex)
						{
							archetypeContextIdx = m_StartArchetypeIndex;
							isStartArchetypeContext = true;
						}
						else
						{
							archetypeContextIdx = 0;
							isStartArchetypeContext = false;
						}

						for (; archetypeContextIdx < archetypesContextsCount; archetypeContextIdx++)
						{
							ArchetypeContextType& ctx = archetypesContexts[archetypeContextIdx];
							if (ctx.m_EntitiesCount == 0) continue;

							std::vector<ArchetypeEntityData>& entitiesData = ctx.Arch->m_EntitiesData;
							CreatePackedContainersTuple<ComponentsTypes...>(containersTuple, ctx);

							for (uint64_t idx = isStartArchetypeContext ? m_StartEntityIndex : 0; idx < ctx.m_EntitiesCount; idx++)
							{
								const auto& entityData = entitiesData[idx];
								if (entityData.IsActive())
								{
									if constexpr (std::is_invocable<Callable, Entity&, typename component_type<ComponentsTypes>::Type&...>())
									{
										entityBuffor.Set(entityData.m_ID, containerContext.m_Container);
										func(entityBuffor, std::get<PackedContainer<ComponentsTypes>*>(containersTuple)->GetAsRef(idx)...);
									}
									else
									{
										func(std::get<PackedContainer<ComponentsTypes>*>(containersTuple)->GetAsRef(idx)...);
									}
								}
								iteratedEntities += 1;
								if (iteratedEntities >= m_EntitiesCount)
								{
									return;
								}
							}
						}
					}
				}
			}

		private:
			QueryType* m_Query = nullptr;
			uint64_t m_StartContainerChunkInex = 0;
			uint64_t m_StartContainerElementIndex = 0;
			uint64_t m_StartArchetypeIndex = 0;
			uint64_t m_StartEntityIndex = 0;
			uint64_t m_EntitiesCount = 0;


		private:
			template<typename T = void, typename... Args>
			void CreatePackedContainersTuple(
				std::tuple<PackedContainer<ComponentsTypes>*...>& containersTuple,
				const ArchetypeContextType& context
			) const noexcept
			{
				constexpr uint64_t compIdx = sizeof...(ComponentsTypes) - sizeof...(Args) - 1;
				std::get<PackedContainer<T>*>(containersTuple) = (reinterpret_cast<PackedContainer<T>*>(context.m_Containers[compIdx]));

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
		};

	public:
		void CreateBatchIterators(
			std::vector<BatchIterator>& iterators,
			const uint64_t& desiredBatchesCount,
			const uint64_t& minBatchSize
		)
		{
			Fetch();
			uint64_t entitiesCount = m_EntitiesCount;

			uint64_t realDesiredBatchSize = std::llround(std::ceil((float)entitiesCount / (float)desiredBatchesCount));
			uint64_t finalBatchSize;
			if (realDesiredBatchSize < minBatchSize)
				finalBatchSize = minBatchSize;
			else
				finalBatchSize = realDesiredBatchSize;


			BatchIterator* iterator = nullptr;

			uint64_t containerContextChunksCount = this->m_ContainerContexts.ChunksCount();
			for (uint64_t chunkIdx = 0; chunkIdx < containerContextChunksCount; chunkIdx++)
			{
				auto chunk = m_ContainerContexts.GetChunk(chunkIdx);
				const uint64_t chunkSize = m_ContainerContexts.GetChunkSize(chunkIdx);

				for (uint64_t elementIndex = 0; elementIndex < chunkSize; elementIndex++)
				{
					ContainerContextType& containerContext = chunk[elementIndex];
					auto archetypesContexts = containerContext.m_ArchetypesContexts.data();
					const uint64_t archetypesContextsCount = containerContext.m_ArchetypesContexts.size();

					for (uint64_t archetypeContextIdx = 0; archetypeContextIdx < archetypesContextsCount; archetypeContextIdx++)
					{
						ArchetypeContextType& ctx = archetypesContexts[archetypeContextIdx];
						if (ctx.m_EntitiesCount == 0) continue;

						uint64_t ctxEntitiesCount = ctx.m_EntitiesCount;
						uint64_t currentEntityIndex = 0;

						while (ctxEntitiesCount > 0)
						{
							if (iterator == nullptr)
							{
								iterator = &iterators.emplace_back(this, chunkIdx, elementIndex, archetypeContextIdx, currentEntityIndex, 0);
							}

							uint64_t neededEntitiesCount = finalBatchSize - iterator->m_EntitiesCount;

							if (neededEntitiesCount > 0)
							{
								uint64_t entitiesCountToAddToIterator;
								if (neededEntitiesCount <= ctxEntitiesCount)
								{
									entitiesCountToAddToIterator = neededEntitiesCount;
								}
								else
								{
									entitiesCountToAddToIterator = ctxEntitiesCount;
								}
								ctxEntitiesCount -= entitiesCountToAddToIterator;
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
		}
	};
}