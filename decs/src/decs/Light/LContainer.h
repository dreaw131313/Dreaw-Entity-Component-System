#pragma once

#include "Archetypes/LArchetypesMap.h"
#include "LEntityManager.h"
#include "Filter/LFilter.h"
#include "Iteration/LQueryManager.h"

namespace decs::light
{
	class Entity;
	template<typename Components, typename Tags>
	struct EntitySpawner;


	struct ContainerConfig
	{
	public:
		uint64_t EntityChunkSize = 1000;
		uint64_t ArchetypeChunkSize = 1000;
	};

	class Container final : private NonCopyableNonMoveable
	{
		template<light_component_or_filter_concept... ComponentsTypes>
		friend class QueryImpl;
		template<light_component_or_filter_concept ...Types>
		friend class light::Query;
		template<light_component_or_filter_concept ...Types>
		friend class light::MultiQuery;
		template<light_component_or_filter_concept...>
		friend class light::IterationContainerContext;
		friend class light::Entity;
		friend class light::ContainerIterator;

		template<typename Components, typename Tags>
		friend struct light::EntitySpawner;

	private:
		static constexpr uint64_t m_DefaultEntitiesChunkSize = 1000;
		static constexpr uint64_t m_DefaultEmptyEntitiesChunkSize = 100;

	public:
		Container();

		Container(const ContainerConfig& config);

		~Container();

	#pragma region UTILITY
	public:
		/// <summary>
		/// Returns all owned entites to entity manager. Clears all created components. Does not destroy created archetypes.
		/// </summary>
		void Clear();

	private:
		void ReturnOwnedEntitiesToEntityManager_Internal();
	#pragma endregion

	#pragma region ENTITIES:
	private:
		std::vector<EntityData*> m_EmptyEntities = {};
		EntityManager m_EntityManager{};

	public:
		[[nodiscard]] Entity CreateEntity();

		[[nodiscard]] inline uint32_t GetEntityCount() const
		{
			return m_EntityManager.GetCreatedEntityCount();
		}

		bool DestroyEntity(const Entity& entity);

		[[nodiscard]] inline uint64_t GetEmptyEntitiesCount() const
		{
			return m_EmptyEntities.size();
		}

		/// <summary>
		/// This function ignores component callbacks orders, callbacks are invoked in order of ComponentTypes in LightComponentTypeGroup parameter.
		/// During observer callbacks invocation removing components can cause undefined behavior or reading from freed memory.
		/// </summary>
		/// <typeparam name="InitFunc"></typeparam>
		/// <typeparam name="...ComponentTypes"></typeparam>
		/// <typeparam name="...TagTypes"></typeparam>
		/// <param name="components"></param>
		/// <param name="tags"></param>
		/// <param name="bIsActive"></param>
		/// <param name="initFunc"></param>
		/// <returns></returns>
		template<typename InitFunc, light_component_or_filter_concept... ComponentTypes, tag_concept... TagTypes>
			requires light_query_callable<InitFunc, ComponentTypes...>
		void CreateEntities(
			const LightComponentTypeGroup<ComponentTypes...> components,
			const TagTypeGroup<TagTypes...> tags,
			uint32_t entityCount,
			InitFunc&& initFunc
		)
		{
			if (entityCount == 0)
			{
				return;
			}

			if constexpr (sizeof...(TagTypes) == 0 && sizeof...(ComponentTypes) == 0)
			{
				for (uint32_t i = 0; i < entityCount; i++)
				{
					Entity e = CreateEntity();
					initFunc(e);
				}
			}
			else
			{
				Archetype* spawnArchetype = nullptr;

				((spawnArchetype = GetArchetypeAfterAddTag(spawnArchetype, Type<TagTypes>::ID())), ...);
				((spawnArchetype = GetArchetypeAfterAddComponent<ComponentTypes>(spawnArchetype)), ...);

				if (spawnArchetype != nullptr)
				{
					std::tuple<PackedLightComponentContainer<drop_const_t<ComponentTypes>>*...> packedContainerTyple = { spawnArchetype->GetTypePackedContainer<drop_const_t<ComponentTypes>>()... };

					for (uint32_t i = 0; i < entityCount; i++)
					{
						if (Entity entity = CreateEntityRaw())
						{
							EntityData* entityData = GetEntityData(entity);
							spawnArchetype->AddEntityData(entityData);

							std::tuple<drop_const_t<ComponentTypes>*...> createdComponents = {
								&std::get<PackedLightComponentContainer<drop_const_t<ComponentTypes>>*>(packedContainerTyple)->EmplaceBack<>()
								...
							};

							if constexpr (is_invocable_with_light_entity_v<InitFunc, ComponentTypes...>)
							{
								initFunc(entity, *std::get<drop_const_t<ComponentTypes>*>(createdComponents)...);
							}
							else
							{
								initFunc(*std::get<ComponentTypes*>(createdComponents)...);
							}
						}
					}
				}
			}
		}

