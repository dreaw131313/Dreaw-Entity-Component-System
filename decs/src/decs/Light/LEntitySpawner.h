#pragma once

#include "decs/Core/trait.h"
#include "Archetypes/LArchetypesMap.h"

#include "LEntity.h"
#include "LContainer.h"

namespace decs::light
{

	template<typename Components, typename Tags = TagTypeGroup<>, typename Filterstuple = std::tuple<>>
	struct EntitySpawner;

	template<light_component_or_filter_concept... ComponentTypes, typename... TagTypes, typename... FiltersType>
	struct EntitySpawner<LightComponentTypeGroup<ComponentTypes...>, TagTypeGroup<TagTypes...>, std::tuple<FiltersType...>>
	{
	public:
		using FilterTupleType = std::tuple<FiltersType...>;

	public:
		EntitySpawner() = default;

		EntitySpawner(Container* container, const FilterTupleType& filters = {}):
			m_FiltersTuple(filters)
		{
			SetContainer(container);
		}

		inline bool IsValid() const noexcept
		{
			return m_Container != nullptr && m_Archetype != nullptr;
		}

		void SetContainer(Container* container)
		{
			if (container == m_Container)
			{
				return;
			}
			m_Container = container;
			FetchArchetype();
		}

		void SetFilters(const FilterTupleType& filters)
		{
			if (m_FiltersTuple == filters)
			{
				return;
			}
			m_FiltersTuple = filters;

			FetchArchetype();
		}

		/// <summary>
		/// Reserves space in archetype, that entities conatiner and each component container has reserved at least desiredSpace
		/// </summary>
		/// <param name="desiredSpace"></param>
		void ReserveSpaceInArchetype(size_t desiredSpace)
		{
			if (!IsValid())
			{
				return;
			}

			m_Archetype->ReserveSpaceInArchetype(desiredSpace);
		}

		/// <summary>
		/// Reserves space in archetype, that entities conatiner and each component conta0iner has reserved at least archetype entity count + desiredSpace capacity
		/// </summary>
		/// <param name="desiredSpace"></param>
		void ReserveAditionalSpaceInArchetype(size_t desiredSpace)
		{
			if (!IsValid())
			{
				return;
			}

			m_Archetype->ReserveSpaceInArchetype(desiredSpace + m_Archetype->GetEntities().Size());
		}

		template<typename InitFunc>
			requires light_query_callable<InitFunc, ComponentTypes...>
		inline Entity Spawn(InitFunc&& func)
		{
			return Spawn_Impl<true>(func);
		}

		template<typename InitFunc>
			requires light_query_callable<InitFunc, ComponentTypes...>
		inline void Spawn(size_t entityCount, InitFunc&& func)
		{
			return Spawn_Impl<true>(entityCount, func);
		}

		template<typename InitFunc>
			requires light_query_callable<InitFunc, ComponentTypes...>
		inline Entity Spawn_NoObservers(InitFunc&& func)
		{
			return Spawn_Impl<false>(func);
		}

		template<typename InitFunc>
			requires light_query_callable<InitFunc, ComponentTypes...>
		inline void Spawn_NoObservers(size_t entityCount, InitFunc&& func)
		{
			return Spawn_Impl<false>(entityCount, func);
		}

	private:
		Container* m_Container = nullptr;
		Archetype* m_Archetype = nullptr;
		std::tuple<TArchetypeTypeData<ComponentTypes>...> m_TypeDataTuple{};
		LightComponentTypeGroup<ComponentTypes...> m_ComponentsTypeGroup{};
		TagTypeGroup<TagTypes...> m_TagsTypeGroup{};
		FilterTupleType m_FiltersTuple{};
	private:
		void Invalidate()
		{
			m_Container = nullptr;
			m_Archetype = nullptr;
			m_TypeDataTuple = {};
		}

		void FetchArchetype()
		{
			if (m_Container == nullptr)
			{
				Invalidate();
				return;
			}

			m_Archetype = m_Container->GetArchetypeWithComponentsTagsFilters(m_ComponentsTypeGroup, m_TagsTypeGroup, m_FiltersTuple);
			if (m_Archetype == nullptr)
			{
				Invalidate();
				return;
			}

			m_TypeDataTuple = { m_Archetype->GetComponentTypeData<ComponentTypes>()... };
		}

		template<bool InvokeObservers, typename InitFunc>
			requires light_query_callable<InitFunc, ComponentTypes...>
		void InitializeEntity(Entity& entity, InitFunc&& func)
		{
			auto entityData = entity.GetEntityData();
			const size_t entityIndex = entity.m_EntityData->IndexInArchetype();
			std::tuple<pure_type_t<ComponentTypes>*...> createdComponents{ std::get<TArchetypeTypeData<pure_type_t<ComponentTypes>>>(m_TypeDataTuple).m_PackedContainer->GetAsPtr(entityIndex)... };

			if constexpr (InvokeObservers)
			{
				entityData->LockOperations();
				{
					(std::get<TArchetypeTypeData<ComponentTypes>>(m_TypeDataTuple).m_ComponentContext->InvokeOnCreate(entity, *std::get<pure_type_t<ComponentTypes>*>(createdComponents)), ...);
				}
				entityData->UnlockOperations();
			}

			if constexpr (is_invocable_with_light_entity_v<InitFunc, ComponentTypes...>)
			{
				func(
					entity,
					*std::get<pure_type_t<ComponentTypes>*>(createdComponents)...
				);
			}
			else
			{
				func(*std::get<pure_type_t<ComponentTypes>*>(createdComponents)...);
			}
		}

		template<bool InvokeObservers, typename InitFunc>
			requires light_query_callable<InitFunc, ComponentTypes...>
		Entity Spawn_Impl(InitFunc&& func)
		{
			if (!IsValid())
			{
				return {};
			}

			if (Entity entity = m_Container->CreateEntityInArchetypeWithoutObservers(*m_Archetype))
			{
				InitializeEntity<InvokeObservers>(entity, func);
			}

			return {};
		}

		template<bool InvokeObservers, typename InitFunc>
			requires light_query_callable<InitFunc, ComponentTypes...>
		void Spawn_Impl(size_t entityCount, InitFunc&& func)
		{
			if (!IsValid() || entityCount == 0)
			{
				return;
			}

			for (size_t i = 0; i < entityCount; i++)
			{
				if (Entity entity = m_Container->CreateEntityInArchetypeWithoutObservers(*m_Archetype))
				{
					InitializeEntity<InvokeObservers>(entity, func);
				}
			}
		}
	};
}