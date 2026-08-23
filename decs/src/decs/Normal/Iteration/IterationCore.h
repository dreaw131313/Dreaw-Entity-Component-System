#pragma once
#include "decs/Core/Core.h"
#include "decs/Core/trait.h"

#include "decs/Normal/Archetypes/ArchetypesMap.h"
#include "decs/Normal/Container.h"
#include "decs/Normal/Entity.h"

namespace decs
{
	class Iteration
	{
	public:
		template<typename Callable, typename... ComponentTypes>
		inline static void InvokeEntityIteration(
			Callable&& func,
			uint64_t entityIndexInArchetype,
			const std::tuple<PackedStableComponentContainer<drop_const_t<ComponentTypes>>*...>& containersTuple
		)
		{
			func(std::get<PackedStableComponentContainer<drop_const_t<ComponentTypes>>*>(containersTuple)->GetAsRef(entityIndexInArchetype)...);
		}

		template<typename Callable, typename... ComponentTypes>
		inline static void InvokeEntityIteration(
			Callable&& func,
			Entity& entityBuffer,
			EntityData& entityData,
			uint64_t entityIndexInArchetype,
			const std::tuple<PackedStableComponentContainer<drop_const_t<ComponentTypes>>*...>& containersTuple
		)
		{
			entityBuffer.SetWithoutLifeTimeDataInvalidation_Internal(entityData);
			func(
				entityBuffer,
				std::get<PackedStableComponentContainer<drop_const_t<ComponentTypes>>*>(containersTuple)->GetAsRef(entityIndexInArchetype)...
			);
		}


		template<typename Callable, typename... ComponentTypes>
		inline static bool InvokeEntityFind(
			Callable&& func,
			uint64_t entityIndexInArchetype,
			const std::tuple<PackedStableComponentContainer<drop_const_t<ComponentTypes>>*...>& containersTuple
		)
		{
			return func(std::get<PackedStableComponentContainer<drop_const_t<ComponentTypes>>*>(containersTuple)->GetAsRef(entityIndexInArchetype)...);
		}

		template<typename Callable, typename... ComponentTypes>
		inline static bool InvokeEntityFind(
			Callable&& func,
			Entity& entityBuffer,
			EntityData& entityData,
			uint64_t entityIndexInArchetype,
			const std::tuple<PackedStableComponentContainer<drop_const_t<ComponentTypes>>*...>& containersTuple
		)
		{
			entityBuffer.SetWithoutLifeTimeDataInvalidation_Internal(entityData);
			return func(
				entityBuffer,
				std::get<PackedStableComponentContainer<drop_const_t<ComponentTypes>>*>(containersTuple)->GetAsRef(entityIndexInArchetype)...
			);
		}
	};

	template<component_concept... ComponentsTypes>
	struct QueryFiltersConfig
	{
	public:
		using TypeGroupType = TypeGroup<drop_const_t<ComponentsTypes>...>;

	public:
		const TypeGroupType& GetIncludes() const
		{
			return m_Includes;
		}

		const ecsVector<TypeID>& GetWithoutTypes() const noexcept
		{
			return m_Without;
		}

		const ecsVector<TypeID>& GetWithAnyTypes() const noexcept
		{
			return 	m_WithAnyOf;
		}

		const ecsVector<TypeID>& GetWithAllTypes() const noexcept
		{
			return m_WithAll;
		}

		inline uint64_t GetMinComponentTagCount() const
		{
			uint64_t includesCount = sizeof...(ComponentsTypes);
			if (m_WithAnyOf.size() > 0) includesCount += 1;
			return sizeof...(ComponentsTypes) + m_WithAll.size();
		}

		template<TComponentOrTagConcept... WithoutTypes>
		void Without()
		{
			if constexpr (sizeof...(WithoutTypes) == 0)
			{
				m_Without.clear();
			}
			else
			{
				m_Without.reserve(sizeof...(WithoutTypes));
				(m_Without.push_back(Type<drop_const_t<WithoutTypes>>::ID()), ...);
			}
		}

