#pragma once
#include "Core.h"
#include "Enums.h"
#include "Type.h"
#include "Entity.h"
#include "Container.h"
#include "decs/CustomApplay.h"
#include <algorithm>
#include "Containers\ChunkedVector.h"

namespace decs
{
	template<typename T>
	struct ChunksIterator
	{
	public:
		ChunkedVector<T>* m_ChunkedVector = nullptr;
		int32_t m_ChunksCount = 0;
		int32_t m_ChunkIndex = 0;
		int32_t m_ChunkSize = 0;
		int32_t m_Index = 0;
		T* m_DataPtr = nullptr;

		ChunksIterator()
		{

		}

		ChunksIterator(ChunkedVector<T>* chunkedVector, bool isBackward = false) :
			m_ChunkedVector(chunkedVector),
			m_ChunksCount(m_ChunkedVector->ChunksCount()),
			m_ChunkIndex(m_ChunksCount - 1),
			m_ChunkSize(chunkedVector->GetChunkSize(m_ChunkIndex)),
			m_Index(m_ChunkSize - 1),
			m_DataPtr(chunkedVector->GetChunk(m_ChunkIndex))
		{

		}

		void Set(ChunkedVector<T>* chunkedVector, bool isBackward = false)
		{
			if (isBackward)
			{
				m_ChunkedVector = chunkedVector;
				m_ChunksCount = m_ChunkedVector->ChunksCount();
				m_ChunkIndex = m_ChunksCount - 1;
				m_ChunkSize = chunkedVector->GetChunkSize(m_ChunkIndex);
				m_Index = m_ChunkSize - 1;
				m_DataPtr = chunkedVector->GetChunk(m_ChunkIndex) + m_Index;
			}
			else
			{
				m_ChunkedVector = chunkedVector;
				m_ChunksCount = m_ChunkedVector->ChunksCount();
				m_ChunkIndex = 0;
				m_ChunkSize = chunkedVector->GetChunkSize(m_ChunkIndex);
				m_Index = 0;
				m_DataPtr = chunkedVector->GetChunk(m_ChunkIndex);
			}
		}

		inline void Increment()
		{
			m_Index += 1;
			if (m_Index > m_ChunkSize)
			{
				m_ChunkIndex += 1;
				m_Index = 0;
				if (m_ChunkIndex < m_ChunksCount)
				{
					m_DataPtr = m_ChunkedVector->GetChunk(m_ChunkIndex);
					m_ChunkSize = m_ChunkedVector->GetChunkSize(m_ChunkIndex);
				}
			}
			else
			{
				m_DataPtr += 1;
			}
		}

		inline void Decrement()
		{
			m_Index -= 1;
			if (m_Index < 0)
			{
				m_ChunkIndex -= 1;
				if (m_ChunkIndex > -1)
				{
					m_DataPtr = m_ChunkedVector->GetChunk(m_ChunkIndex);
					m_ChunkSize = m_ChunkedVector->GetChunkSize(m_ChunkIndex);
					m_Index = m_ChunkSize - 1;
				}
			}
			else
			{
				m_DataPtr -= 1;
			}
		}
	};

	template<typename... ComponentsTypes>
	class View
	{
	private:
		struct ArchetypeContext
		{
		public:
			Archetype* Arch = nullptr;
			uint64_t m_EntitiesCount = 0;
			PackedContainerBase* m_Containers[sizeof...(ComponentsTypes)];
			bool m_SameBucketsSizeInContainers = true;

		public:
			inline void ValidateEntitiesCount() { m_EntitiesCount = Arch->EntitiesCount(); }
		};

	public:
		View()
		{

		}

		View(Container& container) :
			m_Container(&container)
		{

		}

		~View()
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
		View& Without()
		{
			m_IsDirty = true;
			m_Without.clear();
			findIds<ComponentsTypes...>(m_Without);
			return *this;
		}

		template<typename... ComponentsTypes>
		View& WithAnyFrom()
		{
			m_IsDirty = true;
			m_WithAnyOf.clear();
			findIds<ComponentsTypes...>(m_WithAnyOf);
			return *this;
		}

		template<typename... ComponentsTypes>
		View& With()
		{
			m_IsDirty = true;
			m_WithAll.clear();
			findIds<ComponentsTypes...>(m_WithAll);
			return *this;
		}

