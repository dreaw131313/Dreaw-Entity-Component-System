#pragma once
#include "decspch.h"

#include "ViewCore.h"

#include "../Type.h"
#include "../TypeGroup.h"
#include "../core.h"
#include "../ComponentContainer.h"
#include "../Entity.h"
#include "../Container.h"
#include "decs/CustomApplay.h"


namespace decs
{
	template<typename... ComponentsTypes>
	class View
	{
		using ArchetypeContext = ViewArchetypeContext<sizeof...(ComponentsTypes)>;

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

		template<typename Callable>
		void ForEach(Callable func)
		{
			if (m_Container != nullptr)
			{
				Fetch(*m_Container);
			}

			for (ArchetypeContext& archContext : m_ArchetypesContexts)
				archContext.ValidateEntitiesCount();

			uint64_t contextCount = m_ArchetypesContexts.size();
			std::tuple<ComponentsTypes*...> tuple = {};
			for (uint64_t contextIndex = 0; contextIndex < contextCount; contextIndex++)
			{
				ArchetypeContext& ctx = m_ArchetypesContexts[contextIndex];
				TypeID* typeIndexes = ctx.TypeIndexes;
				uint64_t componentsCount = ctx.Arch->GetComponentsCount();

				ArchetypeEntityData* entitiesData = ctx.Arch->m_EntitiesData.data();
				ComponentRef* componentsRefs = ctx.Arch->m_ComponentsRefs.data();

				for (int64_t iterationIndex = ctx.EntitiesCount() - 1; iterationIndex > -1; iterationIndex--)
				{
					auto& entityData = entitiesData[iterationIndex];
					if (entityData.IsActive())
					{
						SetTupleElements<ComponentsTypes...>(
							tuple,
							0,
							iterationIndex * componentsCount,
							typeIndexes,
							componentsRefs
							);
						decs::ApplayTupleWithPointersAsRefrences(func, tuple);
					}
				}
			}
		}