		template<TComponentOrTagConcept... WithAnyTypes>
		void WithAny()
		{
			if constexpr (sizeof...(WithAnyTypes) == 0)
			{
				m_WithAnyOf.clear();
			}
			else
			{
				m_WithAnyOf.reserve(sizeof...(WithAnyTypes));
				(m_WithAnyOf.push_back(Type<drop_const_t<WithAnyTypes>>::ID()), ...);
			}
		}

		template<TComponentOrTagConcept... WithTypes>
		void With()
		{
			if constexpr (sizeof...(WithTypes) == 0)
			{
				m_WithAll.clear();
			}
			else
			{
				m_WithAll.reserve(sizeof...(WithTypes));
				(m_WithAll.push_back(Type<drop_const_t<WithTypes>>::ID()), ...);
			}
		}

		[[nodiscard]] bool Clear()
		{
			bool bResult = false;
			if (m_WithAll.size() > 0)
			{
				bResult = true;
				m_WithAll.clear();
			}
			if (m_WithAnyOf.size() > 0)
			{
				bResult = true;
				m_WithAnyOf.clear();
			}
			if (m_Without.size() > 0)
			{
				bResult = true;
				m_Without.clear();
			}

			return bResult;
		}

	private:
		TypeGroupType m_Includes = {};
		ecsVector<TypeID> m_Without{};
		ecsVector<TypeID> m_WithAnyOf{};
		ecsVector<TypeID> m_WithAll{};
	};

	template<component_concept... ComponentsTypes>
	class IterationArchetypeContext
	{
	public:
		template<typename ComponentType>
		using TPackedContainer = PackedStableComponentContainer<drop_const_t<ComponentType>>;
		using ContainersTuple = std::tuple<TPackedContainer<drop_const_t<ComponentsTypes>>*...>;

	public:
		inline static constexpr uint64_t s_ComponentCount = sizeof...(ComponentsTypes);

	public:
		inline const Archetype* GetArchetype() const noexcept
		{
			return m_Archetype;
		}

		inline uint64_t GetEntityCount() const
		{
			if (m_Archetype != nullptr)
			{
				return m_Archetype->EntityCount();
			}
			return false;
		}

		inline const ContainersTuple& GetContainersTuple() const noexcept
		{
			return m_ContainersTuple;
		}

		bool Initialize(const Archetype* archetype)
		{
			DECS_ASSERT(archetype != nullptr, "Archetype must not be nullptr!");

			m_Archetype = archetype;

			m_ContainersTuple = { m_Archetype->GetTypePackedContainer<drop_const_t<ComponentsTypes>>()... };
			return ((std::get<TPackedContainer<drop_const_t<ComponentsTypes>>*>(m_ContainersTuple) != nullptr) && ...);
		}

	#pragma region FOREACH
	public:
		template<typename Callable>
		void ForEach(Callable&& func) const
		{
			uint64_t ctxEntityCount = this->GetEntityCount();
			if (ctxEntityCount == 0)
			{
				return;
			}

			const auto& containersTuple = this->GetContainersTuple();
			const auto& archetypeEntityStorage = this->GetArchetype()->GetEntityStorage();

			for (uint64_t idx = 0; idx < ctxEntityCount; idx++)
			{
				if (archetypeEntityStorage.GetEnabled(static_cast<size_t>(idx)))
				{
					Iteration::InvokeEntityIteration<Callable, ComponentsTypes...>(func, idx, containersTuple);
				}
			}
		}