		void Fetch()
		{
			if (m_IsDirty)
			{
				m_IsDirty = false;
				Invalidate();
			}

			uint64_t containerArchetypesCount = m_Container->m_ArchetypesMap.m_Archetypes.Size();
			if (m_ArchetypesCount_Dirty != containerArchetypesCount)
			{
				uint64_t newArchetypesCount = containerArchetypesCount - m_ArchetypesCount_Dirty;
				uint64_t minComponentsCountInArchetype = GetMinComponentsCount();

				ArchetypesMap& map = m_Container->m_ArchetypesMap;
				uint64_t maxComponentsInArchetype = map.m_ArchetypesGroupedByComponentsCount.size();
				if (maxComponentsInArchetype < minComponentsCountInArchetype) return;

				if (newArchetypesCount > m_ArchetypesContexts.size())
				{
					// performing normal finding of archetypes
					NormalArchetypesFetching(map, maxComponentsInArchetype, minComponentsCountInArchetype);
				}
				else
				{
					// checking only new archetypes:
					AddingArchetypesWithCheckingOnlyNewArchetypes(map, m_ArchetypesCount_Dirty, minComponentsCountInArchetype);
				}

				m_ArchetypesCount_Dirty = containerArchetypesCount;
			}
		}

		template<typename Callable>
		void ForEach(Callable&& func) noexcept
		{
			if (!IsValid()) return;
			ValidateView();

			if constexpr (std::is_invocable<Callable, Entity&, ComponentsTypes&...>())
			{
				ForEachWithEntityBackward(func);
			}
			else
			{
				ForEachBackward(func);
			}
		}

		template<typename Callable>
		void ForEach(Callable&& func, const IterationType& iterationType) noexcept
		{
			if (!IsValid()) return;
			ValidateView();

			if constexpr (std::is_invocable<Callable, Entity&, ComponentsTypes&...>())
			{
				switch (iterationType)
				{
					case IterationType::Forward:
					{
						ForEachWithEntityForward(func);
						break;
					}
					case IterationType::Backward:
					{
						ForEachWithEntityBackward(func);
						break;
					}
				}
			}
			else
			{
				switch (iterationType)
				{
					case IterationType::Forward:
					{
						ForEachForward(func);
						break;
					}
					case IterationType::Backward:
					{
						ForEachBackward(func);
						break;
					}
				}
			}
		}

	private:
		TypeGroup<ComponentsTypes...> m_Includes = {};
		std::vector<TypeID> m_Without;
		std::vector<TypeID> m_WithAnyOf;
		std::vector<TypeID> m_WithAll;

		Container* m_Container = nullptr;
		bool m_IsDirty = true;

		std::vector<ArchetypeContext> m_ArchetypesContexts;
		ecsSet<Archetype*> m_ContainedArchetypes;

		// cache value to check if view should be updated:
		uint64_t m_ArchetypesCount_Dirty = 0;

	private:
		inline void ValidateView()
		{
			Fetch();
			uint64_t archetypesCount = m_ArchetypesContexts.size();
			for (uint64_t i = 0; i < archetypesCount; i++)
			{
				m_ArchetypesContexts[i].ValidateEntitiesCount();
			}
		}

		template<typename Callable>
		void ForEachForward(Callable&& func)const noexcept
		{
			const uint64_t contextCount = m_ArchetypesContexts.size();
			for (uint64_t contextIndex = 0; contextIndex < contextCount; contextIndex++)
			{
				const ArchetypeContext& ctx = m_ArchetypesContexts[contextIndex];
				if (ctx.m_EntitiesCount == 0) continue;

				ArchetypeEntityData* entitiesData = ctx.Arch->m_EntitiesData.data();
				if (ctx.m_SameBucketsSizeInContainers)
				{
					std::tuple<ComponentsTypes*...> arraysTuple = {};
					uint64_t chunksCount = ctx.Arch->GetChunksCount();
					uint64_t entityIndex = 0;

					for (uint64_t chunkIdx = 0; chunkIdx < chunksCount; chunkIdx++)
					{
						CreateBucketsTuple<ComponentsTypes...>(arraysTuple, ctx, chunkIdx);
						uint64_t chunkSize = ctx.Arch->GetChunkSize(chunkIdx);
						for (uint64_t elementIndex = 0; elementIndex < chunkSize; elementIndex++, entityIndex++)
						{
							const auto& entityData = entitiesData[entityIndex];
							if (entityData.IsActive())
							{
								std::apply(
									func,
									std::forward_as_tuple(std::get<ComponentsTypes*>(arraysTuple)[elementIndex]...)
								);
							}
						}
					}
				}
				else
				{
					std::tuple<ChunksIterator<ComponentsTypes>...> vectorsTuple = {};
					const uint64_t entitiesCount = ctx.m_EntitiesCount;
					uint64_t entityIndex = 0;

					CreateChunksIteratorTuple<ComponentsTypes...>(vectorsTuple, ctx, false);

					while (entityIndex < entitiesCount)
					{
						const auto& entityData = entitiesData[entityIndex];
						if (entityData.IsActive())
						{
							std::apply(
								func,
								std::forward_as_tuple(*std::get<ChunksIterator<ComponentsTypes>>(vectorsTuple).m_DataPtr...)
							);
						}

						IncremenetChunksIterators<ComponentsTypes...>(vectorsTuple);
						entityIndex += 1;
					}
				}
			}
		}

