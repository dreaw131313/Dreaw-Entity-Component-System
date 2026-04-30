#pragma once
#include <tuple>

#include "Archetypes/ArchetypesMap.h"
#include "Component/ComponentContextsManager.h"
#include "Component/PackedComponentContainer.h"
#include "Component/StableComponentContainer.h"
#include "Component/Component.h"

#include "Observers/Observers.h"
#include "EntityManager.h"

namespace decs
{
	class Entity;
	class SpawnEntityCallback;

	struct ContainerConfig
	{
	public:
		uint64_t EntityChunkSize = 1000;
		uint64_t DefaultComponentChunkSize = 1000;
		uint64_t ArchetypeChunkSize = 1000;
	};

	class Container final : private NonCopyableNonMoveable
	{
		template<TComponentConcept ...Types>
		friend class Query;
		template<TComponentConcept ...Types>
		friend class MultiQuery;
		template<TComponentConcept...>
		friend class IterationContainerContext;
		friend class Entity;
		template<typename>
		friend class ContainerSerializer;
		friend class ContainerSerializerComplex;
		friend class ContainerIterator;

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
		/// Some functions change internal state of container during invocation and if error will be thrown in any of this functions they can leave invalid internal state of Container object. This function brings back container to valid state.
		/// </summary>
		void ValidateInternalState();

		/// <summary>
		/// Returns all owned entites to entity manager. Clears all created components. Does not destroy created archetypes and does not clears seted observer manager. This function does not invoke any methods from observers.
		/// </summary>
		void Clear();

		/// <summary>
		/// This function marks all entities dead, but do not destroy any component and observers. This is irreversible, and shoould be performed only to make all Entity class objects null.
		/// Iterating over entities is still posible, but changing active state, adding/removing components/tags is forbidden
		/// </summary>
		void MarkEntitiesDead();

	private:
		void ReturnOwnedEntitiesToEntityManager_Internal();

	#pragma endregion

	#pragma region ENTITIES:
	private:
		std::vector<EntityData*> m_EmptyEntities = {};
		EntityManager m_EntityManager{};

		TRefCountHandle<EnityLifeTimeData> m_LifeTimeData{};

	public:
		[[nodiscard]] const TRefCountHandle<EnityLifeTimeData>& GetLifeTimeData() const
		{
			return m_LifeTimeData;
		}