		template<typename Callable>
		void ForEach_WithEntity(Callable&& func, Entity& entityBuffer) const
		{
			uint64_t ctxEntityCount = this->GetEntityCount();
			if (ctxEntityCount == 0)
			{
				return;
			}

			const auto& containersTuple = this->GetContainersTuple();
			const auto& archetypeEntityStorage = this->GetArchetype()->GetEntityStorage();

			for (uint64_t idx = 0; idx < ctxEntityCount; idx++)
			{
				const ArchetypeEntityRecord entityData = archetypeEntityStorage.GetEntityRecord(static_cast<size_t>(idx));
				if (entityData.m_bEnabled)
				{
					Iteration::InvokeEntityIteration<Callable, ComponentsTypes...>(func, entityBuffer, *entityData.m_EntityData, idx, containersTuple);
				}
			}
		}

	#pragma endregion

	#pragma region FOREACH SAFE
	public:
		template<typename Callable>
		void ForEach_Safe(Callable&& func) const
		{
			uint64_t ctxEntityCount = this->GetEntityCount();
			if (ctxEntityCount == 0)
			{
				return;
			}

			const auto& containersTuple = this->GetContainersTuple();
			const auto& archetypeEntityStorage = this->GetArchetype()->GetEntityStorage();

			for (uint64_t idx = 0; idx < ctxEntityCount; idx++)
			{
				const ArchetypeEntityRecord entityData = archetypeEntityStorage.GetEntityRecord(static_cast<size_t>(idx));
				if (entityData.IsValidAndEnabled())
				{
					Iteration::InvokeEntityIteration<Callable, ComponentsTypes...>(func, idx, containersTuple);
				}
			}
		}

		template<typename Callable>
		void ForEach_WithEntity_Safe(Callable&& func, Entity& entityBuffer) const
		{
			uint64_t ctxEntityCount = this->GetEntityCount();
			if (ctxEntityCount == 0)
			{
				return;
			}

			const auto& containersTuple = this->GetContainersTuple();
			const auto& archetypeEntityStorage = this->GetArchetype()->GetEntityStorage();

			for (uint64_t idx = 0; idx < ctxEntityCount; idx++)
			{
				const ArchetypeEntityRecord entityData = archetypeEntityStorage.GetEntityRecord(static_cast<size_t>(idx));
				if (entityData.IsValidAndEnabled())
				{
					Iteration::InvokeEntityIteration<Callable, ComponentsTypes...>(func, entityBuffer, *entityData.m_EntityData, idx, containersTuple);
				}
			}
		}

	#pragma endregion

	#pragma region FOREACH BACKWARD
	public:
		template<typename Callable>
		void ForEachBackward(Callable&& func) const
		{
			uint64_t ctxEntityCount = this->GetEntityCount();
			if (ctxEntityCount == 0)
			{
				return;
			}

			const auto& containersTuple = this->GetContainersTuple();
			const auto& archetypeEntityStorage = this->GetArchetype()->GetEntityStorage();

			int64_t idx = ctxEntityCount - 1;
			for (; idx > -1; idx--)
			{
				if (archetypeEntityStorage.GetEnabled(idx))
				{
					Iteration::InvokeEntityIteration<Callable, ComponentsTypes...>(func, idx, containersTuple);
				}
			}
		}

		template<typename Callable>
		void ForEachBackward_WithEntity(Callable&& func, Entity& entityBuffer) const
		{
			uint64_t ctxEntityCount = this->GetEntityCount();
			if (ctxEntityCount == 0)
			{
				return;
			}

			const auto& containersTuple = this->GetContainersTuple();
			const auto& archetypeEntityStorage = this->GetArchetype()->GetEntityStorage();

			int64_t idx = ctxEntityCount - 1;
			for (; idx > -1; idx--)
			{
				const ArchetypeEntityRecord entityData = archetypeEntityStorage.GetEntityRecord(static_cast<size_t>(idx));
				if (entityData.m_bEnabled)
				{
					Iteration::InvokeEntityIteration<Callable, ComponentsTypes...>(func, entityBuffer, *entityData.m_EntityData, idx, containersTuple);
				}
			}
		}

	#pragma endregion