		template<typename Callable>
		void ForEachBackward(Callable&& func) const noexcept
		{
			const uint64_t contextCount = m_ArchetypesContexts.size();
			for (uint64_t contextIndex = 0; contextIndex < contextCount; contextIndex++)
			{
				const ArchetypeContext& ctx = m_ArchetypesContexts[contextIndex];
				if (ctx.m_EntitiesCount == 0) continue;

				ArchetypeEntityData* entitiesData = ctx.Arch->m_EntitiesData.data();
				if (ctx.m_SameBucketsSizeInContainers)
				{
					std::tuple<ComponentsTypes*...> arraysTuple = {};
					int32_t entityIndex = ctx.m_EntitiesCount - 1;
					int32_t chunksCount = (int32_t)ctx.Arch->GetChunksCount();

					for (int32_t chunkIdx = chunksCount - 1; chunkIdx > -1; chunkIdx--)
					{
						CreateBucketsTuple<ComponentsTypes...>(arraysTuple, ctx, chunkIdx);
						int32_t elementIndex = (int32_t)ctx.Arch->GetChunkSize(chunkIdx) - 1;
						for (; elementIndex > -1; elementIndex--, entityIndex--)
						{
							const auto& entityData = entitiesData[entityIndex];
							if (entityData.IsActive())
							{
								std::apply(
									func,
									std::forward_as_tuple(std::get<ComponentsTypes*>(arraysTuple)[elementIndex]...)
								);
							}
						}
					}
				}
				else
				{
					std::tuple<ChunksIterator<ComponentsTypes>...> vectorsTuple = {};
					CreateChunksIteratorTuple<ComponentsTypes...>(vectorsTuple, ctx, true);
					int32_t entityIndex = ctx.m_EntitiesCount - 1;

					while (entityIndex > -1)
					{
						const auto& entityData = entitiesData[entityIndex];
						if (entityData.IsActive())
						{
							std::apply(
								func,
								std::forward_as_tuple(*std::get<ChunksIterator<ComponentsTypes>>(vectorsTuple).m_DataPtr...)
							);
						}

						DecrementChunksIterators<ComponentsTypes...>(vectorsTuple);
						entityIndex -= 1;
					}
				}
			}
		}

		template<typename Callable>
		void ForEachWithEntityForward(Callable&& func)const noexcept
		{
			Entity entityBuffor = {};
			const uint64_t contextCount = m_ArchetypesContexts.size();
			for (uint64_t contextIndex = 0; contextIndex < contextCount; contextIndex++)
			{
				const ArchetypeContext& ctx = m_ArchetypesContexts[contextIndex];
				if (ctx.m_EntitiesCount == 0) continue;

				ArchetypeEntityData* entitiesData = ctx.Arch->m_EntitiesData.data();
				if (ctx.m_SameBucketsSizeInContainers)
				{
					std::tuple<ComponentsTypes*...> arraysTuple = {};
					uint64_t chunksCount = ctx.Arch->GetChunksCount();
					uint64_t entityIndex = 0;

					for (uint64_t chunkIdx = 0; chunkIdx < chunksCount; chunkIdx++)
					{
						CreateBucketsTuple<ComponentsTypes...>(arraysTuple, ctx, chunkIdx);
						uint64_t chunkSize = ctx.Arch->GetChunkSize(chunkIdx);
						for (uint64_t elementIndex = 0; elementIndex < chunkSize; elementIndex++, entityIndex++)
						{
							const auto& entityData = entitiesData[entityIndex];
							if (entityData.IsActive())
							{
								entityBuffor.Set(entityData.m_ID, this->m_Container);
								std::apply(
									func,
									std::forward_as_tuple(entityBuffor, std::get<ComponentsTypes*>(arraysTuple)[elementIndex]...)
								);
							}
						}
					}
				}
				else
				{
					std::tuple<ChunksIterator<ComponentsTypes>...> vectorsTuple = {};
					const uint64_t entitiesCount = ctx.m_EntitiesCount;
					uint64_t entityIndex = 0;

					CreateChunksIteratorTuple<ComponentsTypes...>(vectorsTuple, ctx, false);

					while (entityIndex < entitiesCount)
					{
						const auto& entityData = entitiesData[entityIndex];
						if (entityData.IsActive())
						{
							entityBuffor.Set(entityData.m_ID, this->m_Container);
							std::apply(
								func,
								std::forward_as_tuple(entityBuffor, *std::get<ChunksIterator<ComponentsTypes>>(vectorsTuple).m_DataPtr...)
							);
						}

						IncremenetChunksIterators<ComponentsTypes...>(vectorsTuple);
						entityIndex += 1;
					}
				}
			}
		}