		Entity CreateEntity(bool bIsActive = true);

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
		/// This function ignores component callbacks orders, callbacks are invoked in order of ComponentTypes in ComponentTypeGroup parameter.
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
		template<typename InitFunc, TComponentConcept... ComponentTypes, TTagConcept... TagTypes>
			requires query_callable<InitFunc, ComponentTypes...>
		void CreateEntities(
			const ComponentTypeGroup<ComponentTypes...> components,
			const TagTypeGroup<TagTypes...> tags,
			uint32_t entityCount,
			bool bIsActive,
			InitFunc&& initFunc
		)
		{
			if (entityCount == 0 || !m_CanCreateEntities)
			{
				return;
			}

			if constexpr (sizeof...(TagTypes) == 0 && sizeof...(ComponentTypes) == 0)
			{
				for (uint32_t i = 0; i < entityCount; i++)
				{
					Entity e = CreateEntity(bIsActive);
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
					std::tuple<TArchetypeTypeData<drop_const_t<ComponentTypes>>...> typeDataTuple = { spawnArchetype->GetTypeData<drop_const_t<ComponentTypes>>()... };

					auto invokeComponentObservers = [&]<typename T>(
						const Entity & entity,
						const TArchetypeTypeData<drop_const_t<T>>&typeData,
						drop_const_t<T>*component
						)
					{
						typeData.m_ComponentContext->InvokeOnCreateComponent(component, entity);
						if (IsEntityActive(entity))
						{
							typeData.m_ComponentContext->InvokeOnEnableComponent(component, entity);
						}
					};

					for (uint32_t i = 0; i < entityCount; i++)
					{
						if (Entity entity = CreateEntityRaw(bIsActive))
						{
							EntityData* entityData = GetEntityData(entity);
							spawnArchetype->AddEntityData(entityData);

							std::tuple<drop_const_t<ComponentTypes>*...> createdComponents = { std::get<TArchetypeTypeData<drop_const_t<ComponentTypes>>>(typeDataTuple).m_StableContainer->Create()... };
							(std::get<drop_const_t<ComponentTypes>*>(createdComponents)->OnPreCreate(entityData), ...);
							(std::get<TArchetypeTypeData<drop_const_t<ComponentTypes>>>(typeDataTuple).m_PackedContainer->PushBack(std::get<drop_const_t<ComponentTypes>*>(createdComponents)), ...);

							InvokeEntityCreateObserver_Internal(entity);
							if (bIsActive)
							{
								InvokeEntityEnableObserver_Internal(entity);
							}

							(invokeComponentObservers.operator () < drop_const_t<ComponentTypes> > (
								entity,
								std::get<TArchetypeTypeData<drop_const_t<ComponentTypes>>>(typeDataTuple),
								std::get<drop_const_t<ComponentTypes>*>(createdComponents)
								), ...);

							if constexpr (is_invocable_with_entity_v<InitFunc, ComponentTypes...>)
							{
								initFunc(entity, *std::get<ComponentTypes*>(createdComponents)...);
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

		template<typename InitFunc, TComponentConcept... ComponentTypes>
			requires query_callable<InitFunc, ComponentTypes...>
		inline void CreateEntities(
			const ComponentTypeGroup<ComponentTypes...> components,
			uint32_t entityCount,
			bool bIsActive,
			InitFunc&& initFunc
		)
		{
			constexpr const TagTypeGroup<> emptyTagTypeGroup{};
			CreateEntities(components, emptyTagTypeGroup, entityCount, bIsActive, initFunc);
		}

		/// <summary>
		/// This function ignores component callbacks orders, callbacks are invoked in order of ComponentTypes in ComponentTypeGroup parameter.
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
		template<typename InitFunc, TComponentConcept... ComponentTypes, TTagConcept... TagTypes>
			requires query_callable<InitFunc, ComponentTypes...>
		Entity CreateEntity(
			const ComponentTypeGroup<ComponentTypes...> components,
			const TagTypeGroup<TagTypes...> tags,
			bool bIsActive,
			InitFunc&& initFunc
		)
		{
			if (!m_CanCreateEntities)
			{
				return Entity();
			}

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
					std::tuple<TArchetypeTypeData<drop_const_t<ComponentTypes>>...> typeDataTuple = { spawnArchetype->GetTypeData<drop_const_t<ComponentTypes>>()... };

					auto invokeComponentObservers = [&]<typename T>(
						const Entity & entity,
						const TArchetypeTypeData<drop_const_t<T>>&typeData,
						drop_const_t<T>*component
						)
					{
						typeData.m_ComponentContext->InvokeOnCreateComponent(component, entity);
						if (IsEntityActive(entity))
						{
							typeData.m_ComponentContext->InvokeOnEnableComponent(component, entity);
						}
					};

					if (Entity entity = CreateEntityRaw(bIsActive))
					{
						EntityData* entityData = GetEntityData(entity);
						spawnArchetype->AddEntityData(entityData);

						std::tuple<drop_const_t<ComponentTypes>*...> createdComponents = { std::get<TArchetypeTypeData<drop_const_t<ComponentTypes>>>(typeDataTuple).m_StableContainer->Create()... };
						(std::get<drop_const_t<ComponentTypes>*>(createdComponents)->OnPreCreate(entityData), ...);
						(std::get<TArchetypeTypeData<drop_const_t<ComponentTypes>>>(typeDataTuple).m_PackedContainer->PushBack(std::get<drop_const_t<ComponentTypes>*>(createdComponents)), ...);

						InvokeEntityCreateObserver_Internal(entity);
						if (bIsActive)
						{
							InvokeEntityEnableObserver_Internal(entity);
						}

						(invokeComponentObservers.operator () < drop_const_t<ComponentTypes> > (
							entity,
							std::get<TArchetypeTypeData<drop_const_t<ComponentTypes>>>(typeDataTuple),
							std::get<drop_const_t<ComponentTypes>*>(createdComponents)
							), ...);

						if constexpr (is_invocable_with_entity_v<InitFunc, ComponentTypes...>)
						{
							initFunc(entity, *std::get<ComponentTypes*>(createdComponents)...);
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

		template<typename InitFunc, TComponentConcept... ComponentTypes>
			requires query_callable<InitFunc, ComponentTypes...>
		inline Entity CreateEntity(
			const ComponentTypeGroup<ComponentTypes...> components,
			bool bIsActive,
			InitFunc&& initFunc
		)
		{
			constexpr const TagTypeGroup<> emptyTagTypeGroup{};
			return CreateEntity(components, emptyTagTypeGroup, bIsActive, initFunc);
		}


	private:
		void InitializeLifeTimeData();

		void DestroyLifeTimeData();

		bool DestroyEntityInternal(const Entity& entity, bool bInvokeObservers);

		void SetEntityActive(const Entity& entity, bool bIsActive);

		EntityData* GetEntityData(const Entity& entity) const;

		bool IsEntityActive(const Entity& entity) const;

		Entity CreateEntityRaw(bool bIsActive);

	public:
		/// <summary>
		/// Needed for Try Engine to make hierarchies active state changes correct
		/// </summary>
		/// <param name="entity"></param>
		/// <param name="bIsActive"></param>
		void SetEntityActiveOverride(const Entity& entity, bool bIsActiveOverride);

		void SetEntityDisabledOverrideCount(const Entity& entity, uint32_t disabledOverrideCount);

		void ResetDisabledOverrideCount(const Entity& entity);

		uint32_t GetEntityActiveOverrides(const Entity& entity);

	private:
		void AddToEmptyEntitiesRightAfterNewEntityCreation(EntityData& data);

		void AddToEmptyEntities(EntityData& data);

		void RemoveFromEmptyEntities(EntityData& data);

		void InvokeEntityComponentDestructionObservers(const Entity& entity);

	#pragma endregion

	#pragma region SPAWNING ENTITIES:
	private:
		struct SpawnComponentData
		{
		public:
			const EntityComponent* m_PrefabComponent = nullptr;
			IStableComponentContainer* m_SpawnStableContainer = nullptr;
			IComponentContext* m_SpawnedComponentContext = nullptr;
			EntityComponent* m_SpawnedComponent = nullptr;

		public:
			SpawnComponentData()
			{

			}

			SpawnComponentData(
				const EntityComponent* prefabComponent,
				IStableComponentContainer* spawnStableContainer,
				IComponentContext* spawnedComponentContext
			):
				m_PrefabComponent(prefabComponent),
				m_SpawnStableContainer(spawnStableContainer),
				m_SpawnedComponentContext(spawnedComponentContext)
			{

			}

			inline bool IsTag() const
			{
				return m_PrefabComponent == nullptr;
			}
		};

		struct SpawnData
		{
		public:
			std::vector<SpawnComponentData> m_ComponentData;

		public:
			void Reserve(uint64_t size)
			{
				m_ComponentData.reserve(size);
			}

			void Clear()
			{
				m_ComponentData.clear();
			}

			void PopBackSpawnState(uint64_t refsStartIdx)
			{
				auto pIt = m_ComponentData.begin();
				std::advance(pIt, refsStartIdx);
				m_ComponentData.erase(pIt, m_ComponentData.end());

			}
		};

		struct SpawnDataState
		{
		public:
			uint32_t m_ComponentDataStart;

		public:
			SpawnDataState(SpawnData& spawnData):
				m_ComponentDataStart((uint32_t)spawnData.m_ComponentData.size())
			{

			}
		};

	private:
		SpawnData m_SpawnData = {};

	public:
		Entity Spawn(
			const Entity& prefab,
			bool bIsActive = true
		);

		bool Spawn(
			const Entity& prefab,
			uint64_t spawnCount,
			bool areActive = true
		);

		bool Spawn(
			const Entity& prefab,
			std::vector<Entity>& spawnedEntities,
			uint64_t spawnCount,
			bool areActive = true
		);

	private:
		void PrepareSpawnDataFromPrefab(
			const EntityData& prefabEntityData,
			const Container& prefabContainer,
			Archetype*& spawnArchetype
		);

		void CreateEntityFromSpawnData(
			const SpawnDataState& spawnState,
			const Entity& spawnedEntity,
			Archetype& spawnArchetype
		);

		void InvokeComponentCreateAndEnableObserversOnSpawn(const Entity& entity, const Archetype& archetype, const SpawnDataState& spawnState);

	#pragma endregion

	#pragma region COMPONENTS:
	private:
		ComponentContextsManager m_ComponentContextManager = { 1000 };

	private:
		void OnAddComponentInvokeObservers(
			const Entity& entity,
			IComponentContext* componentContext,
			IPackedComponentContainer* packedContainer,
			TypeID compTypeID
		);


		template<TComponentConcept TComponent, typename ...Args>
		TComponent* AddComponent(const Entity& entity, EntityData& entityData, Args&&... args)
		{
			if (!m_CanAddComponents || !entityData.IsValidToPerformComponentOperation())
			{
				return nullptr;
			}

			TYPE_ID_CONSTEXPR TypeID componentTypeID = Type<TComponent>::ID();

			auto currentComponent = GetComponentWithoutCheckingIsAlive<TComponent>(entityData);
			if (currentComponent != nullptr)
			{
				return currentComponent;
			}

			Archetype* oldArchetype = entityData.m_Archetype;
			const uint32_t indexInOldArchetype = entityData.m_IndexInArchetype;

			Archetype* newArchetype = GetArchetypeAfterAddComponent<TComponent>(oldArchetype);
			uint32_t componentTypeIndex = newArchetype->FindTypeIndex<TComponent>();
			ArchetypeTypeData& archetypeTypeData = newArchetype->m_TypeData[componentTypeIndex];

			// Adding component to stable component container
			StableComponentContainer<TComponent>* stableContainer = ::decs::check_cast<StableComponentContainer<TComponent>*>(archetypeTypeData.m_StableContainer);
			TComponent* componentPtr = stableContainer->Create(std::forward<Args>(args)...);
			// Adding component pointer to packed container in archetype
			archetypeTypeData.m_PackedContainer->PushBackFromBase(componentPtr);

			// Adding entity to archetype
			if (oldArchetype != nullptr)
			{
				if (m_PerformDelayedDestruction)
				{
					Archetype::MoveEntityAfterAddComponentWithoutDestroyingFromSource(*oldArchetype, *newArchetype, indexInOldArchetype, componentTypeID);
					AddArchetypeRecordToDelayedRemove(oldArchetype, indexInOldArchetype, false, componentTypeID);
				}
				else
				{
					Archetype::MoveEntityComponentsAfterAddComponent(*oldArchetype, *newArchetype, indexInOldArchetype, componentTypeID);
				}
			}
			else
			{
				RemoveFromEmptyEntities(entityData);
				newArchetype->AddEntityData(&entityData);
			}

			static_cast<EntityComponent*>(componentPtr)->OnPreCreate(&entityData);

			OnAddComponentInvokeObservers(entity, archetypeTypeData.m_ComponentContext, archetypeTypeData.m_PackedContainer, componentTypeID);

			return componentPtr;
		}

		template<TComponentConcept TComponent>
		bool RemoveComponent(const Entity& entity)
		{
			return RemoveComponent(entity, Type<TComponent>::ID());
		}

		bool RemoveComponent(const Entity& entity, TypeID componentTypeID);

	private:
		void InvokeComponentDestroyObservers(IComponentContext& compCtx, EntityComponent& comp, EntityData& entityData);

	public:
		template<TComponentConcept TComponent>
		TComponent* GetComponentWithoutCheckingIsAlive(EntityData& entityData) const
		{
			if (entityData.m_Archetype != nullptr)
			{
				PackedStableComponentContainer<TComponent>* packedContainer = entityData.m_Archetype->GetTypePackedContainer<TComponent>();
				if (packedContainer != nullptr)
				{
					return packedContainer->GetAsPtr(entityData.m_IndexInArchetype);
				}
			}
			return nullptr;
		}

		template<TComponentConcept TComponent>
		TComponent* GetComponent(EntityData& entityData) const
		{
			if (entityData.IsAlive())
			{
				return GetComponentWithoutCheckingIsAlive<TComponent>(entityData);
			}
			return nullptr;
		}

		EntityComponent* GetComponent(EntityData& entityData, TypeID componentType) const
		{
			if (entityData.m_Archetype != nullptr && entityData.IsAlive())
			{
				IPackedComponentContainer* packdContainer = entityData.m_Archetype->GetTypePackedContainer(componentType);
				if (packdContainer != nullptr)
				{
					return packdContainer->GetComponentBasePtr(entityData.m_IndexInArchetype);
				}
			}
			return nullptr;
		}

		/// <summary>
		/// 
		/// </summary>
		/// <param name="entityData"></param>
		/// <param name="componentIndex"></param>
		/// <returns>Component in order of observeres</returns>
		EntityComponent* GetComponentAtIndex_ObserversOrder(EntityData& entityData, uint32_t componentIndex);

		/// <summary>
		/// Gets component by index in order of typeID. Uses std::vector where component and tags records data are placed. It can return nullptr if componentIndex is greater than component and tag count in archetype or where index points to tag instead of component.
		/// </summary>
		/// <param name="entityData"></param>
		/// <param name="componentIndex"></param>
		/// <returns></returns>
		EntityComponent* GetComponentAtIndex_TypeIDOrder(EntityData& entityData, uint32_t componentIndex);

		template<TComponentConcept TComponent>
		TComponent* GetComponentDynamic(EntityData& entityData)
		{
			if (entityData.m_Archetype != nullptr && entityData.IsAlive())
			{
				uint32_t archetypeComponentCount = entityData.m_Archetype->GetComponentAndTagCount();
				const auto& typeDataVector = entityData.m_Archetype->m_TypeData;
				for (uint32_t i = 0; i < archetypeComponentCount; i++)
				{
					auto& typeData = typeDataVector[i];
					if (!typeData.IsTag())
					{
						auto componentPtr = typeData.m_PackedContainer->GetComponentBasePtr(entityData.m_IndexInArchetype);
						TComponent* casted = dynamic_cast<TComponent*>(componentPtr);
						if (casted != nullptr)
						{
							return casted;
						}
					}
				}
			}

			return nullptr;
		}

		template<TComponentConcept TComponent>
		void GetComponentsDynamic(EntityData& entityData, std::vector<TComponent*>& outComponents)
		{
			if (entityData.m_Archetype != nullptr && entityData.IsAlive())
			{
				uint32_t archetypeComponentCount = entityData.m_Archetype->GetComponentAndTagCount();
				const auto& typeDataVector = entityData.m_Archetype->m_TypeData;
				for (uint32_t i = 0; i < archetypeComponentCount; i++)
				{
					auto& typeData = typeDataVector[i];
					if (!typeData.IsTag())
					{
						auto componentPtr = typeData.m_PackedContainer->GetComponentBasePtr(entityData.m_IndexInArchetype);
						TComponent* casted = dynamic_cast<TComponent*>(componentPtr);
						if (casted != nullptr)
						{
							outComponents.push_back(casted);
						}
					}
				}
			}
		}

		bool HasComponentInternal(EntityData& entityData, TypeID typeID) const
		{
			if (entityData.m_Archetype != nullptr)
			{
				return entityData.m_Archetype->HasComponentType(typeID);
			}
			return false;
		}

		template<TComponentConcept TComponent>
		bool HasComponent(EntityData& entityData) const
		{
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
			if (!entityData.IsAlive() || entityData.m_Archetype == nullptr)
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
			if (!m_CanAddComponents || !entityData.IsValidToPerformComponentOperation())
			{
				return HasTag<TTag>(entityData);
			}

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
				if (m_PerformDelayedDestruction)
				{
					Archetype::MoveEntityAfterAddComponentWithoutDestroyingFromSource(*oldArchetype, *newArchetype, indexInOldArchetype, tagTypeID);
					AddArchetypeRecordToDelayedRemove(entityData.m_Archetype, entityData.m_IndexInArchetype, false, tagTypeID);
				}
				else
				{
					Archetype::MoveEntityComponentsAfterAddComponent(*oldArchetype, *newArchetype, indexInOldArchetype, tagTypeID);
				}
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

	#pragma region STABLE COMPONENTS:
	public:
		template<TComponentConcept T>
		bool SetComponentChunkSize(uint32_t chunkSize)
		{
			return m_ComponentContextManager.SetComponentChunkSize<T>(chunkSize);
		}

		bool SetComponentChunkSize(TypeID typeID, uint32_t chunkSize)
		{
			return m_ComponentContextManager.SetComponentChunkSize(typeID, chunkSize);
		}

		template<TComponentConcept T>
		uint64_t GetComponentChunkSize()
		{
			return m_ComponentContextManager.GetComponentChunkSize<T>();
		}

		uint64_t GetComponentChunkSize(TypeID typeID)
		{
			return m_ComponentContextManager.GetComponentChunkSize(typeID);
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
				entityNewArchetype = m_ArchetypesMap.GetSingleComponentArchetype(addedComponentTypeID);
				if (entityNewArchetype == nullptr)
				{
					auto compCtx = m_ComponentContextManager.GetOrCreateComponentContext<TComponent>();
					entityNewArchetype = m_ArchetypesMap.CreateSingleComponentArchetype(
						addedComponentTypeID,
						compCtx
					);
				}
			}
			else
			{
				entityNewArchetype = m_ArchetypesMap.GetArchetypeAfterAddComponent<TComponent>(*toArchetype);
				if (entityNewArchetype == nullptr)
				{
					auto compCtx = m_ComponentContextManager.GetOrCreateComponentContext<TComponent>();
					entityNewArchetype = m_ArchetypesMap.CreateArchetypeAfterAddComponent(
						*toArchetype,
						addedComponentTypeID,
						compCtx
					);
				}
			}

			return entityNewArchetype;
		}

	#pragma endregion

	#pragma region OBSERVERS:
	public:
		void InvokeEntitesOnCreateListeners();

		void InvokeEntitesOnDestroyListeners(bool bMarkEntitiesDead = true);

		/// <summary>
		/// Changes order of invoking function of component observers. Callback for component with lower order will be invoked first.
		/// </summary>
		/// <typeparam name="ComponentType"></typeparam>
		/// <param name="order"></param>
		template<TComponentConcept TComponent>
		void SetComponentOrder(int order)
		{
			if (m_ComponentContextManager.SetComponentOrder<TComponent>(order))
			{
				// sort order of observers in all archetypes that contain ComponentType
				m_ArchetypesMap.UpdateOrderInAllArchetypesWithComponentType<TComponent>();
			}
		}

		/// <summary>
		/// Changes order of invoking function of component observers. Callback for component with lower order will be invoked first.
		/// </summary>
		/// <typeparam name="ComponentType"></typeparam>
		/// <param name="order"></param>
		void SetComponentOrder(TypeID typeID, int order)
		{
			if (m_ComponentContextManager.SetComponentOrder(typeID, order))
			{
				// sort order of observers in all archetypes that contain ComponentType
				m_ArchetypesMap.UpdateOrderInAllArchetypesWithComponentType(typeID);
			}
		}

		inline bool HasEntityCreateObserver() const
		{
			return m_CreateEntityObserver != nullptr;
		}

		inline decs::CreateEntityObserver* GetEntityCreateObserver()
		{
			return m_CreateEntityObserver;
		}

		inline bool HasEntityDestroyObserver() const
		{
			return m_DestroyEntityObserver != nullptr;
		}

		inline decs::DestroyEntityObserver* GetEntityDestroyObserver()
		{
			return m_DestroyEntityObserver;
		}

		inline void SetCreateEntityObserver(CreateEntityObserver* createEntityObserver)
		{
			m_CreateEntityObserver = createEntityObserver;
		}

		inline void SetDestroyEntityObserver(DestroyEntityObserver* destroyEntityObserver)
		{
			m_DestroyEntityObserver = destroyEntityObserver;
		}

		inline void SetEnableEntityObserver(EnableEntityObserver* enableEntityObserver)
		{
			m_EnableEntityObserver = enableEntityObserver;
		}

		inline void SetDisableEntityObserver(DisableEntityObserver* disableEntityObserver)
		{
			m_DisableEntityObserver = disableEntityObserver;
		}

		inline void SetEntityObservers(
			CreateEntityObserver* createEntityObserver,
			DestroyEntityObserver* destroyEntityObserver,
			EnableEntityObserver* enableEntityObserver,
			DisableEntityObserver* disableEntityObserver
		)
		{
			m_CreateEntityObserver = createEntityObserver;
			m_DestroyEntityObserver = destroyEntityObserver;
			m_EnableEntityObserver = enableEntityObserver;
			m_DisableEntityObserver = disableEntityObserver;
		}

		template<TComponentConcept TComponent>
		void SetComponentObservers(
			CreateComponentObserver<TComponent>* createObserver,
			DestroyComponentObserver<TComponent>* destroyObserver,
			EnableComponentObserver<TComponent>* enableObserver,
			DisableComponentObserver<TComponent>* disableObserver
		)
		{
			auto componentContext = m_ComponentContextManager.GetOrCreateComponentContext<TComponent>();
			componentContext->m_Observers.m_CreateObserver = createObserver;
			componentContext->m_Observers.m_DestroyObserver = destroyObserver;
			componentContext->m_Observers.m_EnableObserver = enableObserver;
			componentContext->m_Observers.m_DisableObserver = disableObserver;
		}

		template<TComponentConcept TComponent>
		void SetCreateComponentObserver(CreateComponentObserver<TComponent>* createObserver)
		{
			auto componentContext = m_ComponentContextManager.GetOrCreateComponentContext<TComponent>();
			componentContext->m_Observers.m_CreateObserver = createObserver;
		}

		template<TComponentConcept TComponent>
		void SetDestroyComponentObserver(DestroyComponentObserver<TComponent>* destroyObserver)
		{
			auto componentContext = m_ComponentContextManager.GetOrCreateComponentContext<TComponent>();
			componentContext->m_Observers.m_DestroyObserver = destroyObserver;
		}

		template<TComponentConcept TComponent>
		void SetCreateDestroyComponentObservers(
			CreateComponentObserver<TComponent>* createObserver,
			DestroyComponentObserver<TComponent>* destroyObserver
		)
		{
			auto componentContext = m_ComponentContextManager.GetOrCreateComponentContext<TComponent>();
			componentContext->m_Observers.m_CreateObserver = createObserver;
			componentContext->m_Observers.m_DestroyObserver = destroyObserver;
		}

		template<TComponentConcept TComponent>
		void SetEnableComponentObserver(EnableComponentObserver<TComponent>* enableObserver)
		{
			auto componentContext = m_ComponentContextManager.GetOrCreateComponentContext<TComponent>();
			componentContext->m_Observers.m_EnableObserver = enableObserver;
		}

		template<TComponentConcept TComponent>
		void SetDisableComponentObserver(DisableComponentObserver<TComponent>* disableObserver)
		{
			auto componentContext = m_ComponentContextManager.GetOrCreateComponentContext<TComponent>();
			componentContext->m_Observers.m_DisableObserver = disableObserver;
		}

		template<TComponentConcept TComponent>
		void SetEnableDisableComponentObservers(
			EnableComponentObserver<TComponent>* enableObserver,
			DisableComponentObserver<TComponent>* disableObserver
		)
		{
			auto componentContext = m_ComponentContextManager.GetOrCreateComponentContext<TComponent>();
			componentContext->m_Observers.m_EnableObserver = enableObserver;
			componentContext->m_Observers.m_DisableObserver = disableObserver;
		}

		/// <summary>
		/// Invokes only entity Create and Enable (if entity is enabled). If callbacks was invoked earliers then callbacks will not be invoked. This function should be used with CreateEntity_NoObserver method.
		/// </summary>
		/// <param name="entity"></param>
		void InvokeEntityCreateEnableObservers(const decs::Entity& entity);

	private:
		std::vector<EntityComponent*> m_ActivationChangeComponentPtrs = {};

		CreateEntityObserver* m_CreateEntityObserver = nullptr;
		DestroyEntityObserver* m_DestroyEntityObserver = nullptr;
		EnableEntityObserver* m_EnableEntityObserver = nullptr;
		DisableEntityObserver* m_DisableEntityObserver = nullptr;

	private:
		void InvokeEntityCreateObserver_Internal(const Entity& entity);

		void InvokeEntityDestroyObserver_Internal(const Entity& entity);

		void InvokeEntityEnableObserver_Internal(const Entity& entity);

		void InvokeEntityDisableObserver_Internal(const Entity& entity);

		void InvokeEntityAndComponentEnableObservers_Internal(const Entity& entity);

		void InvokeEntityAndComponentsDisableObservers_Internal(const Entity& entity);

	#pragma endregion

	#pragma region DELAYED DESTROY:
	public:
		/// <summary>
		/// This function clean all entites which was destroyed. It do not invoke any callback observers it only cleans records in archetypes.
		/// </summary>
		/// <param name="maxEntitiesToDestroy"></param>
		void PerformDelayedDestroy(uint64_t maxEntitiesToDestroy = 0);

	private:
		struct DelayedEntityToDestroy
		{
		public:
			EntityData* entityData = nullptr;
			bool bInvokeCallbacks = true;
		};

		std::vector<DelayedEntityToDestroy> m_DelayedEntitiesToDestroy;

		struct ArchetypeRecordDelayedDestroyData
		{
			Archetype* archetype;
			TypeID removedComponentTypeID;
			uint32_t index;
			// if true then stable component of type  "removedComponentTypeID" is also destroyed, if false (after adding component) removed are only record from archetype 
			bool bRemoveAfterRemoveComponent;
		};

		std::vector<ArchetypeRecordDelayedDestroyData> m_ArchetypesRecordsToDelayedRemove = {};

		bool m_PerformDelayedDestruction = false;

	private:
		void PerformDelayedDestruction();

		void DestroyDelayedEntities();

		void RemoveArchetypesRecordsDelayedToRemove();

		void DestroyDelayedEntity(const Entity& entity, bool bInvokeCallbacks);

		void AddEntityToDelayedDestroy(const Entity& entity, bool bInvokeCallbacks);

		void AddArchetypeRecordToDelayedRemove(Archetype* archetype, uint32_t index, bool bRemoveAfterRemoveComponent, TypeID removedComponentTypeID)
		{
			archetype->SetRecordAsIntendedToDelayedDestroy(index);

			m_ArchetypesRecordsToDelayedRemove.push_back({ archetype, removedComponentTypeID, index, bRemoveAfterRemoveComponent });
		}

	#pragma endregion

	#pragma region FLAGS:
	private:
		struct BoolSwitch final
		{
		public:
			BoolSwitch(bool& boolToSwitch):
				m_Bool(boolToSwitch),
				m_FinalValue(!boolToSwitch)
			{
			}

			BoolSwitch(bool& boolToSwitch, const bool& startValue):
				m_Bool(boolToSwitch),
				m_FinalValue(!startValue)
			{
				m_Bool = startValue;
			}

			BoolSwitch(const BoolSwitch&) = delete;
			BoolSwitch(BoolSwitch&&) = delete;

			BoolSwitch& operator=(const BoolSwitch&) = delete;
			BoolSwitch& operator=(BoolSwitch&&) = delete;

			~BoolSwitch()
			{
				m_Bool = m_FinalValue;
			}

		private:
			bool& m_Bool;
			bool m_FinalValue;
		};

	private:
		bool m_IsInvokingObserversCallbacks = false;
		bool m_CanCreateEntities = true;
		bool m_CanDestroyEntities = true;
		bool m_CanSpawn = true;
		bool m_CanAddComponents = true;
		bool m_CanRemoveComponents = true;

	#pragma endregion

	#pragma region NO CALLBACKS methods:
	public:
		Entity CreateEntity_NoObserver(bool bIsActive = true);


		/// <summary>
		/// This function ignores component callbacks orders, callbacks are invoked in order of ComponentTypes in ComponentTypeGroup parameter.
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
		template<typename InitFunc, TComponentConcept... ComponentTypes, TTagConcept... TagTypes>
			requires query_callable<InitFunc, ComponentTypes...>
		Entity CreateEntity_NoObserver(
			const ComponentTypeGroup<ComponentTypes...> components,
			const TagTypeGroup<TagTypes...> tags,
			bool bIsActive,
			InitFunc&& initFunc
		)
		{
			if (!m_CanCreateEntities)
			{
				return Entity();
			}

			if constexpr (sizeof...(TagTypes) == 0 && sizeof...(ComponentTypes) == 0)
			{
				if (Entity e = CreateEntity_NoObserver())
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
					std::tuple<TArchetypeTypeData<drop_const_t<ComponentTypes>>...> typeDataTuple = { spawnArchetype->GetTypeData<drop_const_t<ComponentTypes>>()... };

					if (Entity entity = CreateEntityRaw(bIsActive))
					{
						EntityData* entityData = GetEntityData(entity);
						spawnArchetype->AddEntityData(entityData);

						std::tuple<drop_const_t<ComponentTypes>*...> createdComponents = { std::get<TArchetypeTypeData<drop_const_t<ComponentTypes>>>(typeDataTuple).m_StableContainer->Create()... };
						(std::get<drop_const_t<ComponentTypes>*>(createdComponents)->OnPreCreate(entityData), ...);
						(std::get<TArchetypeTypeData<drop_const_t<ComponentTypes>>>(typeDataTuple).m_PackedContainer->PushBack(std::get<drop_const_t<ComponentTypes>*>(createdComponents)), ...);

						if constexpr (is_invocable_with_entity_v<InitFunc, ComponentTypes...>)
						{
							initFunc(entity, *std::get<ComponentTypes*>(createdComponents)...);
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


		template<typename InitFunc, TComponentConcept... ComponentTypes>
			requires query_callable<InitFunc, ComponentTypes...>
		Entity CreateEntity_NoObserver(
			const ComponentTypeGroup<ComponentTypes...> components,
			bool bIsActive,
			InitFunc&& initFunc
		)
		{
			constexpr const TagTypeGroup<> emptyTagTypeGroup{};
			return CreateEntity_NoObserver(components, emptyTagTypeGroup, bIsActive, initFunc);
		}


		/// <summary>
		/// This function ignores component callbacks orders, callbacks are invoked in order of ComponentTypes in ComponentTypeGroup parameter.
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
		template<typename InitFunc, TComponentConcept... ComponentTypes, TTagConcept... TagTypes>
			requires query_callable<InitFunc, ComponentTypes...>
		void CreateEntities_NoObserver(
			const ComponentTypeGroup<ComponentTypes...> components,
			const TagTypeGroup<TagTypes...> tags,
			uint32_t entityCount,
			bool bIsActive,
			InitFunc&& initFunc
		)
		{
			if (entityCount == 0 || !m_CanCreateEntities)
			{
				return;
			}

			if constexpr (sizeof...(TagTypes) == 0 && sizeof...(ComponentTypes) == 0)
			{
				for (uint32_t i = 0; i < entityCount; i++)
				{
					Entity e = CreateEntity_NoObserver(bIsActive);
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
					std::tuple<TArchetypeTypeData<drop_const_t<ComponentTypes>>...> typeDataTuple = { spawnArchetype->GetTypeData<drop_const_t<ComponentTypes>>()... };

					for (uint32_t i = 0; i < entityCount; i++)
					{
						if (Entity entity = CreateEntityRaw(bIsActive))
						{
							EntityData* entityData = GetEntityData(entity);
							spawnArchetype->AddEntityData(entityData);

							std::tuple<drop_const_t<ComponentTypes>*...> createdComponents = { std::get<TArchetypeTypeData<drop_const_t<ComponentTypes>>>(typeDataTuple).m_StableContainer->Create()... };
							(std::get<drop_const_t<ComponentTypes>*>(createdComponents)->OnPreCreate(entityData), ...);
							(std::get<TArchetypeTypeData<drop_const_t<ComponentTypes>>>(typeDataTuple).m_PackedContainer->PushBack(std::get<drop_const_t<ComponentTypes>*>(createdComponents)), ...);

							if constexpr (is_invocable_with_entity_v<InitFunc, ComponentTypes...>)
							{
								initFunc(entity, *std::get<ComponentTypes*>(createdComponents)...);
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
		template<typename InitFunc, TComponentConcept... ComponentTypes>
			requires query_callable<InitFunc, ComponentTypes...>
		inline void CreateEntities_NoObserver(
			const ComponentTypeGroup<ComponentTypes...> components,
			uint32_t entityCount,
			bool bIsActive,
			InitFunc&& initFunc
		)
		{
			constexpr const TagTypeGroup<> tags{};
			return CreateEntities_NoObserver(components, tags, entityCount, bIsActive, initFunc);
		}

		bool DestroyEntity_NoObserver(const Entity& entity);

		Entity Spawn_NoObserver(
			const Entity& prefab,
			bool bIsActive = true
		);

		bool Spawn_NoObserver(
			const Entity& prefab,
			uint64_t spawnCount,
			bool bAreActive = true
		);

		bool Spawn_NoObserver(
			const Entity& prefab,
			std::vector<Entity>& spawnedEntities,
			uint64_t spawnCount,
			bool bAreActive = true
		);

	private:
		void SetEntityActive_NoObserver(const Entity& entity, bool bIsActive);

		template<TComponentConcept TComponent, typename ...Args>
		TComponent* AddComponent_NoObserver(const Entity& entity, EntityData& entityData, Args&&... args)
		{
			if constexpr (is_tag_v<TComponent>)
			{
				return nullptr;
			}

			if (!m_CanAddComponents) return nullptr;

			if (!entityData.IsValidToPerformComponentOperation())
			{
				return nullptr;
			}

			auto currentComponent = GetComponentWithoutCheckingIsAlive<TComponent>(entityData);
			if (currentComponent != nullptr)
			{
				return currentComponent;
			}

			TYPE_ID_CONSTEXPR TypeID copmonentTypeID = Type<TComponent>::ID();

			Archetype* oldArchetype = entityData.m_Archetype;
			const uint32_t indexInOldArchetype = entityData.m_IndexInArchetype;

			Archetype* newArchetype = GetArchetypeAfterAddComponent<TComponent>(entityData.m_Archetype);
			uint32_t componentTypeIndex = newArchetype->FindTypeIndex<TComponent>();
			ArchetypeTypeData& archetypeTypeData = newArchetype->m_TypeData[componentTypeIndex];

			// Adding component to stable component container
			StableComponentContainer<TComponent>* stableContainer = ::decs::check_cast<StableComponentContainer<TComponent>*>(archetypeTypeData.m_StableContainer);
			TComponent* componentPtr = stableContainer->Create(std::forward<Args>(args)...);

			//StableComponentRef componentNodeInfo = {};
			// Adding component pointer to packed container in archetype
			archetypeTypeData.m_PackedContainer->PushBackFromBase(componentPtr);

			if (oldArchetype != nullptr)
			{
				if (m_PerformDelayedDestruction)
				{
					Archetype::MoveEntityAfterAddComponentWithoutDestroyingFromSource(*oldArchetype, *newArchetype, indexInOldArchetype, copmonentTypeID);
					AddArchetypeRecordToDelayedRemove(entityData.m_Archetype, entityData.m_IndexInArchetype, false, copmonentTypeID);

				}
				else
				{
					Archetype::MoveEntityComponentsAfterAddComponent(*oldArchetype, *newArchetype, indexInOldArchetype, copmonentTypeID);
				}
			}
			else
			{
				RemoveFromEmptyEntities(entityData);
				newArchetype->AddEntityData(&entityData);
			}

			static_cast<EntityComponent*>(componentPtr)->OnPreCreate(&entityData);

			return componentPtr;
		}

		template<TComponentConcept TComponent>
		bool RemoveComponent_NoObserver(const Entity& entity)
		{
			if constexpr (is_tag_v<TComponent>)
			{
				return false;
			}
			if (!m_CanRemoveComponents)
			{
				return false;
			}

			return RemoveComponent_NoObserver(entity, Type<TComponent>::ID());
		}

		bool RemoveComponent_NoObserver(const Entity& entity, TypeID componentTypeID);

	public:
		void SetEntityActiveOverride_NoObserver(const Entity& entity, bool bIsActiveOverride);

		void SetEntityDisabledOverrideCount_NoObserver(const Entity& entity, uint32_t disabledOverrideCount);

		void ResetDisabledOverrideCount_NoObserver(const Entity& entity);

	#pragma endregion

	};
}