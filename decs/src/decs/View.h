#pragma once
#include "Type.h"
#include "TypeGroup.h"
#include "Core.h"
#include "Entity.h"
#include "Container.h"
#include "decs/CustomApplay.h"
#include <algorithm>

namespace decs
{
	enum class IterationType : uint8_t
	{
		Forward = 0,
		Backward = 1
	};

	template<typename... ComponentsTypes>
	class View
	{
	private:
		struct ArchetypeContext
		{
		public:
			Archetype* Arch = nullptr;
			uint64_t TypeIndexes[sizeof...(ComponentsTypes)]; // indexy kontenerów z archetypu z których ma ko¿ystaæ widok - shit
			uint64_t m_EntitiesCount = 0;

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
			Fetch();
			uint64_t archetypesCount = m_ArchetypesContexts.size();
			for (uint64_t i = 0; i < archetypesCount; i++)
			{
				m_ArchetypesContexts[i].ValidateEntitiesCount();
			}

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
			Fetch();
			uint64_t archetypesCount = m_ArchetypesContexts.size();
			for (uint64_t i = 0; i < archetypesCount; i++)
			{
				m_ArchetypesContexts[i].ValidateEntitiesCount();
			}

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

	public:
		template<typename Callable>
		void ForEachForward(Callable&& func)const noexcept
		{
			const uint64_t contextCount = m_ArchetypesContexts.size();
			std::tuple<ComponentsTypes*...> tuple = {};
			for (uint64_t contextIndex = 0; contextIndex < contextCount; contextIndex++)
			{
				const ArchetypeContext& ctx = m_ArchetypesContexts[contextIndex];
				if (ctx.m_EntitiesCount == 0) continue;
				const TypeID* typeIndexes = ctx.TypeIndexes;
				const uint64_t componentsCount = ctx.Arch->GetComponentsCount();

				ArchetypeEntityData* entitiesData = ctx.Arch->m_EntitiesData.data();
				ComponentRef* componentsRefs = ctx.Arch->m_ComponentsRefs.data();

				const uint64_t ctxEntitiesCount = ctx.m_EntitiesCount;
				for (uint64_t iterationIndex = 0; iterationIndex < ctxEntitiesCount; iterationIndex++)
				{
					const auto& entityData = entitiesData[iterationIndex];
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
			}
		}

	private:

		template<typename Callable>
		void ForEachBackward(Callable&& func) const noexcept
		{
			/*const uint64_t contextCount = m_ArchetypesContexts.size();
			std::tuple<ComponentsTypes*...> tuple = {};
			for (uint64_t contextIndex = 0; contextIndex < contextCount; contextIndex++)
			{
				const ArchetypeContext& ctx = m_ArchetypesContexts[contextIndex];
				if (ctx.m_EntitiesCount == 0) continue;

				const TypeID* typeIndexes = ctx.TypeIndexes;
				const uint64_t componentsCount = ctx.Arch->GetComponentsCount();

				const ArchetypeEntityData* entitiesData = ctx.Arch->m_EntitiesData.data();
				ComponentRef* componentsRefs = ctx.Arch->m_ComponentsRefs.data();

				for (int64_t iterationIndex = ctx.m_EntitiesCount - 1; iterationIndex > -1; iterationIndex--)
				{
					const auto& entityData = entitiesData[iterationIndex];
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
		void ForEachWithEntityForward(Callable&& func)const noexcept
		{
			/*const uint64_t contextCount = m_ArchetypesContexts.size();
			Entity iteratedEntity = {};
			std::tuple<Entity*, ComponentsTypes*...> tuple = {};
			std::get<Entity*>(tuple) = &iteratedEntity;

			for (uint64_t contextIndex = 0; contextIndex < contextCount; contextIndex++)
			{
				const ArchetypeContext& ctx = m_ArchetypesContexts[contextIndex];
				if (ctx.m_EntitiesCount == 0) continue;

				const TypeID* typeIndexes = ctx.TypeIndexes;
				const uint64_t componentsCount = ctx.Arch->GetComponentsCount();

				const ArchetypeEntityData* entitiesData = ctx.Arch->m_EntitiesData.data();
				ComponentRef* componentsRefs = ctx.Arch->m_ComponentsRefs.data();

				const uint64_t ctxEntitiesCount = ctx.m_EntitiesCount;
				for (uint64_t iterationIndex = 0; iterationIndex < ctxEntitiesCount; iterationIndex++)
				{
					const auto& entityData = entitiesData[iterationIndex];
					if (entityData.IsActive())
					{
						iteratedEntity.Set(
							entityData.ID(),
							m_Container
						);
						ComponentRef* firstComponentPtr = componentsRefs + (iterationIndex * componentsCount);
						SetWithEntityTupleElements<ComponentsTypes...>(
							tuple,
							typeIndexes,
							firstComponentPtr
							);
						decs::ApplayTupleWithPointersAsRefrences(func, tuple);
					}
				}
			}*/
		}

		template<typename Callable>
		void ForEachWithEntityBackward(Callable&& func)const noexcept
		{
			/*const uint64_t contextCount = m_ArchetypesContexts.size();
			Entity iteratedEntity = {};
			std::tuple<Entity*, ComponentsTypes*...> tuple = {};
			std::get<Entity*>(tuple) = &iteratedEntity;

			for (uint64_t contextIndex = 0; contextIndex < contextCount; contextIndex++)
			{
				const ArchetypeContext& ctx = m_ArchetypesContexts[contextIndex];
				if (ctx.m_EntitiesCount == 0) continue;

				const TypeID* typeIndexes = ctx.TypeIndexes;
				const uint64_t componentsCount = ctx.Arch->GetComponentsCount();

				const ArchetypeEntityData* entitiesData = ctx.Arch->m_EntitiesData.data();
				ComponentRef* componentsRefs = ctx.Arch->m_ComponentsRefs.data();

				for (int64_t iterationIndex = ctx.m_EntitiesCount - 1; iterationIndex > -1; iterationIndex--)
				{
					const auto& entityData = entitiesData[iterationIndex];
					if (entityData.IsActive())
					{
						iteratedEntity.Set(
							entityData.ID(),
							m_Container
						);
						ComponentRef* firstComponentPtr = componentsRefs + (iterationIndex * componentsCount);
						SetWithEntityTupleElements<ComponentsTypes...>(
							tuple,
							typeIndexes,
							firstComponentPtr
							);
						decs::ApplayTupleWithPointersAsRefrences(func, tuple);
					}
				}
			}*/
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

					for (uint64_t typeIdx = 0; typeIdx < m_Includes.Size(); typeIdx++)
					{
						auto typeIDIndex = archetype.FindTypeIndex(m_Includes.IDs()[typeIdx]);
						if (typeIDIndex == std::numeric_limits<uint64_t>::max())
						{
							m_ArchetypesContexts.pop_back();
							return false;
						}
						context.TypeIndexes[typeIdx] = typeIDIndex;
					}

					std::sort(context.TypeIndexes, context.TypeIndexes + sizeof...(ComponentsTypes));
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

					for (uint64_t typeIdx = 0; typeIdx < m_Includes.Size(); typeIdx++)
					{
						auto typeIDIndex = archetype.FindTypeIndex(m_Includes.IDs()[typeIdx]);
						if (typeIDIndex == std::numeric_limits<uint64_t>::max())
						{
							m_ArchetypesContexts.pop_back();
							return false;
						}
						context.TypeIndexes[typeIdx] = typeIDIndex;
					}
					std::sort(context.TypeIndexes, context.TypeIndexes + sizeof...(ComponentsTypes));
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

	private:
		/*template<typename T = void, typename... Ts>
		inline void SetTupleElements(
			std::tuple<ComponentsTypes*...>& tuple,
			const uint64_t& compIndex,
			const TypeID*& typesIndexe,
			ComponentRef*& componentsRefs
		) const noexcept
		{
			auto& compRef = componentsRefs[typesIndexe[compIndex]];
			std::get<T*>(tuple) = reinterpret_cast<T*>(compRef.ComponentPointer);

			if constexpr (sizeof...(Ts) == 0) return;

			SetTupleElements<Ts...>(
				tuple,
				compIndex + 1,
				typesIndexe,
				componentsRefs
				);
		}

		template<>
		inline void SetTupleElements<void>(
			std::tuple<ComponentsTypes*...>& tuple,
			const uint64_t& compIndex,
			const TypeID*& typesIndexe,
			ComponentRef*& componentsRefs
			) const noexcept
		{

		}

		template<typename T = void, typename... Ts>
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