		template<typename Callable>
		void ForEachWithEntityBackward(Callable&& func)const noexcept
		{
			const uint64_t contextCount = m_ArchetypesContexts.size();
			Entity entityBuffor = {};
			for (uint64_t contextIndex = 0; contextIndex < contextCount; contextIndex++)
			{
				const ArchetypeContext& ctx = m_ArchetypesContexts[contextIndex];
				if (ctx.m_EntitiesCount == 0) continue;

				ArchetypeEntityData* entitiesData = ctx.Arch->m_EntitiesData.data();
				if (ctx.m_SameBucketsSizeInContainers)
				{
					std::tuple<ComponentsTypes*...> arraysTuple = {};
					int32_t entityIndex = ctx.m_EntitiesCount - 1;
					int32_t chunksCount = (int32_t)ctx.Arch->GetChunksCount();

					for (int32_t chunkIdx = chunksCount - 1; chunkIdx > -1; chunkIdx--)
					{
						CreateBucketsTuple<ComponentsTypes...>(arraysTuple, ctx, chunkIdx);
						int32_t elementIndex = (int32_t)ctx.Arch->GetChunkSize(chunkIdx) - 1;
						for (; elementIndex > -1; elementIndex--, entityIndex--)
						{
							const auto& entityData = entitiesData[entityIndex];
							if (entityData.IsActive())
							{
								entityBuffor.Set(entityData.m_ID, this->m_Container);
								std::apply(
									func,
									std::forward_as_tuple(entityBuffor, std::get<ComponentsTypes*>(arraysTuple)[elementIndex]...)
								);
							}
						}
					}
				}
				else
				{
					std::tuple<ChunksIterator<ComponentsTypes>...> vectorsTuple = {};
					CreateChunksIteratorTuple<ComponentsTypes...>(vectorsTuple, ctx, true);
					int32_t entityIndex = (int32_t)ctx.m_EntitiesCount - 1;

					while (entityIndex > -1)
					{
						const auto& entityData = entitiesData[entityIndex];
						if (entityData.IsActive())
						{
							entityBuffor.Set(entityData.m_ID, this->m_Container);
							std::apply(
								func,
								std::forward_as_tuple(entityBuffor, *std::get<ChunksIterator<ComponentsTypes>>(vectorsTuple).m_DataPtr...)
							);
						}

						DecrementChunksIterators<ComponentsTypes...>(vectorsTuple);
						entityIndex -= 1;
					}
				}
			}
		}

		inline uint64_t GetMinComponentsCount() const
		{
			uint64_t includesCount = sizeof...(ComponentsTypes);

			if (m_WithAnyOf.size() > 0)
			{
				includesCount += 1;
			}

			return sizeof...(ComponentsTypes) + m_WithAll.size();
		}

		void Invalidate()
		{
			m_ArchetypesContexts.clear();
			m_ContainedArchetypes.clear();
			m_ArchetypesCount_Dirty = std::numeric_limits<uint64_t>::max();
		}

		inline bool ContainArchetype(Archetype* arch) { return m_ContainedArchetypes.find(arch) != m_ContainedArchetypes.end(); }

		bool TryAddArchetype(Archetype& archetype, const bool& tryAddNeighbours)
		{
			if (!ContainArchetype(&archetype) && archetype.GetComponentsCount())
			{
				// exclude test
				{
					uint64_t excludeCount = m_Without.size();
					for (int i = 0; i < excludeCount; i++)
					{
						if (archetype.ContainType(m_Without[i]))
						{
							return false;
						}
					}
				}

				// required any
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
					if (!containRequiredAny) return false;
				}

				// required all
				{
					uint64_t requiredAllCount = m_WithAll.size();

					for (int i = 0; i < requiredAllCount; i++)
					{
						if (!archetype.ContainType(m_WithAll[i]))
						{
							return false;
						}
					}
				}


				// includes
				{
					ArchetypeContext& context = m_ArchetypesContexts.emplace_back();
					uint64_t chunkSizeInContainer = 0;

					for (uint64_t typeIdx = 0; typeIdx < m_Includes.Size(); typeIdx++)
					{
						auto typeIDIndex = archetype.FindTypeIndex(m_Includes.IDs()[typeIdx]);
						if (typeIDIndex == std::numeric_limits<uint64_t>::max())
						{
							m_ArchetypesContexts.pop_back();
							return false;
						}

						auto& packedContainer = archetype.m_PackedContainers[typeIDIndex];
						if (typeIdx == 0)
						{
							chunkSizeInContainer = packedContainer->GetChunkCapacity();
						}
						else if (context.m_SameBucketsSizeInContainers)
						{
							context.m_SameBucketsSizeInContainers = chunkSizeInContainer == packedContainer->GetChunkCapacity();
						}

						context.m_Containers[typeIdx] = packedContainer;
					}
					m_ContainedArchetypes.insert(&archetype);
					context.Arch = &archetype;
				}
			}