	#pragma region FOREACH BACKWARD SAFE
	public:
		template<typename Callable>
		void ForEachBackward_Safe(Callable&& func) const
		{
			uint64_t ctxEntityCount = this->GetEntityCount();
			if (ctxEntityCount == 0)
			{
				return;
			}

			const auto& containersTuple = this->GetContainersTuple();
			const auto& archetypeEntityStorage = this->GetArchetype()->GetEntityStorage();

			int64_t idx = ctxEntityCount - 1;
			for (; idx > -1; idx--)
			{
				const ArchetypeEntityRecord entityData = archetypeEntityStorage.GetEntityRecord(static_cast<size_t>(idx));
				if (entityData.IsValidAndEnabled())
				{
					Iteration::InvokeEntityIteration<Callable, ComponentsTypes...>(func, idx, containersTuple);
				}
			}
		}

		template<typename Callable>
		void ForEachBackward_WithEntity_Safe(Callable&& func, Entity& entityBuffer) const
		{
			uint64_t ctxEntityCount = this->GetEntityCount();
			if (ctxEntityCount == 0)
			{
				return;
			}

			const auto& containersTuple = this->GetContainersTuple();
			const auto& archetypeEntityStorage = this->GetArchetype()->GetEntityStorage();

			int64_t idx = ctxEntityCount - 1;
			for (; idx > -1; idx--)
			{
				const ArchetypeEntityRecord entityData = archetypeEntityStorage.GetEntityRecord(static_cast<size_t>(idx));
				if (entityData.IsValidAndEnabled())
				{
					Iteration::InvokeEntityIteration<Callable, ComponentsTypes...>(func, entityBuffer, *entityData.m_EntityData, idx, containersTuple);
				}
			}
		}

	#pragma endregion

	#pragma region FOR EACH INGORE ENTITY ACTIVE STATE
	public:
		template<typename Callable>
		void ForEach_IngoreEntityActiveState(Callable&& func) const
		{
			uint64_t ctxEntityCount = this->GetEntityCount();
			if (ctxEntityCount == 0)
			{
				return;
			}

			const auto& containersTuple = this->GetContainersTuple();
			const auto& archetypeEntityStorage = this->GetArchetype()->GetEntityStorage();

			for (uint64_t idx = 0; idx < ctxEntityCount; idx++)
			{
				auto entityData = archetypeEntityStorage.GetEntity(static_cast<size_t>(idx));
				if (entityData != nullptr)
				{
					Iteration::InvokeEntityIteration<Callable, ComponentsTypes...>(func, idx, containersTuple);
				}
			}
		}

		template<typename Callable>
		void ForEach_IngoreEntityActiveState_WithEntity(Callable&& func, Entity& entityBuffer) const
		{
			uint64_t ctxEntityCount = this->GetEntityCount();
			if (ctxEntityCount == 0)
			{
				return;
			}

			const auto& containersTuple = this->GetContainersTuple();
			const auto& archetypeEntityStorage = this->GetArchetype()->GetEntityStorage();

			for (uint64_t idx = 0; idx < ctxEntityCount; idx++)
			{
				auto entityData = archetypeEntityStorage.GetEntity(static_cast<size_t>(idx));
				if (entityData != nullptr)
				{
					Iteration::InvokeEntityIteration<Callable, ComponentsTypes...>(func, entityBuffer, *entityData, idx, containersTuple);
				}
			}
		}

	#pragma endregion

	#pragma region FOREACH FROM TO

		template<typename Callable>
		void ForEachFromTo(Callable&& func, uint64_t fromIdx, uint64_t toIdx) const
		{
			const auto& containersTuple = this->GetContainersTuple();
			const auto& archetypeEntityStorage = this->GetArchetype()->GetEntityStorage();

			for (uint64_t idx = fromIdx; idx < toIdx; idx++)
			{
				if (archetypeEntityStorage.GetEnabled(idx))
				{
					Iteration::InvokeEntityIteration<Callable, ComponentsTypes...>(func, idx, containersTuple);
				}
			}
		}

