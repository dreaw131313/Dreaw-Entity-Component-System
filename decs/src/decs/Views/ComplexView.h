#pragma once
#include "decspch.h"

#include "ViewCore.h"

#include "../Type.h"
#include "../TypeGroup.h"
#include "../core.h"
#include "../ComponentContainer.h"
#include "../Entity.h"
#include "../Container.h"

namespace decs
{
	template<typename... ComponentsTypes>
	class ComplexView
	{
		using ArchetypeContext = ViewArchetypeContext<sizeof...(ComponentsTypes)>;

	public:
		ComplexView()
		{

		}

		~ComplexView()
		{

		}

		template<typename... Excludes>
		ComplexView& WithoutAnyOf()
		{
			m_IsDirty = true;
			m_Excludes.clear();
			findIds<Excludes...>(m_Excludes);
			return *this;
		}

		template<typename... Excludes>
		ComplexView& WithAnyOf()
		{
			m_IsDirty = true;
			m_RequiredAny.clear();
			findIds<Excludes...>(m_RequiredAny);
			return *this;
		}

		template<typename... Excludes>
		ComplexView& WithAll()
		{
			m_IsDirty = true;
			m_RequiredAll.clear();
			findIds<Excludes...>(m_RequiredAll);
			return *this;
		}

		void Fetch(Container& container)
		{
			if (m_IsDirty || &container != m_Container)
			{
				m_IsDirty = false;
				m_Container = &container;
				Invalidate();
			}

			uint64_t containerArchetypesCount = m_Container->m_ArchetypesMap.m_Archetypes.size();
			if (m_ArchetypesCount_Dirty != containerArchetypesCount)
			{
				uint64_t newArchetypesCount = containerArchetypesCount - m_ArchetypesCount_Dirty;

				uint64_t minComponentsCountInArchetype = GetMinComponentsCount();
				if (m_RequiredAny.size() > 0) minComponentsCountInArchetype += 1;

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
		void ForEach(Callable func)
		{
			for (ArchetypeContext& archContext : m_ArchetypesContexts)
				archContext.ValidateEntitiesCount();

			uint32_t contextCount = m_ArchetypesContexts.size();
			std::tuple<ComponentsTypes*...> tuple = {};
			for (uint32_t contextIndex = 0; contextIndex < contextCount; contextIndex++)
			{
				ArchetypeContext& ctx = m_ArchetypesContexts[contextIndex];
				TypeID* typeIndexes = ctx.TypeIndexes;
				uint64_t componentsCount = ctx.Arch->GetComponentsCount();

				ArchetypeEntityData* entitiesData = ctx.Arch->m_EntitiesData.data();
				ComponentRef* componentsRefs = ctx.Arch->m_ComponentsRefs.data();

				for (int64_t iterationIndex = ctx.EntitiesCount() - 1; iterationIndex > -1; iterationIndex--)
				{
					auto& entityData = entitiesData[iterationIndex];
					if (entityData.IsActive)
					{
						SetTupleElements<ComponentsTypes...>(
							tuple,
							0,
							iterationIndex * componentsCount,
							typeIndexes,
							componentsRefs
							);
						std::apply(func, tuple);
					}
				}
			}
		}

		template<typename Callable>
		void ForEachWithEntity(Callable func)
		{
			for (ArchetypeContext& archContext : m_ArchetypesContexts)
				archContext.ValidateEntitiesCount();

			uint32_t contextCount = m_ArchetypesContexts.size();
			std::tuple<Entity, ComponentsTypes*...> tuple = {};
			for (uint32_t contextIndex = 0; contextIndex < contextCount; contextIndex++)
			{
				ArchetypeContext& ctx = m_ArchetypesContexts[contextIndex];
				TypeID* typeIndexes = ctx.TypeIndexes;
				uint64_t componentsCount = ctx.Arch->GetComponentsCount();

				ArchetypeEntityData* entitiesData = ctx.Arch->m_EntitiesData.data();
				ComponentRef* componentsRefs = ctx.Arch->m_ComponentsRefs.data();

				for (int64_t iterationIndex = ctx.EntitiesCount() - 1; iterationIndex > -1; iterationIndex--)
				{
					auto& entityData = entitiesData[iterationIndex];
					if (entityData.IsActive)
					{
						std::get<Entity>(tuple) = Entity(entityData.ID, this->m_Container);
						SetWithEntityTupleElements<ComponentsTypes...>(
							tuple,
							0,
							iterationIndex * componentsCount,
							typeIndexes,
							componentsRefs
							);
						std::apply(func, tuple);
					}
				}
			}
		}

	private:
		TypeGroup<ComponentsTypes...> m_Includes = {};
		std::vector<TypeID> m_Excludes;
		std::vector<TypeID> m_RequiredAny;
		std::vector<TypeID> m_RequiredAll;

		Container* m_Container;
		bool m_IsDirty;

		std::vector<ArchetypeContext> m_ArchetypesContexts;
		ecsSet<Archetype*> m_ContainedArchetypes;

		// cache value to check if view should be updated:
		uint32_t m_ArchetypesCount_Dirty = 0;

	private:

		inline uint64_t GetMinComponentsCount() const
		{
			uint64_t includesCount = sizeof...(ComponentsTypes);

			return sizeof...(ComponentsTypes) + m_RequiredAll.size();
		}

		void Invalidate()
		{
			m_ArchetypesContexts.clear();
			m_ContainedArchetypes.clear();
			m_ArchetypesCount_Dirty = std::numeric_limits<uint32_t>::max();
		}

		inline bool ContainArchetype(Archetype* arch) { return m_ContainedArchetypes.find(arch) != m_ContainedArchetypes.end(); }

		virtual bool TryAddArchetype(Archetype& archetype, const bool& tryAddNeighbours)
		{
			if (!ContainArchetype(&archetype) && archetype.GetComponentsCount())
			{
				// exclude test
				{
					uint64_t excludeCount = m_Excludes.size();
					for (int i = 0; i < excludeCount; i++)
					{
						if (archetype.ContainType(m_Excludes[i]))
						{
							return false;
						}
					}
				}

				// required any
				{
					uint64_t requiredAnyCount = m_RequiredAny.size();
					bool containRequiredAny = requiredAnyCount == 0;

					for (int i = 0; i < requiredAnyCount; i++)
					{
						if (archetype.ContainType(m_RequiredAny[i]))
						{
							containRequiredAny = true;
							break;
						}
					}
					if (!containRequiredAny) return false;
				}

				// required all
				{
					uint64_t requiredAllCount = m_RequiredAll.size();

					for (int i = 0; i < requiredAllCount; i++)
					{
						if (!archetype.ContainType(m_RequiredAll[i]))
						{
							return false;
						}
					}
				}


				// includes
				{
					ArchetypeContext& context = m_ArchetypesContexts.emplace_back();

					for (uint32_t typeIdx = 0; typeIdx < m_Includes.Size(); typeIdx++)
					{
						auto typeID_It = archetype.m_TypeIDsIndexes.find(m_Includes.IDs()[typeIdx]);
						if (typeID_It == archetype.m_TypeIDsIndexes.end())
						{
							m_ArchetypesContexts.pop_back();
							return false;
						}
						context.TypeIndexes[typeIdx] = typeID_It->second;
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
			uint32_t addedArchetypes = 0;
			uint64_t archetypesVectorIndex = minComponentsCountInArchetype - 1;

			while (addedArchetypes == 0 && archetypesVectorIndex < maxComponentsInArchetype)
			{
				std::vector<Archetype*>& archetypesToTest = map.m_ArchetypesGroupedByComponentsCount[archetypesVectorIndex];

				uint32_t archsCount = archetypesToTest.size();
				for (uint32_t i = 0; i < archetypesToTest.size(); i++)
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
					uint64_t excludeCount = m_Excludes.size();
					for (int i = 0; i < excludeCount; i++)
					{
						if (archetype.ContainType(m_Excludes[i]))
						{
							return false;
						}
					}
				}

				// required any
				{
					uint64_t requiredAnyCount = m_RequiredAny.size();
					bool containRequiredAny = requiredAnyCount == 0;

					for (int i = 0; i < requiredAnyCount; i++)
					{
						if (archetype.ContainType(m_RequiredAny[i]))
						{
							containRequiredAny = true;
							break;
						}
					}
					if (!containRequiredAny) return false;
				}

				// required all
				{
					uint64_t requiredAllCount = m_RequiredAll.size();

					for (int i = 0; i < requiredAllCount; i++)
					{
						if (!archetype.ContainType(m_RequiredAll[i]))
						{
							return false;
						}
					}
				}


				// includes
				{
					ArchetypeContext& context = m_ArchetypesContexts.emplace_back();

					for (uint32_t typeIdx = 0; typeIdx < m_Includes.Size(); typeIdx++)
					{
						auto typeID_It = archetype.m_TypeIDsIndexes.find(m_Includes.IDs()[typeIdx]);
						if (typeID_It == archetype.m_TypeIDsIndexes.end())
						{
							m_ArchetypesContexts.pop_back();
							return false;
						}
						context.TypeIndexes[typeIdx] = typeID_It->second;
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
			Archetype** archetypes = map.m_Archetypes.data();
			uint64_t archetypesCount = map.m_Archetypes.size();
			for (uint64_t i = startArchetypesIndex; i < archetypesCount; i++)
			{
				TryAddArchetypeWithoutNeighbours(*archetypes[i], minComponentsCountInArchetype);
			}
		}

	private:
		template<typename T = void, typename... Ts>
		void SetTupleElements(
			std::tuple<ComponentsTypes*...>& tuple,
			const uint64_t& compIndex,
			const uint64_t& firstEntityCompIndex,
			TypeID*& typesIndexe,
			ComponentRef*& componentsRefs
		)
		{
			uint64_t compIndexInArchetype = typesIndexe[compIndex];
			auto& compRef = componentsRefs[firstEntityCompIndex + compIndexInArchetype];
			std::get<T*>(tuple) = reinterpret_cast<T*>(compRef.ComponentPointer);

			if (sizeof...(Ts) == 0) return;

			SetTupleElements<Ts...>(
				tuple,
				compIndex + 1,
				firstEntityCompIndex,
				typesIndexe,
				componentsRefs
				);
		}

		template<>
		void SetTupleElements<void>(
			std::tuple<ComponentsTypes*...>& tuple,
			const uint64_t& compIndex,
			const uint64_t& firstEntityCompIndex,
			TypeID*& typesIndexe,
			ComponentRef*& componentsRefs
			)
		{

		}

		template<typename T = void, typename... Ts>
		void SetWithEntityTupleElements(
			std::tuple<Entity, ComponentsTypes*...>& tuple,
			const uint64_t& compIndex,
			const uint64_t& firstEntityCompIndex,
			TypeID*& typesIndexe,
			ComponentRef*& componentsRefs
		)
		{
			uint64_t compIndexInArchetype = typesIndexe[compIndex];
			auto& compRef = componentsRefs[firstEntityCompIndex + compIndexInArchetype];
			std::get<T*>(tuple) = reinterpret_cast<T*>(compRef.ComponentPointer);

			if (sizeof...(Ts) == 0) return;

			SetWithEntityTupleElements<Ts...>(
				tuple,
				compIndex + 1,
				firstEntityCompIndex,
				typesIndexe,
				componentsRefs
				);
		}

		template<>
		void SetWithEntityTupleElements<void>(
			std::tuple<Entity, ComponentsTypes*...>& tuple,
			const uint64_t& compIndex,
			const uint64_t& firstEntityCompIndex,
			TypeID*& typesIndexe,
			ComponentRef*& componentsRefs
			)
		{

		}

	private:

		struct BaseIterator
		{
			using ViewType = ComplexView<ComponentsTypes...>;
			using DataTuple = std::tuple<ComponentsTypes*...>;
		public:
			BaseIterator() {}

			BaseIterator(ViewType& view) :
				m_View(&view),
				m_Container(view.m_Container)
			{

			}

			~BaseIterator()
			{

			}

			inline bool IsValid() { return m_IsValid; }

			inline DataTuple& GetTuple()
			{
				return m_DataTuple;
			}
			virtual void Advance() = 0;

		protected:
			DataTuple m_DataTuple;

			ViewType* m_View = nullptr;
			Container* m_Container = nullptr;

			ArchetypeContext* m_ArchetypeContext = nullptr;
			Archetype* m_CurrentArchetype = nullptr;
			ComponentRef* componentsRefs = nullptr;

			uint64_t m_ArchetypeIndex = 0;
			int64_t m_IterationIndex = 0;

			bool m_IsValid = false;

		protected:
			virtual void SetCurrentArchetypeContext(ArchetypeContext* archetypeContext)
			{
				m_ArchetypeContext = archetypeContext;
				m_CurrentArchetype = m_ArchetypeContext->Arch;
				componentsRefs = m_ArchetypeContext->Arch->m_ComponentsRefs.data();
				m_IterationIndex = m_ArchetypeContext->EntitiesCount() - 1;
			}

			inline void PrepareTuple()
			{
				SetTupleElements<ComponentsTypes...>(0, m_IterationIndex * m_CurrentArchetype->GetComponentsCount());
			}

			template<typename T = void, typename... Ts>
			void SetTupleElements(const uint64_t& compIndex, const uint64_t& firstEntityCompIndex)
			{
				uint64_t compIndexInArchetype = m_ArchetypeContext->TypeIndexes[compIndex];
				auto& compRef = componentsRefs[firstEntityCompIndex + compIndexInArchetype];
				std::get<T*>(m_DataTuple) = reinterpret_cast<T*>(compRef.ComponentPointer);

				if (sizeof...(Ts) == 0) return;

				SetTupleElements<Ts...>(compIndex + 1, firstEntityCompIndex);
			}

			template<>
			void SetTupleElements<void>(const uint64_t& compIndex, const uint64_t& firstEntityCompIndex)
			{

			}

			void InvalidateTuple()
			{
				InvalidateTupleElements<ComponentsTypes...>();
			}

			template<typename T = void, typename... Ts>
			void InvalidateTupleElements()
			{
				std::get<T*>(m_DataTuple) = nullptr;

				if (sizeof...(Ts) == 0) return;

				InvalidateTupleElements<Ts...>();
			}

			template<>
			void InvalidateTupleElements<void>()
			{

			}
		};

	public:
		struct Iterator : public BaseIterator
		{
			using ViewType = ComplexView <ComponentsTypes...>;
		public:
			Iterator() {}

			Iterator(ViewType& view) :
				BaseIterator(view)
			{
				if (this->m_View->m_ArchetypesContexts.size() == 0)
				{
					this->m_IsValid = false;
					this->InvalidateTuple();
				}
				else
				{
					this->m_IsValid = false;
					uint64_t archetypsCount = this->m_View->m_ArchetypesContexts.size();
					this->m_ArchetypeIndex = 0;
					for (; this->m_ArchetypeIndex < archetypsCount; this->m_ArchetypeIndex++)
					{
						this->SetCurrentArchetypeContext(&this->m_View->m_ArchetypesContexts[this->m_ArchetypeIndex]);

						if (this->m_ArchetypeContext->EntitiesCount() > 0)
						{
							while (this->m_IterationIndex >= 0)
							{
								if (this->m_ArchetypeContext->Arch->m_EntitiesData[this->m_IterationIndex].IsActive)
								{
									this->m_IsValid = true;
									this->PrepareTuple();
									return;
								}
								this->m_IterationIndex -= 1;
							}
						}
					}
				}

				this->m_IsValid = false;
				this->InvalidateTuple();
			}

			~Iterator()
			{
			}

			virtual void Advance() override
			{
				this->m_IterationIndex -= 1;

				while (this->m_IterationIndex >= 0)
				{
					if (this->m_CurrentArchetype->m_EntitiesData[this->m_IterationIndex].IsActive)
					{
						this->PrepareTuple();
						return;
					}
					this->m_IterationIndex -= 1;
				}

				this->m_ArchetypeIndex += 1;

				for (; this->m_ArchetypeIndex < this->m_View->m_ArchetypesContexts.size(); this->m_ArchetypeIndex++)
				{
					this->SetCurrentArchetypeContext(&this->m_View->m_ArchetypesContexts[this->m_ArchetypeIndex]);

					if (this->m_ArchetypeContext->EntitiesCount() > 0)
					{
						while (this->m_IterationIndex >= 0)
						{
							if (this->m_CurrentArchetype->m_EntitiesData[this->m_IterationIndex].IsActive)
							{
								this->PrepareTuple();
								return;
							}
							this->m_IterationIndex -= 1;
						}
					}
				}

				this->m_IsValid = false;
				this->InvalidateTuple();
				return;
			}

		};

		class BatchIterator : public BaseIterator
		{
			using ViewType = ComplexView<ComponentsTypes...>;
		public:
			BatchIterator() {}

			BatchIterator(
				ViewType& view
			) :
				BaseIterator(view)
			{
			}

			BatchIterator(
				ViewType& view,
				//
				uint64_t archetypeIndex,
				uint64_t iterationIndex,
				// last iteration condition
				uint64_t archetypeIterationConditionIndex,
				uint64_t lastArchetypeIterationConditionIndex
			) :
				BaseIterator(view)
			{
				this->m_ArchetypeIndex = archetypeIndex;
				this->m_IterationIndex = iterationIndex;

				this->m_ArchetypeIterationConditionIndex = archetypeIterationConditionIndex;
				this->m_LastArchetypeIterationConditionIndex = lastArchetypeIterationConditionIndex;


				// seting archetype context:
				this->m_ArchetypeContext = &this->view.m_ArchetypesContexts[this->m_ArchetypeIndex];
				this->m_CurrentArchetype = this->m_ArchetypeContext->Arch;
				this->componentsRefs = this->m_ArchetypeContext->Arch->m_ComponentsRefs.data();

				if (this->m_ArchetypeIndex == m_ArchetypeIterationConditionIndex)
				{
					m_CurrentIterationConditionIndex = m_LastArchetypeIterationConditionIndex;
				}
				else
				{
					m_CurrentIterationConditionIndex = this->m_ArchetypeContext->EntitiesCount();
				}

				this->m_IsValid =
					this->m_ArchetypeIndex <= m_LastArchetypeIterationConditionIndex &&
					this->m_IterationIndex < m_CurrentIterationConditionIndex;

				if (this->m_IsValid)
					this->PrepareTuple();
			}

			~BatchIterator() {}

			virtual void Advance() override
			{
				this->m_IterationIndex++;

				while (this->m_IterationIndex < m_CurrentIterationConditionIndex)
				{
					if (this->m_CurrentArchetype->m_EntitiesData[this->m_IterationIndex].IsActive)
					{
						this->PrepareTuple();
						return;
					}
					this->m_IterationIndex++;
				}

				this->m_ArchetypeIndex += 1;

				for (; this->m_ArchetypeIndex <= m_ArchetypeIterationConditionIndex; this->m_ArchetypeIndex++)
				{
					this->SetCurrentArchetypeContext(&this->m_View->m_ArchetypesContexts[this->m_ArchetypeIndex]);

					if (this->m_ArchetypeContext->EntitiesCount() > 0)
					{
						while (this->m_IterationIndex < m_CurrentIterationConditionIndex)
						{
							if (this->m_CurrentArchetype->m_EntitiesData[this->m_IterationIndex].IsActive)
							{
								this->PrepareTuple();
								return;
							}
							this->m_IterationIndex++;
						}
					}
				}


				this->m_IsValid = false;
				this->InvalidateTuple();
				return;
			}

		protected:
			virtual void SetCurrentArchetypeContext(ArchetypeContext* archetypeContext) override
			{
				this->m_ArchetypeContext = archetypeContext;
				this->m_CurrentArchetype = this->m_ArchetypeContext->Arch;
				this->componentsRefs = this->m_ArchetypeContext->Arch->m_ComponentsRefs.data();

				this->m_IterationIndex = 0;
				if (this->m_ArchetypeIndex == m_ArchetypeIterationConditionIndex)
				{
					m_CurrentIterationConditionIndex = m_LastArchetypeIterationConditionIndex;
				}
				else
				{
					m_CurrentIterationConditionIndex = this->m_ArchetypeContext->EntitiesCount();
				}
			}

		private:
			uint64_t m_ArchetypeIterationConditionIndex = 0;
			uint64_t m_LastArchetypeIterationConditionIndex = 0;
			uint64_t m_CurrentIterationConditionIndex = 0;
		};


	public:
		inline Iterator GetIterator()
		{
			for (ArchetypeContext& archContext : m_ArchetypesContexts)
				archContext.ValidateEntitiesCount();
			return Iterator(*this);
		}

		void CreateBatchIterators(
			std::vector<BatchIterator>& iterators,
			const uint64_t& desiredBatchesCount,
			const uint64_t& minBatchSize
		)
		{
			uint64_t entitiesCount = 0;
			for (ArchetypeContext& archContext : m_ArchetypesContexts)
			{
				archContext.ValidateEntitiesCount();
				entitiesCount += archContext.EntitiesCount();
			}

			uint64_t realDesiredBatchSize = std::ceil((float)entitiesCount / (float)desiredBatchesCount);
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
			uint64_t itStartArchetypeIndex;
			uint64_t itStartIterationIndex;
			// last iteration condition
			uint64_t itLastArchetypeIndex;
			uint64_t itLastArchetypeMaxIterationIndex;

			uint64_t itNeededEntitiesCount;

			while (entitiesCount > 0)
			{
				itStartArchetypeIndex = currentArchetypeIndex;
				itStartIterationIndex = currentArchEntitiesStartIndex;
				itNeededEntitiesCount = finalBatchSize;

				for (; currentArchetypeIndex < archsCount;)
				{
					ArchetypeContext& ctx = m_ArchetypesContexts[currentArchetypeIndex];

					uint64_t archAvailableEntities = ctx.EntitiesCount() - currentArchEntitiesStartIndex;

					if (itNeededEntitiesCount < archAvailableEntities)
					{
						currentArchEntitiesStartIndex += itNeededEntitiesCount;
						itNeededEntitiesCount = 0;
						itLastArchetypeIndex = currentArchetypeIndex;
					}
					else
					{
						itNeededEntitiesCount -= archAvailableEntities;
						currentArchEntitiesStartIndex += archAvailableEntities;
						itLastArchetypeIndex = currentArchetypeIndex;


						currentArchetypeIndex += 1;
					}

					if (itNeededEntitiesCount == 0)
					{
						break;
					}
				}

				itLastArchetypeMaxIterationIndex = currentArchEntitiesStartIndex;

				BatchIterator& it = iterators.emplace_back(
					*this,
					itStartArchetypeIndex,
					itStartIterationIndex,
					itLastArchetypeIndex,
					itLastArchetypeMaxIterationIndex
				);

				entitiesCount -= finalBatchSize - itNeededEntitiesCount;
			}
		}
	};
	
	template<typename... ComponentsTypes>
	class ComplexView<Entity, ComponentsTypes...>
	{
		using ArchetypeContext = ViewArchetypeContext<sizeof...(ComponentsTypes)>;

	public:
		ComplexView()
		{

		}

		~ComplexView()
		{

		}

		template<typename... Excludes>
		ComplexView& WithoutAnyOf()
		{
			m_IsDirty = true;
			m_Excludes.clear();
			findIds<Excludes...>(m_Excludes);
			return *this;
		}

		template<typename... Excludes>
		ComplexView& WithAnyOf()
		{
			m_IsDirty = true;
			m_RequiredAny.clear();
			findIds<Excludes...>(m_RequiredAny);
			return *this;
		}

		template<typename... Excludes>
		ComplexView& WithAll()
		{
			m_IsDirty = true;
			m_RequiredAll.clear();
			findIds<Excludes...>(m_RequiredAll);
			return *this;
		}

		void Fetch(Container& container)
		{
			if (m_IsDirty || &container != m_Container)
			{
				m_IsDirty = false;
				m_Container = &container;
				Invalidate();
			}

			uint64_t containerArchetypesCount = m_Container->m_ArchetypesMap.m_Archetypes.size();
			if (m_ArchetypesCount_Dirty != containerArchetypesCount)
			{
				uint64_t newArchetypesCount = containerArchetypesCount - m_ArchetypesCount_Dirty;

				uint64_t minComponentsCountInArchetype = GetMinComponentsCount();
				if (m_RequiredAny.size() > 0) minComponentsCountInArchetype += 1;

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
		void ForEach(Callable func)
		{
			for (ArchetypeContext& archContext : m_ArchetypesContexts)
				archContext.ValidateEntitiesCount();

			uint32_t contextCount = m_ArchetypesContexts.size();
			std::tuple<Entity, ComponentsTypes*...> tuple = {};
			for (uint32_t contextIndex = 0; contextIndex < contextCount; contextIndex++)
			{
				ArchetypeContext& ctx = m_ArchetypesContexts[contextIndex];
				TypeID* typeIndexes = ctx.TypeIndexes;
				uint64_t componentsCount = ctx.Arch->GetComponentsCount();

				ArchetypeEntityData* entitiesData = ctx.Arch->m_EntitiesData.data();
				ComponentRef* componentsRefs = ctx.Arch->m_ComponentsRefs.data();

				for (int64_t iterationIndex = ctx.EntitiesCount() - 1; iterationIndex > -1; iterationIndex--)
				{
					auto& entityData = entitiesData[iterationIndex];
					if (entityData.IsActive)
					{
						std::get<Entity>(tuple) = Entity(entityData.ID, m_Container);
						PrepareTuple(
							tuple,
							iterationIndex,
							typeIndexes,
							componentsCount,
							entitiesData,
							componentsRefs
						);

						std::apply(func, tuple);
					}
				}
			}
		}

	private:
		TypeGroup<ComponentsTypes...> m_Includes = {};
		std::vector<TypeID> m_Excludes;
		std::vector<TypeID> m_RequiredAny;
		std::vector<TypeID> m_RequiredAll;

		Container* m_Container;
		bool m_IsDirty;

		std::vector<ArchetypeContext> m_ArchetypesContexts;
		ecsSet<Archetype*> m_ContainedArchetypes;

		// cache value to check if view should be updated:
		uint32_t m_ArchetypesCount_Dirty = 0;

	private:

		inline uint64_t GetMinComponentsCount() const
		{
			uint64_t includesCount = sizeof...(ComponentsTypes);

			return sizeof...(ComponentsTypes) + m_RequiredAll.size();
		}

		void Invalidate()
		{
			m_ArchetypesContexts.clear();
			m_ContainedArchetypes.clear();
			m_ArchetypesCount_Dirty = std::numeric_limits<uint32_t>::max();
		}

		inline bool ContainArchetype(Archetype* arch) { return m_ContainedArchetypes.find(arch) != m_ContainedArchetypes.end(); }

		virtual bool TryAddArchetype(Archetype& archetype, const bool& tryAddNeighbours)
		{
			if (!ContainArchetype(&archetype) && archetype.GetComponentsCount())
			{
				// exclude test
				{
					uint64_t excludeCount = m_Excludes.size();
					for (int i = 0; i < excludeCount; i++)
					{
						if (archetype.ContainType(m_Excludes[i]))
						{
							return false;
						}
					}
				}

				// required any
				{
					uint64_t requiredAnyCount = m_RequiredAny.size();
					bool containRequiredAny = requiredAnyCount == 0;

					for (int i = 0; i < requiredAnyCount; i++)
					{
						if (archetype.ContainType(m_RequiredAny[i]))
						{
							containRequiredAny = true;
							break;
						}
					}
					if (!containRequiredAny) return false;
				}

				// required all
				{
					uint64_t requiredAllCount = m_RequiredAll.size();

					for (int i = 0; i < requiredAllCount; i++)
					{
						if (!archetype.ContainType(m_RequiredAll[i]))
						{
							return false;
						}
					}
				}


				// includes
				{
					ArchetypeContext& context = m_ArchetypesContexts.emplace_back();

					for (uint32_t typeIdx = 0; typeIdx < m_Includes.Size(); typeIdx++)
					{
						auto typeID_It = archetype.m_TypeIDsIndexes.find(m_Includes.IDs()[typeIdx]);
						if (typeID_It == archetype.m_TypeIDsIndexes.end())
						{
							m_ArchetypesContexts.pop_back();
							return false;
						}
						context.TypeIndexes[typeIdx] = typeID_It->second;
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
			uint32_t addedArchetypes = 0;
			uint64_t archetypesVectorIndex = minComponentsCountInArchetype - 1;

			while (addedArchetypes == 0 && archetypesVectorIndex < maxComponentsInArchetype)
			{
				std::vector<Archetype*>& archetypesToTest = map.m_ArchetypesGroupedByComponentsCount[archetypesVectorIndex];

				uint32_t archsCount = archetypesToTest.size();
				for (uint32_t i = 0; i < archetypesToTest.size(); i++)
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
					uint64_t excludeCount = m_Excludes.size();
					for (int i = 0; i < excludeCount; i++)
					{
						if (archetype.ContainType(m_Excludes[i]))
						{
							return false;
						}
					}
				}

				// required any
				{
					uint64_t requiredAnyCount = m_RequiredAny.size();
					bool containRequiredAny = requiredAnyCount == 0;

					for (int i = 0; i < requiredAnyCount; i++)
					{
						if (archetype.ContainType(m_RequiredAny[i]))
						{
							containRequiredAny = true;
							break;
						}
					}
					if (!containRequiredAny) return false;
				}

				// required all
				{
					uint64_t requiredAllCount = m_RequiredAll.size();

					for (int i = 0; i < requiredAllCount; i++)
					{
						if (!archetype.ContainType(m_RequiredAll[i]))
						{
							return false;
						}
					}
				}

				// includes
				{
					ArchetypeContext& context = m_ArchetypesContexts.emplace_back();

					for (uint32_t typeIdx = 0; typeIdx < m_Includes.Size(); typeIdx++)
					{
						auto typeID_It = archetype.m_TypeIDsIndexes.find(m_Includes.IDs()[typeIdx]);
						if (typeID_It == archetype.m_TypeIDsIndexes.end())
						{
							m_ArchetypesContexts.pop_back();
							return false;
						}
						context.TypeIndexes[typeIdx] = typeID_It->second;
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
			Archetype** archetypes = map.m_Archetypes.data();
			uint64_t archetypesCount = map.m_Archetypes.size();
			for (uint64_t i = startArchetypesIndex; i < archetypesCount; i++)
			{
				TryAddArchetypeWithoutNeighbours(*archetypes[i], minComponentsCountInArchetype);
			}
		}
	private:
		inline void PrepareTuple(
			std::tuple<Entity, ComponentsTypes*...>& tuple,
			const uint64_t& iterationIndex,
			TypeID*& typesIndexe,
			const uint64_t& componentsCount,
			ArchetypeEntityData*& entitiesData,
			ComponentRef*& componentsRefs
		)
		{
			SetTupleElements<ComponentsTypes...>(
				tuple,
				0,
				iterationIndex * componentsCount,
				typesIndexe,
				componentsRefs
				);
		}

		template<typename T = void, typename... Ts>
		void SetTupleElements(
			std::tuple<Entity, ComponentsTypes*...>& tuple,
			const uint64_t& compIndex,
			const uint64_t& firstEntityCompIndex,
			TypeID*& typesIndexe,
			ComponentRef*& componentsRefs
		)
		{
			uint64_t compIndexInArchetype = typesIndexe[compIndex];
			auto& compRef = componentsRefs[firstEntityCompIndex + compIndexInArchetype];
			std::get<T*>(tuple) = reinterpret_cast<T*>(compRef.ComponentPointer);

			if (sizeof...(Ts) == 0) return;

			SetTupleElements<Ts...>(
				tuple,
				compIndex + 1,
				firstEntityCompIndex,
				typesIndexe,
				componentsRefs
				);
		}

		template<>
		void SetTupleElements<void>(
			std::tuple<Entity, ComponentsTypes*...>& tuple,
			const uint64_t& compIndex,
			const uint64_t& firstEntityCompIndex,
			TypeID*& typesIndexe,
			ComponentRef*& componentsRefs
			)
		{

		}

	private:
		struct BaseIterator
		{
			using ViewType = ComplexView <Entity, ComponentsTypes...>;
			using DataTuple = std::tuple<Entity, ComponentsTypes*...>;
		public:
			BaseIterator() {}

			BaseIterator(ViewType& view) :
				m_View(&view),
				m_Container(view.m_Container)
			{

			}

			~BaseIterator()
			{

			}

			inline bool IsValid() { return m_IsValid; }

			inline DataTuple& GetTuple()
			{
				return m_DataTuple;
			}
			virtual void Advance() = 0;

		protected:
			DataTuple m_DataTuple;

			ViewType* m_View = nullptr;
			Container* m_Container = nullptr;

			ArchetypeContext* m_ArchetypeContext = nullptr;
			Archetype* m_CurrentArchetype = nullptr;
			ComponentRef* componentsRefs = nullptr;

			uint64_t m_ArchetypeIndex = 0;
			int64_t m_IterationIndex = 0;

			bool m_IsValid = false;

		protected:
			virtual void SetCurrentArchetypeContext(ArchetypeContext* archetypeContext)
			{
				m_ArchetypeContext = archetypeContext;
				m_CurrentArchetype = m_ArchetypeContext->Arch;
				componentsRefs = m_ArchetypeContext->Arch->m_ComponentsRefs.data();
				m_IterationIndex = m_ArchetypeContext->EntitiesCount() - 1;
			}

			inline void PrepareTuple()
			{
				std::get<Entity>(m_DataTuple) = Entity(m_CurrentArchetype->m_EntitiesData[m_IterationIndex].ID, m_Container);
				SetTupleElements<ComponentsTypes...>(0, m_IterationIndex * m_CurrentArchetype->GetComponentsCount());
			}

			template<typename T = void, typename... Ts>
			void SetTupleElements(const uint64_t& compIndex, const uint64_t& firstEntityCompIndex)
			{
				uint64_t compIndexInArchetype = m_ArchetypeContext->TypeIndexes[compIndex];
				auto& compRef = componentsRefs[firstEntityCompIndex + compIndexInArchetype];
				std::get<T*>(m_DataTuple) = reinterpret_cast<T*>(compRef.ComponentPointer);

				if (sizeof...(Ts) == 0) return;

				SetTupleElements<Ts...>(compIndex + 1, firstEntityCompIndex);
			}

			template<>
			void SetTupleElements<void>(const uint64_t& compIndex, const uint64_t& firstEntityCompIndex)
			{

			}

			void InvalidateTuple()
			{
				InvalidateTupleElements<ComponentsTypes...>();
			}

			template<typename T = void, typename... Ts>
			void InvalidateTupleElements()
			{
				std::get<T*>(m_DataTuple) = nullptr;

				if (sizeof...(Ts) == 0) return;

				std::get<Entity>(m_DataTuple) = Entity(decs::Null::ID, nullptr);
				InvalidateTupleElements<Ts...>();
			}

			template<>
			void InvalidateTupleElements<void>()
			{

			}

		};

	public:
		struct Iterator : public BaseIterator
		{
			using ViewType = ComplexView <Entity, ComponentsTypes...>;
		public:
			Iterator() {}

			Iterator(ViewType& view) :
				BaseIterator(view)
			{
				if (this->m_View->m_ArchetypesContexts.size() == 0)
				{
					this->m_IsValid = false;
					this->InvalidateTuple();
				}
				else
				{
					this->m_IsValid = false;
					uint64_t archetypsCount = this->m_View->m_ArchetypesContexts.size();
					this->m_ArchetypeIndex = 0;
					for (; this->m_ArchetypeIndex < archetypsCount; this->m_ArchetypeIndex++)
					{
						this->SetCurrentArchetypeContext(&this->m_View->m_ArchetypesContexts[this->m_ArchetypeIndex]);

						if (this->m_ArchetypeContext->EntitiesCount() > 0)
						{
							while (this->m_IterationIndex >= 0)
							{
								if (this->m_ArchetypeContext->Arch->m_EntitiesData[this->m_IterationIndex].IsActive)
								{
									this->m_IsValid = true;
									this->PrepareTuple();
									return;
								}
								this->m_IterationIndex -= 1;
							}
						}
					}
				}

				this->m_IsValid = false;
				this->InvalidateTuple();
			}

			~Iterator()
			{
			}

			virtual void Advance() override
			{
				this->m_IterationIndex -= 1;

				while (this->m_IterationIndex >= 0)
				{
					if (this->m_CurrentArchetype->m_EntitiesData[this->m_IterationIndex].IsActive)
					{
						this->PrepareTuple();
						return;
					}
					this->m_IterationIndex -= 1;
				}

				this->m_ArchetypeIndex += 1;

				for (; this->m_ArchetypeIndex < this->m_View->m_ArchetypesContexts.size(); this->m_ArchetypeIndex++)
				{
					this->SetCurrentArchetypeContext(&this->m_View->m_ArchetypesContexts[this->m_ArchetypeIndex]);

					if (this->m_ArchetypeContext->EntitiesCount() > 0)
					{
						while (this->m_IterationIndex >= 0)
						{
							if (this->m_CurrentArchetype->m_EntitiesData[this->m_IterationIndex].IsActive)
							{
								this->PrepareTuple();
								return;
							}
							this->m_IterationIndex -= 1;
						}
					}
				}

				this->m_IsValid = false;
				this->InvalidateTuple();
				return;
			}

		};

		class BatchIterator : public BaseIterator
		{
			using ViewType = ComplexView <Entity, ComponentsTypes...>;
		public:
			BatchIterator() {}

			BatchIterator(
				ViewType& view
			) :
				BaseIterator(view)
			{
			}

			BatchIterator(
				ViewType& view,
				//
				uint64_t archetypeIndex,
				uint64_t iterationIndex,
				// last iteration condition
				uint64_t archetypeIterationConditionIndex,
				uint64_t lastArchetypeIterationConditionIndex
			) :
				BaseIterator(view)
			{
				this->m_ArchetypeIndex = archetypeIndex;
				this->m_IterationIndex = iterationIndex;

				this->m_ArchetypeIterationConditionIndex = archetypeIterationConditionIndex;
				this->m_LastArchetypeIterationConditionIndex = lastArchetypeIterationConditionIndex;


				// seting archetype context:
				this->m_ArchetypeContext = &view.m_ArchetypesContexts[this->m_ArchetypeIndex];
				this->m_CurrentArchetype = this->m_ArchetypeContext->Arch;
				this->componentsRefs = this->m_ArchetypeContext->Arch->m_ComponentsRefs.data();

				if (this->m_ArchetypeIndex == m_ArchetypeIterationConditionIndex)
				{
					m_CurrentIterationConditionIndex = m_LastArchetypeIterationConditionIndex;
				}
				else
				{
					m_CurrentIterationConditionIndex = this->m_ArchetypeContext->EntitiesCount();
				}

				this->m_IsValid =
					this->m_ArchetypeIndex <= m_LastArchetypeIterationConditionIndex &&
					this->m_IterationIndex < m_CurrentIterationConditionIndex;

				if (this->m_IsValid)
					this->PrepareTuple();
			}

			~BatchIterator() {}

			virtual void Advance() override
			{
				this->m_IterationIndex++;

				while (this->m_IterationIndex < m_CurrentIterationConditionIndex)
				{
					if (this->m_CurrentArchetype->m_EntitiesData[this->m_IterationIndex].IsActive)
					{
						this->PrepareTuple();
						return;
					}
					this->m_IterationIndex++;
				}

				this->m_ArchetypeIndex += 1;

				for (; this->m_ArchetypeIndex <= m_ArchetypeIterationConditionIndex; this->m_ArchetypeIndex++)
				{
					this->SetCurrentArchetypeContext(&this->m_View->m_ArchetypesContexts[this->m_ArchetypeIndex]);

					if (this->m_ArchetypeContext->EntitiesCount() > 0)
					{
						while (this->m_IterationIndex < m_CurrentIterationConditionIndex)
						{
							if (this->m_CurrentArchetype->m_EntitiesData[this->m_IterationIndex].IsActive)
							{
								this->PrepareTuple();
								return;
							}
							this->m_IterationIndex++;
						}
					}
				}


				this->m_IsValid = false;
				this->InvalidateTuple();
				return;
			}

		protected:
			virtual void SetCurrentArchetypeContext(ArchetypeContext* archetypeContext) override
			{
				this->m_ArchetypeContext = archetypeContext;
				this->m_CurrentArchetype = this->m_ArchetypeContext->Arch;
				this->componentsRefs = this->m_ArchetypeContext->Arch->m_ComponentsRefs.data();

				this->m_IterationIndex = 0;
				if (this->m_ArchetypeIndex == m_ArchetypeIterationConditionIndex)
				{
					m_CurrentIterationConditionIndex = m_LastArchetypeIterationConditionIndex;
				}
				else
				{
					m_CurrentIterationConditionIndex = this->m_ArchetypeContext->EntitiesCount();
				}
			}

		private:
			uint64_t m_ArchetypeIterationConditionIndex = 0;
			uint64_t m_LastArchetypeIterationConditionIndex = 0;
			uint64_t m_CurrentIterationConditionIndex = 0;
		};


	public:
		inline Iterator GetIterator()
		{
			for (ArchetypeContext& archContext : m_ArchetypesContexts)
				archContext.ValidateEntitiesCount();
			return Iterator(*this);
		}

		void CreateBatchIterators(
			std::vector<BatchIterator>& iterators,
			const uint64_t& desiredBatchesCount,
			const uint64_t& minBatchSize
		)
		{
			uint64_t entitiesCount = 0;
			for (ArchetypeContext& archContext : m_ArchetypesContexts)
			{
				archContext.ValidateEntitiesCount();
				entitiesCount += archContext.EntitiesCount();
			}

			uint64_t realDesiredBatchSize = std::ceil((float)entitiesCount / (float)desiredBatchesCount);
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
			uint64_t itStartArchetypeIndex;
			uint64_t itStartIterationIndex;
			// last iteration condition
			uint64_t itLastArchetypeIndex;
			uint64_t itLastArchetypeMaxIterationIndex;

			uint64_t itNeededEntitiesCount;

			while (entitiesCount > 0)
			{
				itStartArchetypeIndex = currentArchetypeIndex;
				itStartIterationIndex = currentArchEntitiesStartIndex;
				itNeededEntitiesCount = finalBatchSize;

				for (; currentArchetypeIndex < archsCount;)
				{
					ArchetypeContext& ctx = m_ArchetypesContexts[currentArchetypeIndex];

					uint64_t archAvailableEntities = ctx.EntitiesCount() - currentArchEntitiesStartIndex;

					if (itNeededEntitiesCount < archAvailableEntities)
					{
						currentArchEntitiesStartIndex += itNeededEntitiesCount;
						itNeededEntitiesCount = 0;
						itLastArchetypeIndex = currentArchetypeIndex;
					}
					else
					{
						itNeededEntitiesCount -= archAvailableEntities;
						currentArchEntitiesStartIndex += archAvailableEntities;
						itLastArchetypeIndex = currentArchetypeIndex;


						currentArchetypeIndex += 1;
					}

					if (itNeededEntitiesCount == 0)
					{
						break;
					}
				}

				itLastArchetypeMaxIterationIndex = currentArchEntitiesStartIndex;

				BatchIterator& it = iterators.emplace_back(
					*this,
					itStartArchetypeIndex,
					itStartIterationIndex,
					itLastArchetypeIndex,
					itLastArchetypeMaxIterationIndex
				);

				entitiesCount -= finalBatchSize - itNeededEntitiesCount;
			}
		}
	};
}