			if (tryAddNeighbours)
			{
				for (auto& [key, edge] : archetype.m_Edges)
				{
					if (edge.AddEdge != nullptr)
					{
						TryAddArchetype(*edge.AddEdge, tryAddNeighbours);
					}
				}
			}

			return true;
		}

		void NormalArchetypesFetching(
			ArchetypesMap& map,
			const uint64_t& maxComponentsInArchetype,
			const uint64_t& minComponentsCountInArchetype
		)
		{
			uint64_t addedArchetypes = 0;
			uint64_t archetypesVectorIndex = minComponentsCountInArchetype - 1;

			while (addedArchetypes == 0 && archetypesVectorIndex < maxComponentsInArchetype)
			{
				std::vector<Archetype*>& archetypesToTest = map.m_ArchetypesGroupedByComponentsCount[archetypesVectorIndex];

				uint64_t archsCount = archetypesToTest.size();
				for (uint64_t i = 0; i < archetypesToTest.size(); i++)
				{
					Archetype* archetype = archetypesToTest[i];

					if (TryAddArchetype(*archetype, true))
					{
						addedArchetypes++;
					}
				}

				archetypesVectorIndex++;
			}
		}

		bool TryAddArchetypeWithoutNeighbours(Archetype& archetype, const uint64_t minComponentsCountInArchetype)
		{
			if (archetype.GetComponentsCount() < minComponentsCountInArchetype) return false;

			if (!ContainArchetype(&archetype) && archetype.GetComponentsCount())
			{
				// exclude test
				{
					uint64_t excludeCount = m_Without.size();
					for (int i = 0; i < excludeCount; i++)
					{
						if (archetype.ContainType(m_Without[i]))
						{
							return false;
						}
					}
				}

				// required any
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
					if (!containRequiredAny) return false;
				}

				// required all
				{
					uint64_t requiredAllCount = m_WithAll.size();

					for (int i = 0; i < requiredAllCount; i++)
					{
						if (!archetype.ContainType(m_WithAll[i]))
						{
							return false;
						}
					}
				}

				// includes
				{
					ArchetypeContext& context = m_ArchetypesContexts.emplace_back();
					uint64_t chunkSizeInContainer = 0;

					for (uint64_t typeIdx = 0; typeIdx < m_Includes.Size(); typeIdx++)
					{
						auto typeIDIndex = archetype.FindTypeIndex(m_Includes.IDs()[typeIdx]);
						if (typeIDIndex == std::numeric_limits<uint64_t>::max())
						{
							m_ArchetypesContexts.pop_back();
							return false;
						}

						auto& packedContainer = archetype.m_PackedContainers[typeIDIndex];
						if (typeIdx == 0)
						{
							chunkSizeInContainer = packedContainer->GetChunkCapacity();
						}
						else if (context.m_SameBucketsSizeInContainers)
						{
							context.m_SameBucketsSizeInContainers = chunkSizeInContainer == packedContainer->GetChunkCapacity();
						}

						context.m_Containers[typeIdx] = packedContainer;
					}
					m_ContainedArchetypes.insert(&archetype);
					context.Arch = &archetype;
				}
			}