		template<typename InitFunc, light_component_or_filter_concept... ComponentTypes>
			requires light_query_callable<InitFunc, ComponentTypes...>
		void CreateEntities(
			const LightComponentTypeGroup<ComponentTypes...> components,
			uint32_t entityCount,
			InitFunc&& initFunc
		)
		{
			constexpr const TagTypeGroup<> tags{};
			CreateEntities(components, tags, entityCount, initFunc);
		}

		/// <summary>
		/// This function ignores component callbacks orders, callbacks are invoked in order of ComponentTypes in LightComponentTypeGroup parameter.
		/// During observer callbacks invocation removing components can cause undefined behavior or reading from freed memory.
		/// </summary>
		/// <typeparam name="InitFunc"></typeparam>
		/// <typeparam name="...ComponentTypes"></typeparam>
		/// <typeparam name="...TagTypes"></typeparam>
		/// <param name="components"></param>
		/// <param name="tags"></param>
		/// <param name="entityCount"></param>
		/// <param name="bIsActive"></param>
		/// <param name="initFunc"></param>
		/// <returns></returns>
		template<typename InitFunc, light_component_or_filter_concept... ComponentTypes, tag_concept... TagTypes>
			requires light_query_callable<InitFunc, ComponentTypes...>
		Entity CreateEntity(
			const LightComponentTypeGroup<ComponentTypes...> components,
			const TagTypeGroup<TagTypes...> tags,
			InitFunc&& initFunc
		)
		{
			if constexpr (sizeof...(TagTypes) == 0 && sizeof...(ComponentTypes) == 0)
			{
				if (Entity e = CreateEntity())
				{
					initFunc(e);
					return e;
				}
			}
			else
			{
				Archetype* spawnArchetype = nullptr;

				((spawnArchetype = GetArchetypeAfterAddTag(spawnArchetype, Type<TagTypes>::ID())), ...);
				((spawnArchetype = GetArchetypeAfterAddComponent<ComponentTypes>(spawnArchetype)), ...);

				if (spawnArchetype != nullptr)
				{
					std::tuple<PackedLightComponentContainer<drop_const_t<ComponentTypes>>*...> packedContainerTyple = { spawnArchetype->GetTypePackedContainer<drop_const_t<ComponentTypes>>()... };

					if (Entity entity = CreateEntityRaw())
					{

						EntityData* entityData = GetEntityData(entity);
						spawnArchetype->AddEntityData(entityData);

						std::tuple<drop_const_t<ComponentTypes>*...> createdComponents = {
							&std::get<PackedLightComponentContainer<drop_const_t<ComponentTypes>>*>(packedContainerTyple)->EmplaceBack<>()
							...
						};

						if constexpr (is_invocable_with_light_entity_v<InitFunc, ComponentTypes...>)
						{
							initFunc(entity, *std::get<drop_const_t<ComponentTypes>*>(createdComponents)...);
						}
						else
						{
							initFunc(*std::get<ComponentTypes*>(createdComponents)...);
						}

						return entity;
					}
				}
			}

			return Entity();
		}