		template<typename Callable>
		void ForEachFromTo_WithEntity(Callable&& func, Entity& entityBuffer, uint64_t fromIdx, uint64_t toIdx) const
		{
			const auto& containersTuple = this->GetContainersTuple();
			const auto& archetypeEntityStorage = this->GetArchetype()->GetEntityStorage();

			for (uint64_t idx = fromIdx; idx < toIdx; idx++)
			{
				const ArchetypeEntityRecord entityData = archetypeEntityStorage.GetEntityRecord(static_cast<size_t>(idx));
				if (entityData.m_bEnabled)
				{
					Iteration::InvokeEntityIteration<Callable, ComponentsTypes...>(func, entityBuffer, *entityData.m_EntityData, idx, containersTuple);
				}
			}
		}

	#pragma endregion

	#pragma region FOREACH FROM TO

		template<typename Callable>
		void ForEachFromTo_IgnoreActiveState(Callable&& func, uint64_t fromIdx, uint64_t toIdx) const
		{
			const auto& containersTuple = this->GetContainersTuple();
			const auto& archetypeEntityStorage = this->GetArchetype()->GetEntityStorage();

			for (uint64_t idx = fromIdx; idx < toIdx; idx++)
			{
				auto entityData = archetypeEntityStorage.GetEntity(static_cast<size_t>(idx));
				if (entityData != nullptr)
				{
					Iteration::InvokeEntityIteration<Callable, ComponentsTypes...>(func, idx, containersTuple);
				}
			}
		}

		template<typename Callable>
		void ForEachFromTo_IgnoreActiveState_WithEntity(Callable&& func, Entity& entityBuffer, uint64_t fromIdx, uint64_t toIdx) const
		{
			const auto& containersTuple = this->GetContainersTuple();
			const auto& archetypeEntityStorage = this->GetArchetype()->GetEntityStorage();

			for (uint64_t idx = fromIdx; idx < toIdx; idx++)
			{
				auto entityData = archetypeEntityStorage.GetEntity(static_cast<size_t>(idx));
				if (entityData != nullptr)
				{
					Iteration::InvokeEntityIteration<Callable, ComponentsTypes...>(func, entityBuffer, *entityData, idx, containersTuple);
				}
			}
		}

	#pragma endregion

	#pragma region FIND

		/// <summary>
		/// returns true if func returns true, else false
		/// Iterate over all entites (enabled and disabled)
		/// </summary>

	public:
		template<typename Callable>
		bool Find(Callable&& func) const
		{
			uint64_t ctxEntityCount = this->GetEntityCount();
			if (ctxEntityCount == 0)
			{
				return false;
			}

			const auto& containersTuple = this->GetContainersTuple();
			const auto& archetypeEntityStorage = this->GetArchetype()->GetEntityStorage();

			for (uint64_t idx = 0; idx < ctxEntityCount; idx++)
			{
				const ArchetypeEntityRecord entityData = archetypeEntityStorage.GetEntityRecord(static_cast<size_t>(idx));
				if (entityData.IsValid())
				{
					if (Iteration::InvokeEntityFind<Callable, ComponentsTypes...>(func, idx, containersTuple))
					{
						return true;
					}
				}
			}

			return false;
		}

		template<typename Callable>
		bool Find_WithEntity(Callable&& func, Entity& entityBuffer) const
		{
			uint64_t ctxEntityCount = this->GetEntityCount();
			if (ctxEntityCount == 0)
			{
				return false;
			}

			const auto& containersTuple = this->GetContainersTuple();
			const auto& archetypeEntityStorage = this->GetArchetype()->GetEntityStorage();

			for (uint64_t idx = 0; idx < ctxEntityCount; idx++)
			{
				const ArchetypeEntityRecord entityData = archetypeEntityStorage.GetEntityRecord(static_cast<size_t>(idx));
				if (entityData.IsValid())
				{
					if (Iteration::InvokeEntityFind<Callable, ComponentsTypes...>(func, entityBuffer, *entityData.m_EntityData, idx, containersTuple))
					{
						return true;
					}
				}
			}

			return false;
		}