			return true;
		}

		void AddingArchetypesWithCheckingOnlyNewArchetypes(
			ArchetypesMap& map,
			const uint64_t& startArchetypesIndex,
			const uint64_t& minComponentsCountInArchetype
		)
		{
			auto& archetypes = map.m_Archetypes;
			uint64_t archetypesCount = map.m_Archetypes.Size();
			for (uint64_t i = startArchetypesIndex; i < archetypesCount; i++)
			{
				TryAddArchetypeWithoutNeighbours(archetypes[i], minComponentsCountInArchetype);
			}
		}


		template<typename T = void, typename... Args>
		void CreateBucketsTuple(
			std::tuple<ComponentsTypes*...>& arraysTuple,
			const ArchetypeContext& context,
			const uint64_t& chunkIndex
		) const noexcept
		{
			constexpr uint64_t compIdx = sizeof...(ComponentsTypes) - sizeof...(Args) - 1;
			std::get<T*>(arraysTuple) = reinterpret_cast<T*>(context.m_Containers[compIdx]->GetChunkData(chunkIndex));

			if constexpr (sizeof...(Args) == 0) return;

			CreateBucketsTuple<Args...>(
				arraysTuple,
				context,
				chunkIndex
				);
		}

		template<>
		void CreateBucketsTuple<void>(
			std::tuple<ComponentsTypes*...>& arraysTuple,
			const ArchetypeContext& context,
			const uint64_t& chunkIndex
			) const noexcept
		{

		}


		template<typename T = void, typename... Args>
		void CreateChunksIteratorTuple(
			std::tuple<ChunksIterator<ComponentsTypes>...>& vectorsTuple,
			const ArchetypeContext& context,
			const bool& isBackward
		) const
		{
			constexpr uint64_t compIdx = sizeof...(ComponentsTypes) - sizeof...(Args) - 1;
			std::get<ChunksIterator<T>>(vectorsTuple).Set(&((PackedContainer<T>*)context.m_Containers[compIdx])->m_Data, isBackward);

			if constexpr (sizeof...(Args) == 0) return;
			CreateChunksIteratorTuple<Args...>(vectorsTuple, context, isBackward);
		}

		template<>
		void CreateChunksIteratorTuple<void>(
			std::tuple<ChunksIterator<ComponentsTypes>...>& vectorsTuple,
			const ArchetypeContext& context,
			const bool& isBackward
			) const
		{

		}

		template<typename T = void, typename... Args>
		void IncremenetChunksIterators(
			std::tuple<ChunksIterator<ComponentsTypes>...>& vectorsTuple
		) const
		{
			std::get<ChunksIterator<T>>(vectorsTuple).Increment();
			if constexpr (sizeof...(Args) == 0) return;
			IncremenetChunksIterators<Args...>(vectorsTuple);
		}

		template<>
		void IncremenetChunksIterators(
			std::tuple<ChunksIterator<ComponentsTypes>...>& vectorsTuple
		)const
		{

		}

		template<typename T = void, typename... Args>
		void DecrementChunksIterators(
			std::tuple<ChunksIterator<ComponentsTypes>...>& vectorsTuple
		) const
		{
			std::get<ChunksIterator<T>>(vectorsTuple).Decrement();
			if constexpr (sizeof...(Args) == 0) return;
			DecrementChunksIterators<Args...>(vectorsTuple);
		}

		template<>
		void DecrementChunksIterators(
			std::tuple<ChunksIterator<ComponentsTypes>...>& vectorsTuple
		)const
		{

		}


	private:
		template<typename T = void, typename... Ts>
		inline void SetTupleElements(
			std::tuple<ComponentsTypes*...>& tuple,
			std::tuple<ComponentsTypes*...>& arraytuple,
			const uint64_t& elementIndex
		) const noexcept
		{
			std::get<T*>(tuple) = &std::get<T*>(arraytuple)[elementIndex];

			if constexpr (sizeof...(Ts) == 0) return;

			SetTupleElements<Ts...>(
				tuple,
				arraytuple,
				elementIndex
				);
		}

		template<>
		inline void SetTupleElements<void>(
			std::tuple<ComponentsTypes*...>& tuple,
			std::tuple<ComponentsTypes*...>& arraytuple,
			const uint64_t& elementIndex
			) const noexcept
		{

		}

		/*template<typename T = void, typename... Ts>
		void SetWithEntityTupleElements(
			std::tuple<Entity*, ComponentsTypes*...>& tuple,
			const TypeID*& typesIndexe,
			ComponentRef*& componentsRefs
		) const noexcept
		{
			constexpr uint64_t compIndex = sizeof...(ComponentsTypes) - sizeof...(Ts) - 1;
			auto& compRef = componentsRefs[typesIndexe[compIndex]];
			std::get<T*>(tuple) = reinterpret_cast<T*>(compRef.ComponentPointer);

			if constexpr (sizeof...(Ts) == 0) return;

			SetWithEntityTupleElements<Ts...>(
				tuple,
				typesIndexe,
				componentsRefs
				);
		}

		template<>
		void SetWithEntityTupleElements<void>(
			std::tuple<Entity*, ComponentsTypes*...>& tuple,
			const TypeID*& typesIndexe,
			ComponentRef*& componentsRefs
			) const noexcept
		{

		}*/

