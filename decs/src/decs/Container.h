#pragma once
#include "Core.h"
#include "Type.h"

#include <tuple>

#include "Archetypes/ArchetypesMap.h"
#include "Component/ComponentContextsManager.h"
#include "Component/PackedComponentContainer.h"
#include "Component/StableComponentContainer.h"
#include "Component/Component.h"

#include "Observers/Observers.h"


#include "EntityManager.h"
#include "trait.h"

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

	class Container
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

		TRefCounterHandle<EnityLifeTimeData> m_LifeTimeData{};

	public:
		[[nodiscard]] const TRefCounterHandle<EnityLifeTimeData>& GetLifeTimeData() const
		{
			return m_LifeTimeData;
		}

		[[nodiscard]] Entity CreateEntity(bool bIsActive = true);

		[[nodiscard]] inline uint32_t GetEntityCount() const
		{
			return m_EntityManager.GetCreatedEntityCount();
		}

		bool DestroyEntity(const Entity& entity);

		[[nodiscard]] inline uint64_t GetEmptyEntitiesCount() const
		{
			return m_EmptyEntities.size();
		}

		template<typename InitFunc, TComponentConcept... ComponentTypes, TTagConcept... TagTypes>
			requires query_callable<InitFunc, ComponentTypes...>
		Entity CreateEntity(
			const ComponentTypeGroup<ComponentTypes...> components,
			const TagTypeGroup<TagTypes...> tags,
			bool bIsActive,
			InitFunc&& initFunc
		)
		{
			if (Entity entity = CreateEntity(bIsActive))
			{
				EntityData* entityData = GetEntityData(entity);

				(AddTag<TagTypes>(*entityData), ...);

				std::tuple<ComponentTypes*...> componentsTuple = { AddComponent<ComponentTypes>(entity, *entityData)... };

				if constexpr (is_invocable_with_entity_v<InitFunc, ComponentTypes...>)
				{
					initFunc(entity, *std::get<ComponentTypes*>(componentsTuple)...);
				}
				else
				{
					initFunc(*std::get<ComponentTypes*>(componentsTuple)...);
				}

				return entity;
			}

			return Entity();
		}

	private:
		void InitializeLifeTimeData();

		void DestroyLifeTimeData();

		bool DestroyEntityInternal(const Entity& entity, bool bInvokeObservers);

		void SetEntityActive(const Entity& entity, bool bIsActive);

		EntityData* GetEntityData(const Entity& entity) const;

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

		template<TComponentConcept T>
		using ContainerType = PackedStableComponentContainer<drop_const_t<T>>;

		template<TComponentConcept... ComponentTypes>
		void CreateEntityFromSpawnData_Templated(
			const SpawnDataState& spawnState,
			const Entity& spawnedEntity,
			Archetype& spawnArchetype
		)
		{
			constexpr const TypeGroup<ComponentTypes...> componentsGroup{};

			EntityData* entityData = GetEntityData(spawnedEntity);
			spawnArchetype.AddEntityData(entityData);

			auto& archetypeTypeData = spawnArchetype.m_TypeData;
			const uint64_t typeCount = spawnArchetype.GetTypeCount();
			for (uint32_t i = 0; i < typeCount; i++)
			{
				ArchetypeTypeData& currentTypeData = archetypeTypeData[i];
				SpawnComponentData& spawnComponentData = m_SpawnData.m_ComponentData[i + spawnState.m_ComponentDataStart];

				if (spawnComponentData.IsTag())
				{
					continue;
				}

				auto spawnedCompPtr = spawnComponentData.m_SpawnStableContainer->CreateFromComponentBase(spawnComponentData.m_PrefabComponent);
				currentTypeData.m_PackedContainer->PushBack(spawnedCompPtr);
				spawnedCompPtr->OnPreCreate(entityData);

				spawnComponentData.m_SpawnedComponent = spawnedCompPtr;
			}


			uint32_t indexInArchetype = entityData->m_IndexInArchetype;


			std::tuple<ContainerType<ComponentTypes>*...> containersTuple = { spawnArchetype.GetTypePackedContainer<ComponentTypes>()... };

			std::tuple<drop_const_t<ComponentTypes>*> componentsTuple = { std::get<ContainerType<ComponentTypes>*>(containersTuple)->GetAsPtr(indexInArchetype)};
		}

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

			uint32_t componentContainerIndex = 0;
			Archetype* newArchetype = GetArchetypeAfterAddComponent<TComponent>(entityData.m_Archetype, componentContainerIndex);
			ArchetypeTypeData& archetypeTypeData = newArchetype->m_TypeData[componentContainerIndex];

			// Adding component to stable component container
			StableComponentContainer<TComponent>* stableContainer = static_cast<StableComponentContainer<TComponent>*>(archetypeTypeData.m_StableContainer);
			TComponent* componentPtr = stableContainer->Create(std::forward<Args>(args)...);

			//StableComponentRef componentNodeInfo = {};
			// Adding component pointer to packed container in archetype
			archetypeTypeData.m_PackedContainer->PushBack(componentPtr);

			// Adding entity to archetype
			if (oldArchetype != nullptr)
			{
				if (m_PerformDelayedDestruction)
				{
					AddArchetypeRecordToDelayedRemove(oldArchetype, indexInOldArchetype, false, componentTypeID);
					Archetype::MoveEntityAfterAddComponentWithoutDestroyingFromSource(*oldArchetype, *newArchetype, indexInOldArchetype, componentTypeID);
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

		template<TComponentConcept TComponent, typename TCallable>
		bool RemoveComponent_If(EntityData& entityData, TCallable&& canRemoveFunc)
		{
			if constexpr (is_tag_v<TComponent>)
			{
				return false;
			}

			if (!m_CanRemoveComponents)
			{
				return false;
			}

			TYPE_ID_CONSTEXPR TypeID componentTypeID = Type<TComponent>::ID();

			if (entityData.m_Archetype == nullptr || !entityData.IsValidToPerformComponentOperation()) return false;

			uint32_t compIdxInArch = entityData.m_Archetype->FindTypeIndex(componentTypeID);
			if (compIdxInArch == std::numeric_limits<uint32_t>::max()) return false;

			Archetype* oldArchetype = entityData.m_Archetype;
			uint64_t indexInOldArchetype = entityData.m_IndexInArchetype;

			ArchetypeTypeData& oldArchetypeTypeData = oldArchetype->m_TypeData[compIdxInArch];
			if (oldArchetypeTypeData.IsTag())
			{
				return false;
			}

			auto packedContainer = oldArchetypeTypeData.m_PackedContainer;
			EntityComponent* componentBasePtr = packedContainer->GetComponentBasePtr(indexInOldArchetype);
			if (componentBasePtr->GetDependecyCount() > 0)
			{
				return false;
			}

			const TComponent& compConstPtr = *static_cast<TComponent*>(componentBasePtr);
			if (!canRemoveFunc(compConstPtr))
			{
				return false;
			}

			Archetype* newArchetype = m_ArchetypesMap.GetArchetypeAfterRemoveComponent(
				*oldArchetype,
				componentTypeID
			);

			if (newArchetype != nullptr)
			{
				Archetype::MoveEntityAfterRemoveComponentWithoutDestroyingFromSource(*oldArchetype, *newArchetype, indexInOldArchetype, componentTypeID);
			}
			else
			{
				AddToEmptyEntities(entityData);
			}

			if (m_PerformDelayedDestruction)
			{
				AddArchetypeRecordToDelayedRemove(oldArchetype, static_cast<uint32_t>(indexInOldArchetype), true, componentTypeID);
			}
			else
			{
				oldArchetype->RemoveSwapBackEntityAfterRemoveComponent(indexInOldArchetype);
			}

			// Invoking remove observers:
			{
				InvokeComponentDestroyObservers(*oldArchetypeTypeData.m_ComponentContext, *componentBasePtr, entityData);
				oldArchetypeTypeData.m_StableContainer->Destroy(componentBasePtr);
			}

			return true;
		}
	private:
		void InvokeComponentDestroyObservers(IComponentContext& compCtx, EntityComponent& comp, EntityData& entityData);

	public:

/*template<typename... ComponentsTypes>
	uint32_t RemoveMultipleComponnets(Entity entity, EntityData& entityData)
	{
		if constexpr (sizeof...(ComponentsTypes) == 0)
		{
			return 0;
		}

		if (!m_CanRemoveComponents || entityData.m_Archetype == nullptr || !entityData.IsValidToPerformComponentOperation())
		{
			return 0;
		}

		Archetype* currentArchetype = entityData.m_Archetype;
		if (currentArchetype == nullptr)
		{
			return 0;
		}

		// invoke on destroy listeners:
		{
			TypeGroup<ComponentsTypes...> componentsTypes = {};

			for (uint32_t i = 0; i < componentsTypes.Size(); i++)
			{
				auto type = componentsTypes[i];

				if (currentArchetype != nullptr)
				{
					uint32_t typeIdx = currentArchetype->FindTypeIndex(type);
					if (typeIdx != std::numeric_limits<uint32_t>::max())
					{
						auto& typeData = currentArchetype->m_TypeData[typeIdx];
						typeData.m_ComponentContext->InvokeOnDestroyComponent(typeData.m_PackedContainer->GetComponentPtrAsVoi(entityData.m_IndexInArchetype), entity);
					}

					currentArchetype = entityData.m_Archetype;
				}
				else
				{
					break;
				}
			}
		}

		// archetype has changed during invoking of observers
		if (currentArchetype == nullptr)
		{
			return 0;
		}

		Archetype* newArchetype = m_ArchetypesMap.GetArchetypeAfterRemoveComponents<ComponentsTypes...>(currentArchetype);

		if (newArchetype == currentArchetype)
		{
			// archetype not changed
			return  0;
		}

		// archetype changed:
		if (newArchetype != nullptr)
		{
			uint32_t removedComponents = currentArchetype->GetComponentAndTagCount() - newArchetype->GetComponentAndTagCount();
			newArchetype->MoveEntityComponentsAfterRemoveComponent(currentArchetype, entityData.m_IndexInArchetype, &entityData);

			return removedComponents;
		}
		else
		{
			// here new archetype is nullptr
			entityData.m_Archetype->RemoveSwapBackEntity(entityData.m_IndexInArchetype);
			AddToEmptyEntities(entityData);

			return currentArchetype->GetComponentAndTagCount();
		}
	}*/

		template<TComponentConcept TComponent>
		TComponent* GetComponent(EntityData& entityData) const
		{
			if constexpr (is_tag_v<TComponent>)
			{
				return nullptr;
			}

			if (entityData.m_Archetype != nullptr && entityData.IsAlive())
			{
				uint32_t findTypeIndex = entityData.m_Archetype->FindTypeIndex<TComponent>();
				if (findTypeIndex != std::numeric_limits<uint32_t>::max())
				{
					PackedStableComponentContainer<TComponent>* container = static_cast<PackedStableComponentContainer<TComponent>*>(entityData.m_Archetype->m_TypeData[findTypeIndex].m_PackedContainer);
					return container->GetAsPtr(entityData.m_IndexInArchetype);
				}
			}
			return nullptr;
		}

		EntityComponent* GetComponent(EntityData& entityData, TypeID componentType) const
		{
			if (entityData.m_Archetype != nullptr && entityData.IsAlive())
			{
				uint32_t findTypeIndex = entityData.m_Archetype->FindTypeIndex(componentType);
				if (findTypeIndex != std::numeric_limits<uint32_t>::max())
				{
					const auto& typeData = entityData.m_Archetype->m_TypeData[findTypeIndex];
					if (typeData.IsTag())
					{
						return nullptr;
					}
					return typeData.m_PackedContainer->GetComponentBasePtr(entityData.m_IndexInArchetype);
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
		EntityComponent* GetComponentAtIndex(EntityData& entityData, uint32_t componentIndex);

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

		template<TComponentConcept TComponent>
		TComponent* GetComponentWithoutCheckingIsAlive(EntityData& entityData) const
		{
			if (entityData.m_Archetype != nullptr)
			{
				uint32_t findTypeIndex = entityData.m_Archetype->FindTypeIndex<TComponent>();
				if (findTypeIndex != std::numeric_limits<uint32_t>::max())
				{
					PackedStableComponentContainer<TComponent>* container = static_cast<PackedStableComponentContainer<TComponent>*>(entityData.m_Archetype->m_TypeData[findTypeIndex].m_PackedContainer);
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

		template<TComponentConcept TComponent>
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
					AddArchetypeRecordToDelayedRemove(entityData.m_Archetype, entityData.m_IndexInArchetype, false, tagTypeID);
					Archetype::MoveEntityAfterAddComponentWithoutDestroyingFromSource(*oldArchetype, *newArchetype, indexInOldArchetype, tagTypeID);
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
		Archetype* GetArchetypeAfterAddComponent(Archetype* toArchetype, uint32_t& componentContainerIndex)
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

				componentContainerIndex = entityNewArchetype->FindTypeIndex<TComponent>();
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
		/// Invokes only entity Create and Enable (if entity is enabled). If callbacks was invoked earliers then callbacks will not be invoked. This function should be used with CreateEntity_NoCallbacks method.
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
		Entity CreateEntity_NoCallbacks(bool bIsActive = true);

		bool DestroyEntity_NoCallback(const Entity& entity);

		Entity Spawn_NoCallback(
			const Entity& prefab,
			bool bIsActive = true
		);

		bool Spawn_NoCallback(
			const Entity& prefab,
			uint64_t spawnCount,
			bool bAreActive = true
		);

		bool Spawn_NoCallback(
			const Entity& prefab,
			std::vector<Entity>& spawnedEntities,
			uint64_t spawnCount,
			bool bAreActive = true
		);

	private:
		void SetEntityActive_NoCallback(const Entity& entity, bool bIsActive);

		template<TComponentConcept TComponent, typename ...Args>
		TComponent* AddComponent_NoCallback(const Entity& entity, EntityData& entityData, Args&&... args)
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

			uint32_t componentContainerIndex = 0;
			Archetype* newArchetype = GetArchetypeAfterAddComponent<TComponent>(entityData.m_Archetype, componentContainerIndex);
			ArchetypeTypeData& archetypeTypeData = newArchetype->m_TypeData[componentContainerIndex];

			// Adding component to stable component container
			StableComponentContainer<TComponent>* stableContainer = static_cast<StableComponentContainer<TComponent>*>(archetypeTypeData.m_StableContainer);
			TComponent* componentPtr = stableContainer->Create(std::forward<Args>(args)...);

			//StableComponentRef componentNodeInfo = {};
			// Adding component pointer to packed container in archetype
			archetypeTypeData.m_PackedContainer->PushBack(componentPtr);

			// Adding entity to archetype
			uint32_t entityIndexBuffor = newArchetype->EntityCount();
			if (oldArchetype != nullptr)
			{
				if (m_PerformDelayedDestruction)
				{
					AddArchetypeRecordToDelayedRemove(entityData.m_Archetype, entityData.m_IndexInArchetype, false, copmonentTypeID);
					Archetype::MoveEntityAfterAddComponentWithoutDestroyingFromSource(*oldArchetype, *newArchetype, indexInOldArchetype, copmonentTypeID);

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
		bool RemoveComponent_NoCallback(const Entity& entity)
		{
			if constexpr (is_tag_v<TComponent>)
			{
				return false;
			}
			if (!m_CanRemoveComponents)
			{
				return false;
			}

			return RemoveComponent_NoCallback(entity, Type<TComponent>::ID());
		}

		bool RemoveComponent_NoCallback(const Entity& entity, TypeID componentTypeID);

	public:
		void SetEntityActiveOverride_NoCallback(const Entity& entity, bool bIsActiveOverride);

		void SetEntityDisabledOverrideCount_NoCallback(const Entity& entity, uint32_t disabledOverrideCount);

		void ResetDisabledOverrideCount_NoCallback(const Entity& entity);

	#pragma endregion

	};
}