		template<typename InitFunc, light_component_or_filter_concept... ComponentTypes>
			requires light_query_callable<InitFunc, ComponentTypes...>
		Entity CreateEntity(
			const LightComponentTypeGroup<ComponentTypes...> components,
			InitFunc&& initFunc
		)
		{
			constexpr const TagTypeGroup<> tags{};
			return CreateEntity(components, tags, initFunc);
		}

	private:
		/// <summary>
		/// 
		/// </summary>
		/// <param name="archetype"></param>
		/// <returns>New entity index in archetype</returns>
		Entity CreateEntityInArchetype(Archetype& archetype);

		bool DestroyEntityInternal(const Entity& entity, bool bInvokeObservers);

		EntityData* GetEntityData(const Entity& entity) const;

		Entity CreateEntityRaw();

	private:
		void AddToEmptyEntitiesRightAfterNewEntityCreation(EntityData& data);

		void AddToEmptyEntities(EntityData& data);

		void RemoveFromEmptyEntities(EntityData& data);

	#pragma endregion

	#pragma region SPAWNING ENTITIES:
	public:
		Entity Spawn(const Entity& prefab);

		bool Spawn(const Entity& prefab, uint64_t spawnCount);

		bool Spawn(const Entity& prefab, std::vector<Entity>& spawnedEntities, uint64_t spawnCount);

	private:
		Archetype* GetArchetypeForSpawn(const EntityData& prefabEntityData);

		void CreateEntityFromSpawnData(
			const EntityData& prefabEntityData,
			const Archetype& prefabArchetype,
			const Entity& spawnedEntity,
			Archetype& spawnArchetype
		);

	#pragma endregion

	#pragma region COMPONENTS:
	private:
		template<light_component_or_filter_concept TComponent, typename ...Args>
		TComponent* AddComponent(const Entity& entity, EntityData& entityData, Args&&... args)
		{
			TYPE_ID_CONSTEXPR TypeID componentTypeID = Type<TComponent>::ID();

			auto currentComponent = GetComponent<TComponent>(entityData);
			if (currentComponent != nullptr)
			{
				return currentComponent;
			}

			Archetype* oldArchetype = entityData.m_Archetype;
			const uint32_t indexInOldArchetype = entityData.m_IndexInArchetype;

			Archetype* newArchetype = GetArchetypeAfterAddComponent<TComponent>(entityData.m_Archetype);
			PackedLightComponentContainer<TComponent>* packedContainer = newArchetype->GetTypePackedContainer<drop_const_t<TComponent>>();
			TComponent* componentPtr = &packedContainer->EmplaceBack(std::forward<Args>(args)...);

			// Adding entity to archetype
			if (oldArchetype != nullptr)
			{
				Archetype::MoveEntityAfterAddType(*oldArchetype, *newArchetype, indexInOldArchetype, componentTypeID);
			}
			else
			{
				RemoveFromEmptyEntities(entityData);
				newArchetype->AddEntityData(&entityData);
			}

			return componentPtr;
		}

		template<light_component_or_filter_concept TComponent>
		bool RemoveComponent(const Entity& entity)
		{
			return RemoveComponent(entity, Type<TComponent>::ID());
		}

		bool RemoveComponent(const Entity& entity, TypeID componentTypeID);

		template<light_component_or_filter_concept TComponent>
		TComponent* GetComponent(EntityData& entityData) const
		{
			if constexpr (is_tag_v<TComponent>)
			{
				return nullptr;
			}

			if (entityData.m_Archetype != nullptr)
			{
				uint32_t findTypeIndex = entityData.m_Archetype->FindTypeIndex<TComponent>();
				if (findTypeIndex != std::numeric_limits<uint32_t>::max())
				{
					PackedLightComponentContainer<TComponent>* container = ::decs::check_cast<PackedLightComponentContainer<TComponent>*>(entityData.m_Archetype->m_TypeData[findTypeIndex].m_PackedContainer);
					return container->GetAsPtr(entityData.m_IndexInArchetype);
				}
			}
			return nullptr;
		}

