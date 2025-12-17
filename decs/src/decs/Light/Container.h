#pragma once
#include <tuple>

#include "decs/Core.h"
#include "decs/trait.h"
#include "decs/Type.h"
#include "decs/Component/PackedComponentContainer.h"

#include "Archetypes/ArchetypesMap.h"
#include "EntityManager.h"

namespace decs::light
{
	class Entity;

	struct ContainerConfig
	{
	public:
		uint64_t EntityChunkSize = 1000;
		uint64_t ArchetypeChunkSize = 1000;
	};

	class Container
	{
		template<TLightComponentConcept ...Types>
		friend class Query;
		template<TLightComponentConcept ...Types>
		friend class MultiQuery;
		template<TLightComponentConcept...>
		friend class IterationContainerContext;
		friend class Entity;
		friend class ContainerIterator;

		NON_COPYABLE(Container);
		NON_MOVEABLE(Container);

	private:
		static constexpr uint64_t m_DefaultEntitiesChunkSize = 1000;
		static constexpr uint64_t m_DefaultEmptyEntitiesChunkSize = 100;

	public:
		Container();

		Container(const ContainerConfig& config);

		~Container();


	#pragma region Extension data
	public:
		template<typename T>
		void SetExtensionData(T* data)
		{
			m_ExtensionData = static_cast<T*>(data);
		}

		template<typename T>
		[[nodiscard]] T* GetExtensionData()
		{
			return static_cast<T*>(m_ExtensionData);
		}

	private:
		void* m_ExtensionData = nullptr;

	#pragma endregion 

	#pragma region UTILITY
	public:
		/// <summary>
		/// Returns all owned entites to entity manager. Clears all created components. Does not destroy created archetypes and does not clears seted observer manager. This function does not invoke any methods from observers.
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
		Entity CreateEntity();

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
		template<typename InitFunc, TLightComponentConcept... ComponentTypes, TTagConcept... TagTypes>
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
					std::tuple<PackedComponentContainer<drop_const_t<ComponentTypes>>*...> packedContainerTyple = { spawnArchetype->GetTypePackedContainer<drop_const_t<ComponentTypes>>()... };

					for (uint32_t i = 0; i < entityCount; i++)
					{
						if (Entity entity = CreateEntityRaw())
						{
							EntityData* entityData = GetEntityData(entity);
							spawnArchetype->AddEntityData(entityData);

							std::tuple<drop_const_t<ComponentTypes>*...> createdComponents = {
								&std::get<PackedComponentContainer<drop_const_t<ComponentTypes>>*>(packedContainerTyple)->EmplaceBack<>()
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
		template<typename InitFunc, TLightComponentConcept... ComponentTypes, TTagConcept... TagTypes>
			requires light_query_callable<InitFunc, ComponentTypes...>
		[[nodiscard]] Entity CreateEntity(
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
					std::tuple<PackedComponentContainer<drop_const_t<ComponentTypes>>*...> packedContainerTyple = { spawnArchetype->GetTypePackedContainer<drop_const_t<ComponentTypes>>()... };

					if (Entity entity = CreateEntityRaw())
					{

						EntityData* entityData = GetEntityData(entity);
						spawnArchetype->AddEntityData(entityData);

						std::tuple<drop_const_t<ComponentTypes>*...> createdComponents = {
							&std::get<PackedComponentContainer<drop_const_t<ComponentTypes>>*>(packedContainerTyple)->EmplaceBack<>()
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

	private:
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
		template<TLightComponentConcept TComponent, typename ...Args>
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
			PackedComponentContainer<TComponent>* packedContainer = newArchetype->GetTypePackedContainer<drop_const_t<TComponent>>();
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

		template<TLightComponentConcept TComponent>
		bool RemoveComponent(const Entity& entity)
		{
			return RemoveComponent(entity, Type<TComponent>::ID());
		}

		bool RemoveComponent(const Entity& entity, TypeID componentTypeID);

		template<TLightComponentConcept TComponent>
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
					PackedComponentContainer<TComponent>* container = ::decs::check_cast<PackedComponentContainer<TComponent>*>(entityData.m_Archetype->m_TypeData[findTypeIndex].m_PackedContainer);
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

		template<TLightComponentConcept TComponent>
		bool HasComponent(EntityData& entityData) const
		{
			if constexpr (is_tag_v<TComponent>)
			{
				return false;
			}

			return HasComponentInternal(entityData, Type<TComponent>::ID());
		}

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

		template<TTagConcept TTag>
		inline bool HasTag(const EntityData& entityData)
		{
			return HasTag(entityData, Type<TTag>::ID());
		}

		template<TTagConcept TTag>
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

		template<TTagConcept TTag>
		bool RemoveTag(EntityData& entityData)
		{
			return RemoveTag(entityData, Type<TTag>::ID());
		}

	#pragma endregion

	#pragma region ARCHETYPES:
	private:
		ArchetypesMap m_ArchetypesMap{};

	public:
		inline void ShrinkArchetypesToFit()
		{
			m_ArchetypesMap.ShrinkArchetypesToFit();
		}

		inline void ShrinkArchetypesToFit(ArchetypesShrinkToFitState& state)
		{
			m_ArchetypesMap.ShrinkArchetypesToFit(state);
		}

		inline const TChunkedVector<Archetype>& GetArchetypesChunkedVector() const
		{
			return m_ArchetypesMap.GetArchetypesChunkedVector();
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

	#pragma endregion

	};
}