		template<typename Callable>
		void ForEachWithEntity(Callable func)
		{
			if (m_Container != nullptr)
			{
				Fetch(*m_Container);
			}

			for (ArchetypeContext& archContext : m_ArchetypesContexts)
				archContext.ValidateEntitiesCount();

			uint64_t contextCount = m_ArchetypesContexts.size();
			std::tuple<Entity*, ComponentsTypes*...> tuple = {};
			for (uint64_t contextIndex = 0; contextIndex < contextCount; contextIndex++)
			{
				ArchetypeContext& ctx = m_ArchetypesContexts[contextIndex];
				TypeID* typeIndexes = ctx.TypeIndexes;
				uint64_t componentsCount = ctx.Arch->GetComponentsCount();

				ArchetypeEntityData* entitiesData = ctx.Arch->m_EntitiesData.data();
				ComponentRef* componentsRefs = ctx.Arch->m_ComponentsRefs.data();

				for (int64_t iterationIndex = ctx.EntitiesCount() - 1; iterationIndex > -1; iterationIndex--)
				{
					auto& entityData = entitiesData[iterationIndex];
					std::get<Entity*>(tuple) = entityData.EntityPtr();
					if (entityData.IsActive())
					{
						SetWithEntityTupleElements<ComponentsTypes...>(
							tuple,
							0,
							iterationIndex * componentsCount,
							typeIndexes,
							componentsRefs
							);
						decs::ApplayTupleWithPointersAsRefrences(func, tuple);
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

		virtual bool TryAddArchetype(Archetype& archetype, const bool& tryAddNeighbours)
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
			std::tuple<Entity*, ComponentsTypes*...>& tuple,
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
			std::tuple<Entity*, ComponentsTypes*...>& tuple,
			const uint64_t& compIndex,
			const uint64_t& firstEntityCompIndex,
			TypeID*& typesIndexe,
			ComponentRef*& componentsRefs
			)
		{

		}

	private:
#pragma region Iterator base class
		template<typename TupleClass, typename... ComponentsTypes>
		class ViewBasicIterator
		{
			using ViewType = View<ComponentsTypes...>;
			using ArchetypeContext = ViewArchetypeContext<sizeof...(ComponentsTypes)>;

			template<typename... Types>
			friend class View;
		public:
			ViewBasicIterator() {}

			ViewBasicIterator(ViewType& view) :
				m_View(&view),
				m_Container(view.m_Container)
			{
			}

			~ViewBasicIterator() {}

			inline bool IsValid() { return m_IsValid; }

			inline TupleClass& GetTuple()
			{
				return m_DataTuple;
			}
			virtual void Advance() = 0;

		protected:
			TupleClass m_DataTuple;

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

			virtual void PrepareTuple() = 0;

			virtual void InvalidateTuple() = 0;
		};
#pragma endregion

	public:
#pragma region View Iterator
		template<typename... ComponentsTypes>
		class ViewIterator : public ViewBasicIterator<std::tuple<ComponentsTypes*...>, ComponentsTypes...>
		{
			using ViewType = View<ComponentsTypes...>;
			using ArchetypeContext = ViewArchetypeContext<sizeof...(ComponentsTypes)>;

			template<typename... Types>
			friend class View;
		public:
			ViewIterator() {}

			ViewIterator(ViewType& view) :
				ViewBasicIterator(view)
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

			~ViewIterator()
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
		protected:
			virtual void PrepareTuple() override
			{
				this->SetTupleElements<ComponentsTypes...>(0, this->m_IterationIndex * this->m_CurrentArchetype->GetComponentsCount());
			}

			void InvalidateTuple() override
			{
				this->InvalidateTupleElements<ComponentsTypes...>();
			}

		};

#pragma endregion

#pragma region View Iterator with entity
		template<typename... ComponentsTypes>
		class ViewIterator<Entity, ComponentsTypes...> : public ViewBasicIterator<std::tuple<Entity, ComponentsTypes*...>, ComponentsTypes...>
		{
			using ViewType = View<ComponentsTypes...>;
			using ArchetypeContext = ViewArchetypeContext<sizeof...(ComponentsTypes)>;

			template<typename... Types>
			friend class View;
		public:
			ViewIterator() {}

			ViewIterator(ViewType& view) :
				ViewBasicIterator(view)
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

			~ViewIterator()
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

		protected:
			virtual void PrepareTuple() override
			{
				std::get<Entity>(this->m_DataTuple) = Entity(this->m_CurrentArchetype->m_EntitiesData[this->m_IterationIndex].ID, this->m_Container);
				this->SetTupleElements<ComponentsTypes...>(0, this->m_IterationIndex * this->m_CurrentArchetype->GetComponentsCount());
			}

			void InvalidateTuple() override
			{
				std::get<Entity>(this->m_DataTuple) = Entity();
				this->InvalidateTupleElements<ComponentsTypes...>();
			}

		};

#pragma endregion

#pragma region Batch Iterator

		template<typename... ComponentsTypes>
		class ViewBatchIterator : public ViewBasicIterator<std::tuple<ComponentsTypes*...>, ComponentsTypes...>
		{
			using ViewType = View<ComponentsTypes...>;
			using ArchetypeContext = ViewArchetypeContext<sizeof...(ComponentsTypes)>;

			template<typename... Types>
			friend class View;
		public:
			ViewBatchIterator() {}

			ViewBatchIterator(
				ViewType& view
			) :
				ViewBasicIterator(view)
			{
			}

			ViewBatchIterator(
				ViewType& view,
				//
				uint64_t archetypeIndex,
				uint64_t iterationIndex,
				// last iteration condition
				uint64_t archetypeIterationConditionIndex,
				uint64_t lastArchetypeIterationConditionIndex
			) :
				ViewBasicIterator(view)
			{
				this->m_ArchetypeIndex = archetypeIndex;
				this->m_IterationIndex = iterationIndex;

				this->m_ArchetypeIterationConditionIndex = archetypeIterationConditionIndex;
				this->m_LastArchetypeIterationConditionIndex = lastArchetypeIterationConditionIndex;


				// seting archetype context:
				this->m_ArchetypeContext = &this->m_View->m_ArchetypesContexts[this->m_ArchetypeIndex];
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

			~ViewBatchIterator() {}

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


			virtual void PrepareTuple() override
			{
				this->SetTupleElements<ComponentsTypes...>(0, this->m_IterationIndex * this->m_CurrentArchetype->GetComponentsCount());
			}

			void InvalidateTuple() override
			{
				this->InvalidateTupleElements<ComponentsTypes...>();
			}

		private:
			uint64_t m_ArchetypeIterationConditionIndex = 0;
			uint64_t m_LastArchetypeIterationConditionIndex = 0;
			uint64_t m_CurrentIterationConditionIndex = 0;
		};

#pragma endregion

#pragma region Batch Iterator with entity

		template<typename... ComponentsTypes>
		class ViewBatchIterator<Entity, ComponentsTypes...> : public ViewBasicIterator<std::tuple<Entity, ComponentsTypes*...>, ComponentsTypes...>
		{
			using ViewType = View<ComponentsTypes...>;
			using ArchetypeContext = ViewArchetypeContext<sizeof...(ComponentsTypes)>;

			template<typename... Types>
			friend class View;
		public:
			ViewBatchIterator() {}

			ViewBatchIterator(
				ViewType& view
			) :
				ViewBasicIterator(view)
			{
			}

			ViewBatchIterator(
				ViewType& view,
				//
				uint64_t archetypeIndex,
				uint64_t iterationIndex,
				// last iteration condition
				uint64_t archetypeIterationConditionIndex,
				uint64_t lastArchetypeIterationConditionIndex
			) :
				ViewBasicIterator(view)
			{
				this->m_ArchetypeIndex = archetypeIndex;
				this->m_IterationIndex = iterationIndex;

				this->m_ArchetypeIterationConditionIndex = archetypeIterationConditionIndex;
				this->m_LastArchetypeIterationConditionIndex = lastArchetypeIterationConditionIndex;


				// seting archetype context:
				this->m_ArchetypeContext = &this->m_View->m_ArchetypesContexts[this->m_ArchetypeIndex];
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

			~ViewBatchIterator() {}

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


			virtual void PrepareTuple() override
			{
				std::get<Entity>(this->m_DataTuple) = Entity(this->m_CurrentArchetype->m_EntitiesData[this->m_IterationIndex].ID, this->m_Container);
				this->SetTupleElements<ComponentsTypes...>(0, this->m_IterationIndex * this->m_CurrentArchetype->GetComponentsCount());
			}

			void InvalidateTuple() override
			{
				std::get<Entity>(this->m_DataTuple) = Entity();
				this->InvalidateTupleElements<ComponentsTypes...>();
			}

		private:
			uint64_t m_ArchetypeIterationConditionIndex = 0;
			uint64_t m_LastArchetypeIterationConditionIndex = 0;
			uint64_t m_CurrentIterationConditionIndex = 0;
		};

#pragma endregion

	public:
		using Iterator = ViewIterator<ComponentsTypes...>;
		using IteratorWithEntity = ViewIterator<Entity, ComponentsTypes...>;
		using BatchIterator = ViewBatchIterator<ComponentsTypes...>;
		using BatchIteratorWithEntity = ViewBatchIterator<Entity, ComponentsTypes...>;

		inline Iterator GetIterator()
		{
			for (ArchetypeContext& archContext : m_ArchetypesContexts)
				archContext.ValidateEntitiesCount();
			return Iterator(*this);
		}

		inline IteratorWithEntity GetIteratorWithEntity()
		{
			for (ArchetypeContext& archContext : m_ArchetypesContexts)
				archContext.ValidateEntitiesCount();
			return IteratorWithEntity(*this);
		}

	private:

		template<typename BatchIteratorType>
		void CreateBatchIterators_Template(
			std::vector<BatchIteratorType>& iterators,
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

				BatchIteratorType& it = iterators.emplace_back(
					*this,
					itStartArchetypeIndex,
					itStartIterationIndex,
					itLastArchetypeIndex,
					itLastArchetypeMaxIterationIndex
				);

				entitiesCount -= finalBatchSize - itNeededEntitiesCount;
			}
		}

	public:
		inline void CreateBatchIterators(
			std::vector<BatchIterator>& iterators,
			const uint64_t& desiredBatchesCount,
			const uint64_t& minBatchSize
		)
		{
			CreateBatchIterators_Template<BatchIterator>(iterators, desiredBatchesCount, minBatchSize);
		}

		inline void CreateBatchIteratorsWithEntity(
			std::vector<BatchIteratorWithEntity>& iterators,
			const uint64_t& desiredBatchesCount,
			const uint64_t& minBatchSize
		)
		{
			CreateBatchIterators_Template<BatchIteratorWithEntity>(iterators, desiredBatchesCount, minBatchSize);
		}
	};
}