		bool HasComponentInternal(EntityData& entityData, TypeID typeID) const
		{
			if (entityData.m_Archetype != nullptr)
			{
				return entityData.m_Archetype->HasComponentType(typeID);
			}
			return false;
		}

		template<light_component_or_filter_concept TComponent>
		bool HasComponent(EntityData& entityData) const
		{
			if constexpr (is_tag_v<TComponent>)
			{
				return false;
			}

			return HasComponentInternal(entityData, Type<TComponent>::ID());
		}

	#pragma endregion

	#pragma region FILTERS
	private:
		template<filter_concept FilterType>
		Archetype* GetArchetypeAfterSetFilter(Archetype* toArchetype, const FilterType& filter)
		{
			if (toArchetype == nullptr)
			{
				return m_ArchetypesMap.GetOrCreateSingleFilterArchetype<FilterType>(filter);
			}
			else
			{
				return m_ArchetypesMap.GetOrCreateArchetypeAfterSetFilter<FilterType>(*toArchetype, filter);
			}
		}

		Archetype* GetArchetypeAfterRemoveFilter(Archetype* fromArchetype, TypeID filterID);

		template<filter_concept FilterType>
		bool SetFilter(EntityData& entityData, const FilterType& filter)
		{
			TYPE_ID_CONSTEXPR TypeID filterTypeID = Type<FilterType>::ID();

			Archetype* oldArchetype = entityData.m_Archetype;
			const uint32_t indexInOldArchetype = entityData.m_IndexInArchetype;

			Archetype* newArchetype = this->GetArchetypeAfterSetFilter<FilterType>(oldArchetype, filter);
			if (newArchetype == oldArchetype)
			{
				return true;
			}

			if (oldArchetype != nullptr)
			{
				Archetype::MoveEntiyAfterFilterChange(*oldArchetype, *newArchetype, indexInOldArchetype);
			}
			else
			{
				RemoveFromEmptyEntities(entityData);
				newArchetype->AddEntityData(&entityData);
			}

			return true;
		}

		bool RemoveFilter(EntityData& entityData, TypeID filterTypeID);

		template<filter_concept FilterType>
		bool RemoveFilter(EntityData& entityData)
		{
			return RemoveFilter(entityData, Type<FilterType>::ID());
		}

		template<filter_concept FilterType>
		const FilterType* GetFilter(EntityData& entityData)
		{
			if (entityData.m_Archetype == nullptr
				|| entityData.m_Archetype->GetFilters().size() == 0
				)
			{
				return nullptr;
			}

			FilterContainer<FilterType>* filterContainer = entityData.m_Archetype->GetFilterContainer<FilterType>();
			if (filterContainer == nullptr)
			{
				return nullptr;
			}

			return &filterContainer->m_Data;
		}

		bool HasFilter(const EntityData& entityData, TypeID filterID);

		template<filter_concept FilterType>
		bool HasFilter(const EntityData& entityData)
		{
			return HasFilter(entityData, Type<FilterType>::ID());
		}

		template<filter_concept FilterType>
		bool HasFilter(const EntityData& entityData, const FilterType& filterData)
		{
			if (entityData.m_Archetype == nullptr
				|| entityData.m_Archetype->GetFilters().size() == 0
				)
			{
				return false;
			}

			FilterContainer<FilterType>* filterContainer = entityData.m_Archetype->GetFilterContainer<FilterType>();
			if (filterContainer == nullptr)
			{
				return false;
			}

			return filterContainer->m_Data == filterData;
		}

	private:
		FilterManager m_FilterManager{};

	#pragma endregion

	#pragma region TAGS:
	private:
		Archetype* GetArchetypeAfterAddTag(Archetype* toArchetype, TypeID tagID)
		{
			if (toArchetype == nullptr)
			{
				return m_ArchetypesMap.CreateSingleTagArchetype(tagID);
			}

			return m_ArchetypesMap.GetArchetypeAfterAddTag(*toArchetype, tagID);
		}