#pragma region BATCH ITERATOR
	public:
		class BatchIterator
		{
			using ViewType = View<ComponentsTypes...>;

			template<typename... Types>
			friend class View;
		public:
			BatchIterator() {}

			BatchIterator(
				ViewType& view,
				const uint64_t& firstArchetypeIndex,
				const uint64_t& firstIterationIndex,
				const uint64_t& lastArchetypeIndex,
				const uint64_t& lastIterationIndex
			) :
				m_IsValid(true),
				m_View(&view),
				m_FirstArchetypeIndex(firstArchetypeIndex),
				m_FirstIterationIndex(firstIterationIndex),
				m_LastArchetypeIndex(lastArchetypeIndex),
				m_LastIterationIndex(lastIterationIndex)
			{
			}

			~BatchIterator() {}

			inline bool IsValid() { return m_IsValid; }

			template<typename Callable>
			inline void ForEach(Callable&& func)
			{
				if (!m_IsValid) return;

				if constexpr (std::is_invocable<Callable, Entity&, ComponentsTypes&...>())
				{
					ForEachWithEntity(func);
				}
				else
				{
					ForEachWithoutEntity(func);
				}
			}
		private:
			template<typename Callable>
			void ForEachWithoutEntity(Callable&& func)
			{
				/*std::tuple<ComponentsTypes*...> tuple = {};
				uint64_t contextIndex = m_FirstArchetypeIndex;
				uint64_t contextCount = m_LastArchetypeIndex + 1;

				for (; contextIndex < contextCount; contextIndex++)
				{
					ArchetypeContext& ctx = m_View->m_ArchetypesContexts[contextIndex];
					TypeID* typeIndexes = ctx.TypeIndexes;
					uint64_t componentsCount = ctx.Arch->GetComponentsCount();

					ArchetypeEntityData* entitiesData = ctx.Arch->m_EntitiesData.data();
					ComponentRef* componentsRefs = ctx.Arch->m_ComponentsRefs.data();

					uint64_t iterationIndex;
					uint64_t iterationsCount;

					if (contextIndex == m_FirstArchetypeIndex)
						iterationIndex = m_FirstIterationIndex;
					else
						iterationIndex = 0;

					if (contextIndex == m_LastArchetypeIndex)
						iterationsCount = m_LastIterationIndex;
					else
						iterationsCount = ctx.m_EntitiesCount;

					for (; iterationIndex < iterationsCount; iterationIndex++)
					{
						auto& entityData = entitiesData[iterationIndex];
						if (entityData.IsActive())
						{
							ComponentRef* firstComponentPtr = componentsRefs + (iterationIndex * componentsCount);
							SetTupleElements<ComponentsTypes...>(
								tuple,
								0,
								typeIndexes,
								firstComponentPtr
								);
							decs::ApplayTupleWithPointersAsRefrences(func, tuple);
						}
					}
				}*/
			}

			template<typename Callable>
			void ForEachWithEntity(Callable&& func)
			{
				/*Entity iteratedEntity = {};
				std::tuple<Entity*, ComponentsTypes*...> tuple = {};
				std::get<Entity*>(tuple) = &iteratedEntity;
				uint64_t contextIndex = m_FirstArchetypeIndex;
				uint64_t contextCount = m_LastArchetypeIndex + 1;
				Container* container = m_View->m_Container;

				for (; contextIndex < contextCount; contextIndex++)
				{
					ArchetypeContext& ctx = m_View->m_ArchetypesContexts[contextIndex];
					TypeID* typeIndexes = ctx.TypeIndexes;
					uint64_t componentsCount = ctx.Arch->GetComponentsCount();

					ArchetypeEntityData* entitiesData = ctx.Arch->m_EntitiesData.data();
					ComponentRef* componentsRefs = ctx.Arch->m_ComponentsRefs.data();

					uint64_t iterationIndex;
					uint64_t iterationsCount;

					if (contextIndex == m_FirstArchetypeIndex)
						iterationIndex = m_FirstIterationIndex;
					else
						iterationIndex = 0;

					if (contextIndex == m_LastArchetypeIndex)
						iterationsCount = m_LastIterationIndex;
					else
						iterationsCount = ctx.m_EntitiesCount;

					for (; iterationIndex < iterationsCount; iterationIndex++)
					{
						auto& entityData = entitiesData[iterationIndex];
						if (entityData.IsActive())
						{
							iteratedEntity.Set(
								entityData.ID(),
								container
							);
							ComponentRef* firstComponentPtr = componentsRefs + (iterationIndex * componentsCount);
							SetWithEntityTupleElements<ComponentsTypes...>(
								tuple,
								0,
								typeIndexes,
								firstComponentPtr
								);
							decs::ApplayTupleWithPointersAsRefrences(func, tuple);
						}
					}
				}*/
			}

		protected:
			ViewType* m_View = nullptr;
			bool m_IsValid = false;

			uint64_t m_FirstArchetypeIndex = 0;
			uint64_t m_FirstIterationIndex = 0;
			uint64_t m_LastArchetypeIndex = 0;
			uint64_t m_LastIterationIndex = 0;

		private:
			/*template<typename T = void, typename... Ts>
			inline void SetTupleElements(
				std::tuple<ComponentsTypes*...>& tuple,
				const uint64_t& compIndex,
				TypeID*& typesIndexes,
				ComponentRef*& componentsRefs
			)const
			{
				auto& compRef = componentsRefs[typesIndexes[compIndex]];
				std::get<T*>(tuple) = reinterpret_cast<T*>(compRef.ComponentPointer);

				if constexpr (sizeof...(Ts) == 0) return;

				SetTupleElements<Ts...>(
					tuple,
					compIndex + 1,
					typesIndexes,
					componentsRefs
					);
			}

			template<>
			inline void SetTupleElements<void>(
				std::tuple<ComponentsTypes*...>& tuple,
				const uint64_t& compIndex,
				TypeID*& typesIndexes,
				ComponentRef*& componentsRefs
				) const
			{

			}

			template<typename T = void, typename... Ts>
			inline void SetWithEntityTupleElements(
				std::tuple<Entity*, ComponentsTypes*...>& tuple,
				const uint64_t& compIndex,
				TypeID*& typesIndexes,
				ComponentRef*& componentsRefs
			) const
			{
				auto& compRef = componentsRefs[typesIndexes[compIndex]];
				std::get<T*>(tuple) = reinterpret_cast<T*>(compRef.ComponentPointer);

				if constexpr (sizeof...(Ts) == 0) return;

				SetWithEntityTupleElements<Ts...>(
					tuple,
					compIndex + 1,
					typesIndexes,
					componentsRefs
					);
			}

			template<>
			inline void SetWithEntityTupleElements<void>(
				std::tuple<Entity*, ComponentsTypes*...>& tuple,
				const uint64_t& compIndex,
				TypeID*& typesIndexes,
				ComponentRef*& componentsRefs
				) const
			{

			}*/
		};