		template<typename Callable>
		bool FindEnabled(Callable&& func) const
		{
			uint64_t ctxEntityCount = this->GetEntityCount();
			if (ctxEntityCount == 0)
			{
				return false;
			}

			const auto& containersTuple = this->GetContainersTuple();
			const auto& archetypeEntityStorage = this->GetArchetype()->GetEntityStorage();

			for (uint64_t idx = 0; idx < ctxEntityCount; idx++)
			{
				const ArchetypeEntityRecord entityData = archetypeEntityStorage.GetEntityRecord(static_cast<size_t>(idx));
				if (entityData.IsValidAndEnabled())
				{
					if (Iteration::InvokeEntityFind<Callable, ComponentsTypes...>(func, idx, containersTuple))
					{
						return true;
					}
				}
			}

			return false;
		}

		template<typename Callable>
		bool FindEnabled_WithEntity(Callable&& func, Entity& entityBuffer) const
		{
			uint64_t ctxEntityCount = this->GetEntityCount();
			if (ctxEntityCount == 0)
			{
				return false;
			}

			const auto& containersTuple = this->GetContainersTuple();
			const auto& archetypeEntityStorage = this->GetArchetype()->GetEntityStorage();

			for (uint64_t idx = 0; idx < ctxEntityCount; idx++)
			{
				const ArchetypeEntityRecord entityData = archetypeEntityStorage.GetEntityRecord(static_cast<size_t>(idx));
				if (entityData.IsValidAndEnabled())
				{
					if (Iteration::InvokeEntityFind<Callable, ComponentsTypes...>(func, entityBuffer, *entityData.m_EntityData, idx, containersTuple))
					{
						return true;
					}
				}
			}

			return false;
		}

		template<typename Callable>
		bool FindDisabled(Callable&& func) const
		{
			uint64_t ctxEntityCount = this->GetEntityCount();
			if (ctxEntityCount == 0)
			{
				return false;
			}

			const auto& containersTuple = this->GetContainersTuple();
			const auto& archetypeEntityStorage = this->GetArchetype()->GetEntityStorage();

			for (uint64_t idx = 0; idx < ctxEntityCount; idx++)
			{
				const ArchetypeEntityRecord entityData = archetypeEntityStorage.GetEntityRecord(static_cast<size_t>(idx));
				if (entityData.IsValid() && !entityData.IsEnabled())
				{
					if (Iteration::InvokeEntityFind<Callable, ComponentsTypes...>(func, idx, containersTuple))
					{
						return true;
					}
				}
			}

			return false;
		}

		template<typename Callable>
		bool FindDisabled_WithEntity(Callable&& func, Entity& entityBuffer) const
		{
			uint64_t ctxEntityCount = this->GetEntityCount();
			if (ctxEntityCount == 0)
			{
				return false;
			}

			const auto& containersTuple = this->GetContainersTuple();
			const auto& archetypeEntityStorage = this->GetArchetype()->GetEntityStorage();

			for (uint64_t idx = 0; idx < ctxEntityCount; idx++)
			{
				const ArchetypeEntityRecord entityData = archetypeEntityStorage.GetEntityRecord(static_cast<size_t>(idx));
				if (entityData.IsValid() && !entityData.IsEnabled())
				{
					if (Iteration::InvokeEntityFind<Callable, ComponentsTypes...>(func, entityBuffer, *entityData.m_EntityData, idx, containersTuple))
					{
						return true;
					}
				}
			}

			return false;
		}

	#pragma endregion