		Archetype* GetArchetypeAfterRemoveTag(Archetype& fromArchetype, TypeID tagID)
		{
			return m_ArchetypesMap.GetArchetypeAfterRemoveTag(fromArchetype, tagID);
		}

		inline bool HasTag(const EntityData& entityData, TypeID tagType)
		{
			if (entityData.m_Archetype == nullptr)
			{
				return false;
			}

			return entityData.m_Archetype->HasTag(tagType);
		}

		template<tag_concept TTag>
		inline bool HasTag(const EntityData& entityData)
		{
			return HasTag(entityData, Type<TTag>::ID());
		}

		template<tag_concept TTag>
		bool AddTag(EntityData& entityData)
		{
			Archetype* oldArchetype = entityData.m_Archetype;
			if (oldArchetype != nullptr && oldArchetype->HasTag<TTag>())
			{
				return true;
			}

			TYPE_ID_CONSTEXPR const TypeID tagTypeID = Type<TTag>::ID();

			const uint32_t indexInOldArchetype = entityData.m_IndexInArchetype;
			Archetype* newArchetype = GetArchetypeAfterAddTag(oldArchetype, tagTypeID);

			if (oldArchetype != nullptr)
			{
				Archetype::MoveEntityAfterAddType(*oldArchetype, *newArchetype, indexInOldArchetype, tagTypeID);
			}
			else
			{
				// means that archetype has one component/tag so we just need add entity to it
				RemoveFromEmptyEntities(entityData);
				newArchetype->AddEntityData(&entityData);
			}

			return true;
		}

		bool RemoveTag(EntityData& entityData, TypeID tagType);

		template<tag_concept TTag>
		bool RemoveTag(EntityData& entityData)
		{
			return RemoveTag(entityData, Type<TTag>::ID());
		}

	#pragma endregion

	#pragma region ARCHETYPES:
	private:
		QueryManager m_QueryManager;
		ArchetypesMap m_ArchetypesMap;

	public:
		inline void ShrinkArchetypesToFit()
		{
			m_ArchetypesMap.ShrinkArchetypesToFit();
		}

		inline void ShrinkArchetypesToFit(ArchetypesShrinkToFitState& state)
		{
			m_ArchetypesMap.ShrinkArchetypesToFit(state);
		}

		inline uint64_t GetArchetypeCount() const
		{
			return m_ArchetypesMap.GetArchetypesCount();
		}

	private:
		template<typename TComponent>
		Archetype* GetArchetypeAfterAddComponent(Archetype* toArchetype)
		{
			TYPE_ID_CONSTEXPR const TypeID addedComponentTypeID = Type<TComponent>::ID();

			Archetype* entityNewArchetype = nullptr;
			if (toArchetype == nullptr)
			{
				entityNewArchetype = m_ArchetypesMap.CreateSingleComponentArchetype<TComponent>();
			}
			else
			{
				entityNewArchetype = m_ArchetypesMap.GetArchetypeAfterAddComponent<TComponent>(*toArchetype);
			}

			return entityNewArchetype;
		}

		template<light_component_or_filter_concept... ComponentTypes, tag_concept... TagTypes>
		Archetype* GetArchetypeWithComponentsAndTags(
			const LightComponentTypeGroup<ComponentTypes...> components,
			const TagTypeGroup<TagTypes...> tags
		)
		{
			Archetype* spawnArchetype = nullptr;

			((spawnArchetype = GetArchetypeAfterAddTag(spawnArchetype, Type<TagTypes>::ID())), ...);
			((spawnArchetype = GetArchetypeAfterAddComponent<ComponentTypes>(spawnArchetype)), ...);

			return spawnArchetype;
		}

		void AddQuery(IQuery* query);

		void RemoveQuery(IQuery* query);

		void AddMultiQuery(IMultiQuery* query);

		void RemoveMultiQuery(IMultiQuery* query);


	#pragma endregion

	};
}