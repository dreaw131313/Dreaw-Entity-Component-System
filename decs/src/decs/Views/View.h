#pragma once
#include "decspch.h"

#include "ViewCore.h"

#include "../core.h"
#include "../ComponentContainer.h"
#include "../Entity.h"
#include "../Container.h"

#include <cmath>

namespace decs
{
	template<typename... ComponentsTypes>
	class ViewBase
	{
		using ArchetypeContext = ViewArchetypeContext<sizeof...(ComponentsTypes)>;

	protected:
		ViewBase() {}

		ViewBase(Container& container) :
			m_Container(container)
		{
		}

	public:

		uint64_t EntitiesCount()
		{
			uint64_t entitiesCount = 0;
			for (auto& archContext : m_ArchetypesContexts)
			{
				entitiesCount += archContext.EntitiesCount;
			}

			return entitiesCount;
		}

		void Fetch(Container& container)
		{
			if (this->m_Container != &container)
			{
				this->m_Container = &container;
				Invalidate();
			}

			if (m_ArchetypesCount_Dirty != m_Container->m_ArchetypesMap.m_Archetypes.size())
			{
				m_ArchetypesCount_Dirty = m_Container->m_ArchetypesMap.m_Archetypes.size();

				FindRootArchetype();
				if (m_RootArchetype != nullptr)
				{
					TryAddRootChildsArchetype();
				}
				else
				{
					FetchViewArchetypes();
				}
			}
		}

