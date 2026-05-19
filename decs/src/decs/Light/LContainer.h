#pragma once

#include "Archetypes/LArchetypesMap.h"
#include "LEntityManager.h"
#include "Filter/LFilter.h"
#include "Iteration/LQueryManager.h"
#include "Component/LComponentContext.h"

namespace decs::light
{
	class Entity;
	template<typename, typename, typename>
	struct EntitySpawner;


	struct ContainerConfig
	{
	public:
		uint64_t EntityChunkSize = 1000;
		uint64_t ArchetypeChunkSize = 1000;
	};

	class Container final : private NonCopyableNonMoveable
	{
		template<light_component_or_filter_concept ...Types>
		friend class light::Query;
		template<light_component_or_filter_concept ...Types>
		friend class light::MultiQuery;
		template<light_component_or_filter_concept...>
		friend class light::IterationContainerContext;
		friend class light::Entity;
		friend class light::ContainerIterator;

		template<typename, typename, typename>
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

		bool DestroyEntity_NoObservers(const Entity& entity);

		[[nodiscard]] inline uint64_t GetEmptyEntitiesCount() const
		{
			return m_EmptyEntities.size();
		}

	private:
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
		template<bool InvokeObservers, typename InitFunc, light_component_concept... ComponentTypes, typename... TagTypes, typename... FiltersData>
			requires light_query_callable<InitFunc, ComponentTypes...>
		void CreateEntities_Impl(
			const LightComponentTypeGroup<ComponentTypes...>& components,
			const TagTypeGroup<TagTypes...>& tags,
			const std::tuple<FiltersData...>& filtersTupleData,
			uint32_t entityCount,
			InitFunc&& initFunc
		)
		{
			if (entityCount == 0)
			{
				return;
			}

			if constexpr (sizeof...(TagTypes) == 0 && sizeof...(ComponentTypes) == 0 && sizeof...(FiltersData) == 0)
			{
				for (uint32_t i = 0; i < entityCount; i++)
				{
					Entity e = CreateEntity();
					initFunc(e);
				}
			}
			else
			{
				Archetype* spawnArchetype = GetArchetypeWithComponentsTagsFilters(components, tags, filtersTupleData);

				if (spawnArchetype != nullptr)
				{
					std::tuple<TArchetypeTypeData<ComponentTypes>...>  typeDataTuple{ spawnArchetype->GetComponentTypeData<ComponentTypes>()... };

					for (uint32_t i = 0; i < entityCount; i++)
					{
						if (Entity entity = CreateEntityRaw())
						{
							EntityData* entityData = GetEntityData(entity);
							spawnArchetype->AddEntityData(entityData);

							std::tuple<pure_type_t<ComponentTypes>*...> createdComponents = {
								&std::get<TArchetypeTypeData<ComponentTypes>>(typeDataTuple).m_PackedContainer->EmplaceBack<>()
								...
							};

							if constexpr (InvokeObservers)
							{
								entityData->LockOperations();
								{
									(std::get<TArchetypeTypeData<ComponentTypes>>(typeDataTuple).m_ComponentContext->InvokeOnCreate(entity, *std::get<pure_type_t<ComponentTypes>*>(createdComponents)), ...);
								}
								entityData->UnlockOperations();
							}

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

	public:
		template<typename InitFunc, light_component_concept... ComponentTypes, typename... TagTypes, typename... FiltersData>
			requires light_query_callable<InitFunc, ComponentTypes...>
		inline void CreateEntities(
			const LightComponentTypeGroup<ComponentTypes...>& components,
			const TagTypeGroup<TagTypes...>& tags,
			const std::tuple<FiltersData...>& filtersTupleData,
			uint32_t entityCount,
			InitFunc&& initFunc
		)
		{
			CreateEntities_Impl<true>(components, tags, filtersTupleData, entityCount, initFunc);
		}

		template<typename InitFunc, light_component_concept... ComponentTypes, typename... TagTypes>
			requires light_query_callable<InitFunc, ComponentTypes...>
		void CreateEntities(
			const LightComponentTypeGroup<ComponentTypes...> components,
			const TagTypeGroup<TagTypes...>& tags,
			uint32_t entityCount,
			InitFunc&& initFunc
		)
		{
			const std::tuple<> filtersTuple{};
			CreateEntities(components, tags, filtersTuple, entityCount, initFunc);
		}

		template<typename InitFunc, light_component_concept... ComponentTypes>
			requires light_query_callable<InitFunc, ComponentTypes...>
		void CreateEntities(
			const LightComponentTypeGroup<ComponentTypes...> components,
			uint32_t entityCount,
			InitFunc&& initFunc
		)
		{
			constexpr const TagTypeGroup<> tags{};
			const std::tuple<> filtersTuple{};
			CreateEntities(components, tags, filtersTuple, entityCount, initFunc);
		}


	private:
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
		template<bool InvokeObservers, typename InitFunc, light_component_concept... ComponentTypes, typename... TagTypes, typename... FiltersData>
			requires light_query_callable<InitFunc, ComponentTypes...>
		Entity CreateEntity_Impl(
			const LightComponentTypeGroup<ComponentTypes...> components,
			const TagTypeGroup<TagTypes...> tags,
			const std::tuple<FiltersData...>& filtersTupleData,
			InitFunc&& initFunc
		)
		{
			if constexpr (sizeof...(TagTypes) == 0 && sizeof...(ComponentTypes) == 0 && sizeof...(FiltersData) == 0)
			{
				if (Entity e = CreateEntity())
				{
					initFunc(e);
					return e;
				}
			}
			else
			{
				Archetype* spawnArchetype = GetArchetypeWithComponentsTagsFilters(components, tags, filtersTupleData);

				if (spawnArchetype != nullptr)
				{
					std::tuple<TArchetypeTypeData<ComponentTypes>...>  typeDataTuple{ spawnArchetype->GetComponentTypeData<ComponentTypes>()... };

					if (Entity entity = CreateEntityRaw())
					{
						EntityData* entityData = GetEntityData(entity);
						spawnArchetype->AddEntityData(entityData);

						std::tuple<pure_type_t<ComponentTypes>*...> createdComponents = {
							&std::get<TArchetypeTypeData<ComponentTypes>>(typeDataTuple).m_PackedContainer->EmplaceBack<>()
							...
						};

						if constexpr (InvokeObservers)
						{
							entityData->LockOperations();
							{
								(std::get<TArchetypeTypeData<ComponentTypes>>(typeDataTuple).m_ComponentContext->InvokeOnCreate(entity, *std::get<pure_type_t<ComponentTypes>*>(createdComponents)), ...);
							}
							entityData->UnlockOperations();
						}

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

	public:
		template<typename InitFunc, light_component_concept... ComponentTypes, typename... TagTypes, typename... FiltersData>
			requires light_query_callable<InitFunc, ComponentTypes...>
		Entity CreateEntity(
			const LightComponentTypeGroup<ComponentTypes...> components,
			const TagTypeGroup<TagTypes...> tags,
			const std::tuple<FiltersData...>& filtersTupleData,
			InitFunc&& initFunc
		)
		{
			return CreateEntity_Impl<true>(components, tags, filtersTupleData, initFunc);
		}

		template<typename InitFunc, light_component_concept... ComponentTypes, typename... TagTypes>
			requires light_query_callable<InitFunc, ComponentTypes...>
		Entity CreateEntity(
			const LightComponentTypeGroup<ComponentTypes...> components,
			const TagTypeGroup<TagTypes...> tags,
			InitFunc&& initFunc
		)
		{
			const std::tuple<> filtersTuple{};
			return CreateEntity(components, tags, filtersTuple, initFunc);
		}

		template<typename InitFunc, light_component_concept... ComponentTypes>
			requires light_query_callable<InitFunc, ComponentTypes...>
		Entity CreateEntity(
			const LightComponentTypeGroup<ComponentTypes...> components,
			InitFunc&& initFunc
		)
		{
			constexpr const TagTypeGroup<> tags{};
			const std::tuple<> filtersTuple{};
			return CreateEntity(components, tags, filtersTuple, initFunc);
		}



	private:
		/// <summary>
		/// 
		/// </summary>
		/// <param name="archetype"></param>
		/// <returns>New entity index in archetype</returns>
		Entity CreateEntityInArchetype(Archetype& archetype);

		Entity CreateEntityInArchetypeWithoutObservers(Archetype& archetype);

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
	public:
		/// <summary>
		/// 
		/// </summary>
		/// <typeparam name="Func"></typeparam>
		/// <typeparam name="ComponentType"></typeparam>
		/// <param name="func"></param>
		/// <returns>id for removing function observer</returns>
		template<light_component_concept ComponentType, typename Func>
			requires light_component_observer_func<Func, ComponentType>
		ObserverID AddComponentCreateObserver(Func&& func)
		{
			TComponentContext<pure_type_t<ComponentType>>* context = m_ComponentContextManager.GetOrCreateContext<ComponentType>();
			return context->m_OnCreateFunction.AddFunction(func);
		}

		template<light_component_concept ComponentType>
		bool RemoveComponentCreateObserver(ObserverID id)
		{
			TComponentContext<pure_type_t<ComponentType>>* context = m_ComponentContextManager.GetOrCreateContext<ComponentType>();
			return context->m_OnCreateFunction.RemoveFunction(id);
		}

		/// <summary>
		/// 
		/// </summary>
		/// <typeparam name="Func"></typeparam>
		/// <typeparam name="ComponentType"></typeparam>
		/// <param name="func"></param>
		/// <returns>id for removing function observer</returns>
		template<light_component_concept ComponentType, typename Func>
			requires light_component_observer_func<Func, ComponentType>
		ObserverID AddComponentDestroyObserver(Func&& func)
		{
			TComponentContext<pure_type_t<ComponentType>>* context = m_ComponentContextManager.GetOrCreateContext<ComponentType>();
			return context->m_OnDestroyFunction.AddFunction(func);
		}

		template<light_component_concept ComponentType>
		bool RemoveComponentDestroyObserver(ObserverID id)
		{
			TComponentContext<pure_type_t<ComponentType>>* context = m_ComponentContextManager.GetOrCreateContext<ComponentType>();
			return context->m_OnDestroyFunction.RemoveFunction(id);
		}

		template<light_component_concept ComponentType, typename Func>
			requires light_component_observer_func<Func, ComponentType>
		ObserverID AddComponentSetObserver(Func&& func)
		{
			TComponentContext<pure_type_t<ComponentType>>* context = m_ComponentContextManager.GetOrCreateContext<ComponentType>();
			return context->m_OnSetFunction.AddFunction(func);
		}

		template<light_component_concept ComponentType>
		bool RemoveComponentSetObserver(ObserverID id)
		{
			TComponentContext<pure_type_t<ComponentType>>* context = m_ComponentContextManager.GetOrCreateContext<ComponentType>();
			return context->m_OnSetFunction.RemoveFunction(id);
		}

	private:
		ComponentContextManager m_ComponentContextManager{};

	private:
		template<bool InvokeObserver, light_component_concept TComponent, typename ...Args>
		TComponent* AddComponent_Impl(const Entity& entity, EntityData& entityData, Args&&... args)
		{
			if (entityData.OperationsLocked())
			{
				return nullptr;
			}

			using PureComponentType = pure_type_t<TComponent>;
			using PackedContainerType = PackedLightComponentContainer<PureComponentType>;

			TYPE_ID_CONSTEXPR TypeID componentTypeID = Type<PureComponentType>::ID();

			auto currentComponent = GetComponent<TComponent>(entityData);
			if (currentComponent != nullptr)
			{
				return currentComponent;
			}

			Archetype* oldArchetype = entityData.m_Archetype;
			const uint32_t indexInOldArchetype = entityData.m_IndexInArchetype;

			Archetype* newArchetype = GetArchetypeAfterAddComponent<TComponent>(entityData.m_Archetype);
			TArchetypeTypeData<PureComponentType> newTypeData = newArchetype->GetComponentTypeData<PureComponentType>();

			PureComponentType* componentPtr = &newTypeData.m_PackedContainer->EmplaceBack(std::forward<Args>(args)...);

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

			// Invoke observer
			if constexpr (InvokeObserver)
			{
				entityData.LockOperations();
				{
					newTypeData.m_ComponentContext->InvokeOnCreate(entity, *componentPtr);
				}
				entityData.UnlockOperations();
			}

			return componentPtr;
		}

		template<light_component_concept ComponentType, typename ...Args>
		inline ComponentType* AddComponent(const Entity& entity, EntityData& entityData, Args&&... args)
		{
			return AddComponent_Impl<true, ComponentType, Args...>(entity, entityData, std::forward<Args>(args)...);
		}

		template<light_component_concept ComponentType, typename ...Args>
		inline ComponentType* AddComponent_NoObserver(const Entity& entity, EntityData& entityData, Args&&... args)
		{
			return AddComponent_Impl<false, ComponentType, Args...>(entity, entityData, std::forward<Args>(args)...);
		}

		bool RemoveComponent_Impl(const Entity& entity, TypeID componentTypeID, bool bInvokeObservers);

		inline bool RemoveComponent(const Entity& entity, TypeID componentTypeID)
		{
			return RemoveComponent_Impl(entity, componentTypeID, true);
		}

		template<light_component_concept ComponentType>
		bool RemoveComponent(const Entity& entity)
		{
			return RemoveComponent_Impl(entity, Type<pure_type_t<ComponentType>>::ID(), true);
		}

		inline bool RemoveComponent_NoObserver(const Entity& entity, TypeID componentTypeID)
		{
			return RemoveComponent_Impl(entity, componentTypeID, false);
		}

		template<light_component_concept ComponentType>
		bool RemoveComponent_NoObserver(const Entity& entity)
		{
			return RemoveComponent_Impl(entity, Type<pure_type_t<ComponentType>>::ID(), false);
		}

		inline bool HasComponentInternal(EntityData& entityData, TypeID typeID) const
		{
			return entityData.m_Archetype != nullptr && entityData.m_Archetype->HasComponentType(typeID);
		}

		template<light_component_concept ComponentType>
		inline bool HasComponent(EntityData& entityData) const
		{
			return HasComponentInternal(entityData, Type<pure_type_t<ComponentType>>::ID());
		}

		template<light_component_concept ComponentType>
		ComponentType* GetComponent(EntityData& entityData) const
		{
			if (entityData.m_Archetype != nullptr)
			{
				return GetComponentFromArchetypeAtIndex<ComponentType>(*entityData.m_Archetype, entityData.m_IndexInArchetype);
			}
			return nullptr;
		}

		template<light_component_concept... ComponentTypes>
		std::tuple<ComponentTypes*...> GetComponents(EntityData& entityData) const
		{
			if (entityData.m_Archetype != nullptr)
			{
				size_t entityIndex = static_cast<size_t>(entityData.m_IndexInArchetype);
				return { GetComponentFromArchetypeAtIndex<pure_type_t<ComponentTypes>>(*entityData.m_Archetype, entityIndex) ... };
			}
			return { static_cast<ComponentTypes*>(nullptr) ... };
		}

		template<light_component_concept ComponentType>
		inline ComponentType* GetComponentFromArchetypeAtIndex(Archetype& archetype, size_t index) const
		{
			PackedLightComponentContainer<ComponentType>* container = archetype.GetTypePackedContainer<ComponentType>();
			return container != nullptr ? container->GetAsPtr(index) : nullptr;
		}

		/// <summary>
		/// Sets component for entity only if entity has this component. If componnet == currentComponent, then it will return true but will not invoke callbacks
		/// </summary>
		/// <typeparam name="ComponentType"></typeparam>
		/// <param name="component"></param>
		/// <returns>true if entity has component of type ComponentType, else false</returns>
		template<bool InvokeObserver, light_component_concept ComponentType>
		bool SetComponent_Impl(EntityData& entityData, const ComponentType& component)
		{
			if (entityData.m_Container != this || entityData.m_Archetype == nullptr)
			{
				return false;
			}

			using PureCompoenentType = pure_type_t<ComponentType>;
			TArchetypeTypeData<PureCompoenentType> typeData = entityData.m_Archetype->GetComponentTypeData<PureCompoenentType>();
			if (!typeData)
			{
				return false;
			}

			ComponentType& currentComponent = typeData.m_PackedContainer->GetAsRef(entityData.m_IndexInArchetype);
			if (currentComponent == component)
			{
				return true;
			}

			typeData.m_PackedContainer->Set(entityData.m_IndexInArchetype, component);

			if constexpr (InvokeObserver)
			{
				entityData.LockOperations();
				{
					typeData.m_ComponentContext->InvokeOnSet(
						Entity(&entityData),
						typeData.m_PackedContainer->GetAsRef(entityData.m_IndexInArchetype)
					);
				}
				entityData.UnlockOperations();
			}

			return true;
		}

		template<light_component_concept ComponentType>
		bool SetComponent(EntityData& entityData, const ComponentType& component)
		{
			return SetComponent_Impl<true, pure_type_t<ComponentType>>(entityData, component);
		}

		template<light_component_concept ComponentType>
		bool SetComponent_NoObserver(EntityData& entityData, const ComponentType& component)
		{
			return SetComponent_Impl<false, pure_type_t<ComponentType>>(entityData, component);
		}

	#pragma endregion

	#pragma region FILTERS
	private:
		template<filter_concept FilterType>
		Archetype* GetArchetypeAfterSetFilter(Archetype* toArchetype, const filter_data_t<FilterType>& filterData)
		{
			if (toArchetype == nullptr)
			{
				return m_ArchetypesMap.GetOrCreateSingleFilterArchetype<FilterType>(filterData);
			}
			else
			{
				return m_ArchetypesMap.GetOrCreateArchetypeAfterSetFilter<FilterType>(*toArchetype, filterData);
			}
		}

		Archetype* GetArchetypeAfterRemoveFilter(Archetype* fromArchetype, TypeID filterID);

		template<filter_concept FilterType>
		bool SetFilter(EntityData& entityData, const filter_data_t<FilterType>& filter)
		{
			if (entityData.OperationsLocked())
			{
				return false;
			}

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
		bool HasFilter(const EntityData& entityData, const filter_data_t<FilterType>& filterData)
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

		template<tag_concept TagType>
		inline bool HasTag(const EntityData& entityData)
		{
			return HasTag(entityData, Type<TagType>::ID());
		}

		template<tag_concept... TagType>
		inline bool HasTags(const EntityData& entityData)
		{
			if (entityData.m_Archetype == nullptr)
			{
				return false;
			}

			return (entityData.m_Archetype->HasTag(Type<TagType>::ID()) && ...);
		}

		template<tag_concept TagType>
		bool AddTag(EntityData& entityData)
		{
			Archetype* oldArchetype = entityData.m_Archetype;
			if (entityData.OperationsLocked()
				|| (oldArchetype != nullptr && oldArchetype->HasTag<TagType>())
				)
			{
				return true;
			}

			TYPE_ID_CONSTEXPR const TypeID tagTypeID = Type<TagType>::ID();

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

		template<tag_concept TagType>
		bool RemoveTag(EntityData& entityData)
		{
			return RemoveTag(entityData, Type<TagType>::ID());
		}

	#pragma endregion

	#pragma region ARCHETYPES:
	private:
		QueryManager m_QueryManager;
		ArchetypesMap m_ArchetypesMap;

	public:
		/// <summary>
		/// Function which destroy empty archetypes
		/// </summary>
		void TryDestroyArchetypes(ArchetypeDestroyState& state, const ArchetypeDestroyConfig& config);

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

		template<light_component_concept... ComponentTypes, typename... TagTypes>
		Archetype* GetArchetypeWithComponentsTags(
			const LightComponentTypeGroup<ComponentTypes...>& components,
			const TagTypeGroup<TagTypes...>& tags
		)
		{
			Archetype* spawnArchetype = nullptr;

			((spawnArchetype = GetArchetypeAfterAddTag(spawnArchetype, Type<tag_type_t<TagTypes>>::ID())), ...);
			((spawnArchetype = GetArchetypeAfterAddComponent<ComponentTypes>(spawnArchetype)), ...);

			return spawnArchetype;
		}

		template<light_component_concept... ComponentTypes, typename... TagTypes, typename... FiltersData>
		Archetype* GetArchetypeWithComponentsTagsFilters(
			const LightComponentTypeGroup<ComponentTypes...>&,
			const TagTypeGroup<TagTypes...>&,
			const std::tuple<FiltersData...>& filtersDataTuple
		)
		{
			Archetype* spawnArchetype = nullptr;

			if constexpr (sizeof...(TagTypes) > 0)
			{
				((spawnArchetype = GetArchetypeAfterAddTag(spawnArchetype, Type<tag_type_t<TagTypes>>::ID())), ...);
			}

			if constexpr (sizeof...(FiltersData) > 0)
			{
				((spawnArchetype = GetArchetypeAfterSetFilter<filter_type_t<pure_type_t<FiltersData>>>(spawnArchetype, std::get<FiltersData>(filtersDataTuple))), ...);
			}

			if constexpr (sizeof...(ComponentTypes) > 0)
			{
				((spawnArchetype = GetArchetypeAfterAddComponent<ComponentTypes>(spawnArchetype)), ...);
			}

			return spawnArchetype;
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

		template<light_component_concept... ComponentTypes, tag_concept... TagTypes>
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