#pragma once
#include "Core.h"
#include "IterationCore.h"
#include "Type.h"
#include "Entity.h"
#include "Container.h"

namespace decs
{
	template<typename... ComponentsTypes>
	class Query
	{
	private:
		using ArchetypeContextType = ArchetypeContext<sizeof...(ComponentsTypes)>;
		using ContainerContextType = ContainerContext<ArchetypeContextType, ComponentsTypes...>;
	public:

		inline uint64_t GetMinComponentsCount() const
		{
			uint64_t includesCount = sizeof...(ComponentsTypes);
			if (m_WithAnyOf.size() > 0) includesCount += 1;
			return sizeof...(ComponentsTypes) + m_WithAll.size();
		}

		template<typename... ComponentsTypes>
		Query& Without()
		{
			m_IsDirty = true;
			m_Without.clear();
			findIds<ComponentsTypes...>(m_Without);
			return *this;
		}

		template<typename... ComponentsTypes>
		Query& WithAnyFrom()
		{
			m_IsDirty = true;
			m_WithAnyOf.clear();
			findIds<ComponentsTypes...>(m_WithAnyOf);
			return *this;
		}

		template<typename... ComponentsTypes>
		Query& With()
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
								if constexpr (std::is_invocable<Callable, Entity&, typename stable_type<ComponentsTypes>::Type&...>())
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
								if constexpr (std::is_invocable<Callable, Entity&, typename stable_type<ComponentsTypes>::Type&...>())
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

		void Fetch()
		{
			uint64_t containerContextsSize = m_ContainerContexts.Size();
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
		TypeGroup<ComponentsTypes...> m_Includes = {};
		std::vector<TypeID> m_Without;
		std::vector<TypeID> m_WithAnyOf;
		std::vector<TypeID> m_WithAll;

		bool m_IsDirty = true;

		ChunkedVector<ContainerContextType> m_ContainerContexts = {};
		ecsMap<Container*, uint64_t> m_ContainerContextsIndexes;

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
}