		template<typename Callable>
		void ForEach(Callable func)
		{
			uint32_t contextCount = m_ArchetypesContexts.size();
			for (uint32_t contextIndex = 0; contextIndex < contextCount; contextIndex++)
				m_ArchetypesContexts[contextIndex].ValidateEntitiesCount();

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

	protected:
		TypeGroup<ComponentsTypes...> m_IncludeTypesGroup = {};

		std::vector<ArchetypeContext> m_ArchetypesContexts;
		Archetype* m_RootArchetype = nullptr;
		ecsSet<Archetype*> m_ContainedArchetypes;
		Container* m_Container = nullptr;
		// cache value to check if view should be updated:
		uint32_t m_ArchetypesCount_Dirty = 0;

	protected:
		void Invalidate()
		{
			m_RootArchetype = nullptr;
			m_ArchetypesContexts.clear();
			m_ContainedArchetypes.clear();
			m_ArchetypesCount_Dirty = std::numeric_limits<uint32_t>::max();
		}

		inline bool ContainArchetype(Archetype* arch) { return m_ContainedArchetypes.find(arch) != m_ContainedArchetypes.end(); }

		virtual void FetchViewArchetypes()
		{
			if (m_RootArchetype == nullptr)
			{
				uint32_t testedArchetypesComponentsCount = sizeof...(ComponentsTypes);

				uint32_t addedRootArchetypes = 0;
				uint32_t maxComponentsCountInArchetypes = m_Container->m_ArchetypesMap.m_Archetypes.size();
				do
				{
					testedArchetypesComponentsCount += 1;
					if (testedArchetypesComponentsCount > maxComponentsCountInArchetypes)
					{
						break;
					}

					auto& testedArchetypesList = m_Container->m_ArchetypesMap.m_ArchetypesGroupedByComponentsCount[testedArchetypesComponentsCount - 1];

					uint32_t testedArchCount = testedArchetypesList.size();
					for (uint32_t archIdx = 0; archIdx < testedArchCount; archIdx++)
					{
						Archetype* testedArchetype = testedArchetypesList[archIdx];
						if (TryAddArchetype(*testedArchetype))
						{
							addedRootArchetypes += 1;
						}
					}

				} while (addedRootArchetypes == 0);
			}
		}

		void FindRootArchetype()
		{
			if (m_RootArchetype != nullptr) return;

			uint32_t rootArchetypeComponentsCount = sizeof...(ComponentsTypes);
			uint32_t maxComponentsCountInArchetypes = m_Container->m_ArchetypesMap.m_ArchetypesGroupedByComponentsCount.size();

			if (rootArchetypeComponentsCount > maxComponentsCountInArchetypes)
			{
				return;
			}
			else
			{
				auto& rootArchetypeList = m_Container->m_ArchetypesMap.m_ArchetypesGroupedByComponentsCount[rootArchetypeComponentsCount - 1];

				uint32_t archCount = rootArchetypeList.size();
				for (uint32_t archIdx = 0; archIdx < archCount; archIdx++)
				{
					Archetype* testedArchetype = rootArchetypeList[archIdx];

					if (TryAddRootArchetype(*testedArchetype))
					{
						m_RootArchetype = testedArchetype;
						break;
					}
				}
			}
		}

		void TryAddRootChildsArchetype()
		{
			for (auto& [key, edge] : m_RootArchetype->m_Edges)
			{
				if (edge.AddEdge != nullptr)
				{
					TryAddArchetype(*edge.AddEdge);
				}
			}
		}

		bool TryAddRootArchetype(Archetype& archetype)
		{
			auto it = m_ContainedArchetypes.find(&archetype);
			if (it == m_ContainedArchetypes.end())
			{
				// chcecing thaht archetype contains necesary components:
				ArchetypeContext& context = m_ArchetypesContexts.emplace_back();

				for (uint32_t typeIdx = 0; typeIdx < m_IncludeTypesGroup.Size(); typeIdx++)
				{
					auto typeID_It = archetype.m_TypeIDsIndexes.find(m_IncludeTypesGroup.IDs()[typeIdx]);
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

			return true;
		}

		virtual bool TryAddArchetype(Archetype& archetype) = 0;

	private:
		inline void PrepareTuple(
			std::tuple<ComponentsTypes*...>& tuple,
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

	private:
		///////////////
		// ITERATORS //
		///////////////

		struct BaseIterator
		{
			using ViewType = ViewBase <ComponentsTypes...>;
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
			using ViewType = ViewBase <ComponentsTypes...>;

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
			using ViewType = ViewBase<ComponentsTypes...>;
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
					SetCurrentArchetypeContext(&this->m_View->m_ArchetypesContexts[this->m_ArchetypeIndex]);

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
			uint32_t contextCount = m_ArchetypesContexts.size();
			for (uint32_t contextIndex = 0; contextIndex < contextCount; contextIndex++)
				m_ArchetypesContexts[contextIndex].ValidateEntitiesCount();

			return Iterator(*this);
		}

		void CreateBatchIterators(
			std::vector<BatchIterator>& iterators,
			const uint64_t& desiredBatchesCount,
			const uint64_t& minBatchSize
		)
		{
			uint64_t entitiesCount = 0;
			uint32_t contextCount = m_ArchetypesContexts.size();
			for (uint32_t contextIndex = 0; contextIndex < contextCount; contextIndex++)
			{
				ArchetypeContext& archContext = m_ArchetypesContexts[contextIndex];
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

	template<typename ...ComponentsTypes>
	class View : public ViewBase<ComponentsTypes...>
	{
		using ViewType = ViewBase<ComponentsTypes...>;
		using ArchetypeContext = ViewArchetypeContext<sizeof...(ComponentsTypes)>;
	public:
		View()
		{

		}

		~View()
		{

		}

	protected:
		bool TryAddArchetype(Archetype& archetype) override
		{
			if (!this->ContainArchetype(&archetype))
			{
				// chcecing thaht archetype contains necesary components:
				ArchetypeContext& context = this->m_ArchetypesContexts.emplace_back();

				for (uint32_t typeIdx = 0; typeIdx < this->m_IncludeTypesGroup.Size(); typeIdx++)
				{
					auto typeID_It = archetype.m_TypeIDsIndexes.find(this->m_IncludeTypesGroup.IDs()[typeIdx]);
					if (typeID_It == archetype.m_TypeIDsIndexes.end())
					{
						this->m_ArchetypesContexts.pop_back();
						return false;
					}
					context.TypeIndexes[typeIdx] = typeID_It->second;
				}

				this->m_ContainedArchetypes.insert(&archetype);
				context.Arch = &archetype;
			}


			for (auto& [key, Edge] : archetype.m_Edges)
			{
				if (Edge.AddEdge != nullptr)
				{
					TryAddArchetype(*Edge.AddEdge);
				}
			}

			return true;
		}

	};

	template<typename... Includes, typename... Excludes>
	class View<Include<Includes...>, Exclude<Excludes...>> : public ViewBase<Includes...>
	{
		using ArchetypeContext = ViewArchetypeContext<sizeof...(Includes)>;

	public:
		View()
		{

		}
		~View()
		{

		}

	private:
		Exclude<Excludes...> m_ExcludeTypesGroup = {};

	protected:
		bool TryAddArchetype(Archetype& archetype)
		{
			auto it = this->m_ContainedArchetypes.find(&archetype);
			if (it == this->m_ContainedArchetypes.end())
			{
				// checking if archetype does not containe exclude views:
				{
					for (uint64_t typeIdx = 0; typeIdx < this->m_ExcludeTypesGroup.Size(); typeIdx++)
					{
						auto it = archetype.m_TypeIDsIndexes.find(this->m_ExcludeTypesGroup.IDs()[typeIdx]);
						if (it != archetype.m_TypeIDsIndexes.end())
						{
							return false;
						}
					}
				}

				// chcecing thaht archetype contains necesary components:
				ArchetypeContext& context = this->m_ArchetypesContexts.emplace_back();

				for (uint32_t typeIdx = 0; typeIdx < this->m_IncludeTypesGroup.Size(); typeIdx++)
				{
					auto typeID_It = archetype.m_TypeIDsIndexes.find(this->m_IncludeTypesGroup.IDs()[typeIdx]);
					if (typeID_It == archetype.m_TypeIDsIndexes.end())
					{
						this->m_ArchetypesContexts.pop_back();
						return false;
					}
					context.TypeIndexes[typeIdx] = typeID_It->second;
				}

				this->m_ContainedArchetypes.insert(&archetype);
				context.Arch = &archetype;
			}


			for (auto& [key, Edge] : archetype.m_Edges)
			{
				if (Edge.AddEdge != nullptr)
				{
					TryAddArchetype(*Edge.AddEdge);
				}
			}

			return true;
		}
	};

}