#pragma once

#include "decs/Core/trait.h"
#include "Archetypes/LArchetypesMap.h"

#include "LEntity.h"
#include "LContainer.h"

namespace decs::light
{

	template<typename Components, typename Tags = TagTypeGroup<>>
	struct EntitySpawner;

	template<TLightComponentConcept... ComponentTypes, TTagConcept... TagTypes>
	struct EntitySpawner<LightComponentTypeGroup<ComponentTypes...>, TagTypeGroup<TagTypes...>>
	{
	public:
		EntitySpawner() = default;

		EntitySpawner(Container* container)
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
		Entity Spawn(InitFunc&& func)
		{
			if (!IsValid())
			{
				return {};
			}

			if (Entity entity = m_Container->CreateEntityInArchetype(*m_Archetype))
			{
				const size_t entityIndex = entity.m_EntityData->IndexInArchetype();
				if constexpr (is_invocable_with_light_entity_v<InitFunc, ComponentTypes...>)
				{
					func(
						entity,
						std::get<PackedLightComponentContainer<drop_const_t<ComponentTypes>>*>(m_ComponentsPackedContainers)->GetAsRef(entityIndex)...
					);
				}
				else
				{
					func(std::get<PackedLightComponentContainer<drop_const_t<ComponentTypes>>*>(m_ComponentsPackedContainers)->GetAsRef(entityIndex)...);
				}

				return entity;
			}

			return {};
		}

		template<typename InitFunc>
			requires light_query_callable<InitFunc, ComponentTypes...>
		void Spawn(size_t entityCount, InitFunc&& func)
		{
			if (!IsValid() || entityCount == 0)
			{
				return;
			}

			for (size_t i = 0; i < entityCount; i++)
			{
				if (Entity entity = m_Container->CreateEntityInArchetype(*m_Archetype))
				{
					const size_t entityIndex = entity.m_EntityData->IndexInArchetype();
					if constexpr (is_invocable_with_light_entity_v<InitFunc, ComponentTypes...>)
					{
						func(
							entity,
							std::get<PackedLightComponentContainer<drop_const_t<ComponentTypes>>*>(m_ComponentsPackedContainers)->GetAsRef(entityIndex)...
						);
					}
					else
					{
						func(std::get<PackedLightComponentContainer<drop_const_t<ComponentTypes>>*>(m_ComponentsPackedContainers)->GetAsRef(entityIndex)...);
					}
				}
			}
		}

	private:
		Container* m_Container = nullptr;
		Archetype* m_Archetype = nullptr;
		std::tuple<PackedLightComponentContainer<drop_const_t<ComponentTypes>>*...> m_ComponentsPackedContainers{};
		LightComponentTypeGroup<ComponentTypes...> m_ComponentsTypeGroup{};
		TagTypeGroup<TagTypes...> m_TagsTypeGroup{};

	private:
		void Invalidate()
		{
			m_Container = nullptr;
			m_Archetype = nullptr;
			m_ComponentsPackedContainers = {};
		}

		void FetchArchetype()
		{
			if (m_Container == nullptr)
			{
				Invalidate();
				return;
			}

			m_Archetype = m_Container->GetArchetypeWithComponentsAndTags(m_ComponentsTypeGroup, m_TagsTypeGroup);
			if (m_Archetype == nullptr)
			{
				Invalidate();
				return;
			}

			m_ComponentsPackedContainers = { m_Archetype->GetTypePackedContainer<drop_const_t<ComponentTypes>>()... };
		}

	};
}