#pragma endregion

	public:
		void CreateBatchIterators(
			std::vector<BatchIterator>& iterators,
			const uint64_t& desiredBatchesCount,
			const uint64_t& minBatchSize
		)
		{
			if (!IsValid())
			{
				return;
			}

			Fetch();

			uint64_t entitiesCount = 0;

			for (ArchetypeContext& archContext : m_ArchetypesContexts)
			{
				archContext.ValidateEntitiesCount();
				entitiesCount += archContext.m_EntitiesCount;
			}

			uint64_t realDesiredBatchSize = std::llround(std::ceil((float)entitiesCount / (float)desiredBatchesCount));
			uint64_t finalBatchSize;

			if (realDesiredBatchSize < minBatchSize)
				finalBatchSize = minBatchSize;
			else
				finalBatchSize = realDesiredBatchSize;

			// archetype iteration stuff:
			uint64_t currentArchetypeIndex = 0;
			uint64_t archsCount = m_ArchetypesContexts.size();
			uint64_t currentArchEntitiesStartIndex = 0;

			// iterator creating stuff:
			uint64_t startArchetypeIndex;
			uint64_t startIterationIndex; // first archetype start iteration index
			// last iteration condition
			uint64_t lastArchetypeIndex;
			uint64_t lastIterationIndex; // last archetypoe last iterationIndex

			uint64_t itNeededEntitiesCount;

			while (entitiesCount > 0)
			{
				startArchetypeIndex = currentArchetypeIndex;
				startIterationIndex = currentArchEntitiesStartIndex;
				itNeededEntitiesCount = finalBatchSize;

				for (; currentArchetypeIndex < archsCount;)
				{
					ArchetypeContext& ctx = m_ArchetypesContexts[currentArchetypeIndex];
					if (ctx.m_EntitiesCount == 0)
					{
						currentArchetypeIndex += 1;
						continue;
					}
					uint64_t archAvailableEntities = ctx.m_EntitiesCount - currentArchEntitiesStartIndex;

					if (itNeededEntitiesCount < archAvailableEntities)
					{
						currentArchEntitiesStartIndex += itNeededEntitiesCount;
						itNeededEntitiesCount = 0;
						lastArchetypeIndex = currentArchetypeIndex;
					}
					else
					{
						itNeededEntitiesCount -= archAvailableEntities;
						currentArchEntitiesStartIndex += archAvailableEntities;
						lastArchetypeIndex = currentArchetypeIndex;
						currentArchetypeIndex += 1;
					}

					if (itNeededEntitiesCount == 0)
					{
						break;
					}
				}

				lastIterationIndex = currentArchEntitiesStartIndex;

				BatchIterator& it = iterators.emplace_back(
					*this,
					startArchetypeIndex,
					startIterationIndex,
					lastArchetypeIndex,
					lastIterationIndex
				);

				entitiesCount -= finalBatchSize - itNeededEntitiesCount;
			}
		}
	};
}