	private:
		const Archetype* m_Archetype = nullptr;
		ContainersTuple m_ContainersTuple{};
	};


	template<component_concept... ComponentsTypes>
	class IterationContainerContext
	{
	public:
		using ArchetypeContextType = IterationArchetypeContext<ComponentsTypes...>;
		using QueryFilterConfigType = QueryFiltersConfig<drop_const_t<ComponentsTypes>...>;

	public:
		ecsVector<ArchetypeContextType> m_ArchetypesContexts{};
		ecsHashMap<const Archetype*, size_t> m_ArchetypeIndices{};
		Container* m_Container = nullptr;
		TRefCountHandle<ContainerLifetimeData> m_LifeTimeData{};
		bool m_bIsEnabled = true;
		bool m_bIsDirty = true;

	public:
		IterationContainerContext() = default;

		IterationContainerContext(Container* container, bool bIsEnabled = true) :
			m_Container(container),
			m_LifeTimeData(m_Container != nullptr ? m_Container->GetLifeTimeData() : nullptr),
			m_bIsEnabled(bIsEnabled)
		{

		}

		inline bool IsValid() const noexcept
		{
			return m_LifeTimeData.IsValid() && m_LifeTimeData->IsAlive();;
		}

		inline bool IsEnabled() const noexcept
		{
			return m_bIsEnabled;
		}

		inline bool IsValidAndEnabled() const noexcept
		{
			return IsValid() && IsEnabled();
		}

		inline bool IsDirty() const noexcept
		{
			return m_bIsDirty;
		}

		inline void SetDirty()
		{
			m_bIsDirty = true;
		}

		inline Container* GetContainer() const noexcept
		{
			return m_Container;
		}

		const ecsVector<ArchetypeContextType>& GetArchetypeContexts() const noexcept
		{
			return m_ArchetypesContexts;
		}

		inline bool ContainsArchetype(const Archetype* archetype) const noexcept
		{
			return m_ArchetypeIndices.contains(archetype);
		}

		void Clear()
		{
			m_ArchetypesContexts.clear();
			m_ArchetypeIndices.clear();
		}

		void SetContainer(Container* container)
		{
			Clear();
			m_Container = container;
			m_LifeTimeData = m_Container != nullptr ? m_Container->GetLifeTimeData() : nullptr;
		}

		void Fetch(const QueryFilterConfigType& filter)
		{
			if (!IsValid() || !IsDirty())
			{
				return;
			}

			m_bIsDirty = false;

			uint64_t minComponentTagCount = filter.GetMinComponentTagCount();

			ArchetypesMap& map = m_Container->m_ArchetypesMap;
			uint64_t maxComponentsInArchetype = map.GetMaxComponentTagCount();
			if (maxComponentsInArchetype >= minComponentTagCount)
			{
				if (filter.GetIncludes().Size() > 0)
				{
					auto group = GetBestArchetypesGroup(filter);
					FetchArchetypesFromArchetypesGroup(group, filter);
				}
				else
				{
					AddAllArchetypesToQuery(map, filter);
				}
			}
		}

		bool Contains(const decs::Entity& entity)
		{
			return m_ArchetypeIndices.contains(entity.GetArchetype());
		}

		void ValidateCachedEntityCount()
		{
			const uint64_t ctxCount = m_ArchetypesContexts.size();
			for (uint64_t i = 0; i < ctxCount; i++)
			{
				m_ArchetypesContexts[i].ValidateCachedEntityCount();
			}
		}

		uint64_t GetEntityCount() const
		{
			uint64_t entityCount = 0;

			for (const auto& archetypeCtx : m_ArchetypesContexts)
			{
				entityCount += archetypeCtx.GetEntityCount();
			}

			return entityCount;
		}

		void TryAddArchetype(const Archetype& archetype, const QueryFilterConfigType& filter)
		{
			if (!ContainsArchetype(&archetype) && archetype.GetComponentAndTagCount())
			{
				// without test
				{
					auto& without = filter.GetWithoutTypes();

					uint64_t excludeCount = without.size();
					for (int i = 0; i < excludeCount; i++)
					{
						if (archetype.ContainType(without[i]))
						{
							return;
						}
					}
				}

				// with any test
				{
					auto& withAnyOf = filter.GetWithAnyTypes();

					uint64_t requiredAnyCount = withAnyOf.size();
					bool containRequiredAny = requiredAnyCount == 0;

					for (int i = 0; i < requiredAnyCount; i++)
					{
						if (archetype.ContainType(withAnyOf[i]))
						{
							containRequiredAny = true;
							break;
						}
					}
					if (!containRequiredAny) return;
				}

				// required all test
				{
					auto& withAll = filter.GetWithAllTypes();
					uint64_t requiredAllCount = withAll.size();

					for (int i = 0; i < requiredAllCount; i++)
					{
						if (!archetype.ContainType(withAll[i]))
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
						m_ArchetypeIndices[&archetype] = m_ArchetypesContexts.size();
						m_ArchetypesContexts.push_back(context);
					}
				}
			}
		}

		void TryRemoveArchetype(const Archetype& archetype)
		{
			auto it = m_ArchetypeIndices.find(&archetype);
			if (it == m_ArchetypeIndices.end())
			{
				return;
			}

			size_t index = it->second;
			if (index < (m_ArchetypesContexts.size() - 1))
			{
				ArchetypeContextType& lastArchetypeContext = m_ArchetypesContexts.back();
				m_ArchetypesContexts[index] = lastArchetypeContext;
				m_ArchetypeIndices[lastArchetypeContext.GetArchetype()] = index;
			}
			m_ArchetypesContexts.pop_back();
			m_ArchetypeIndices.erase(&archetype);
		}

	private:
		ArchetypesGroupByOneType* GetBestArchetypesGroup(const QueryFilterConfigType& filter)
		{
			auto& groupsMap = m_Container->m_ArchetypesMap.m_ArchetypesGroupedByOneType;

			uint64_t bestArchetypesCount = std::numeric_limits<uint64_t>::max();
			ArchetypesGroupByOneType* bestGroup = nullptr;

			const auto& includes = filter.GetIncludes();
			for (uint64_t i = 0; i < includes.Size(); i++)
			{
				auto it = groupsMap.find(includes[i]);
				if (it != groupsMap.end())
				{
					uint64_t bufforGroupArchetypesCount = it->second->GetArchetypesCount();
					if (bufforGroupArchetypesCount < bestArchetypesCount)
					{
						bestArchetypesCount = bufforGroupArchetypesCount;
						bestGroup = it->second;
					}
				}
			}

			return bestGroup;
		}

		void FetchArchetypesFromArchetypesGroup(ArchetypesGroupByOneType* group, const QueryFilterConfigType& filter)
		{
			if (group == nullptr) return;
			uint64_t maxComponentCountsInGroup = group->MaxComponentsCount();

			for (uint64_t i = filter.GetMinComponentTagCount(); i <= maxComponentCountsInGroup; i++)
			{
				std::span<const Archetype* const> archetypes = group->GetArchetypesWithTypeCount(i);
				for (auto archetype : archetypes)
				{
					TryAddArchetype(*archetype, filter);
				}
			}
		}

		void AddAllArchetypesToQuery(ArchetypesMap& map, const QueryFilterConfigType& filter)
		{
			size_t minRequiredComponentTagCount = filter.GetMinComponentTagCount();

			auto archetypes = map.m_ArchetypeAllocator.GetCreatedArchetypes();

			for (size_t i = 0; i < archetypes.size(); i++)
			{
				Archetype* archetype = archetypes[i];
				if (archetype->GetComponentTagCount() >= minRequiredComponentTagCount)
				{
					TryAddArchetype(*archetype, filter);
				}
			}
		}
	};
}