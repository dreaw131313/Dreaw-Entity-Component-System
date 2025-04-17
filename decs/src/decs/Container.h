#pragma once
#include "Core.h"
#include "Type.h"

#include "Archetypes/ArchetypesMap.h"
#include "EntityManager.h"
#include "ComponentContext/ComponentContextsManager.h"

#include "Observers/Observers.h"

#include "Component/Component.h"

#include "decs/ComponentContainers/PackedContainer.h"
#include "decs/ComponentContainers/StableContainer.h"

#include "trait.h"

namespace decs
{
	class Entity;
	class SpawnEntityCallback;

	class Container
	{
		template<typename ...Types>
		friend class Query;
		template<typename ...Types>
		friend class MultiQuery;
		template<typename, typename...>
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

		Container(
			uint64_t enititesChunkSize,
			uint32_t stableComponentDefaultChunkSize
		);

		Container(
			EntityManager* entityManager,
			uint32_t stableComponentDefaultChunkSize
		);

		Container(bool bCreateInvalid);

		~Container();

	#pragma region Extension data
	public:
		template<typename T>
		void SetExtensionData(T* data)
		{
			m_ExtensionData = static_cast<T*>(data);
		}

		template<typename T>
		T* GetExtensionData()
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

		void SetDataIfCreatedInvalid(
			EntityManager* entityManager,
			uint32_t stableComponentDefaultChunkSize
		);

		/// <summary>
		/// Returns all owned entites to entity manager. Clears all created components. Does not destroy created archetypes and does not clears seted observer manager. This function does not invoke any methods from observers.
		/// </summary>
		void Clear();

		/// <summary>
		/// Returns owned entites to entity manager. This is helper function. This function does not destroy all created components and archetypes. It returns entites to manager so if this container is using shared "entities manager" entites can be returned and then destroying of this object can be performed in desired moment (for example in different thread). After invoking this function this container is in invalid state and must be only destroyed. Creating entites or modifying entities is after invoking this method is undefined behavior. If this method is not invoked destroycotor of this object will return owned entites. If this method was invoked destrucor will not perform returning of owned entities.
		/// </summary>
		void ReturnOwnedEntitiesToEntityManager();

	private:
		void ReturnOwnedEntitiesToEntityManager_Internal(bool bNullEntityManagerIfIsNotHisOwner);

	#pragma endregion

	#pragma region ENTITIES:
	private:
		std::vector<EntityData*> m_EmptyEntities = {};
		EntityManager* m_EntityManager = nullptr;
		uint32_t m_EntityCount = 0;
		bool m_HaveOwnEntityManager = false;

	public:
		Entity CreateEntity(bool bIsActive = true, void* userData = nullptr);

		inline uint32_t GetEntityCount() const
		{
			return m_EntityCount;
		}

	public:
		bool DestroyEntity(const Entity& entity);

		inline uint64_t GetEmptyEntitiesCount() const
		{
			return m_EmptyEntities.size();
		}

	private:
		bool DestroyEntityInternal(Entity entity, bool bInvokeObservers);

		void SetEntityActive(const Entity& entity, bool bIsActive);

		void AddToEmptyEntitiesRightAfterNewEntityCreation(EntityData& data);

		void AddToEmptyEntities(EntityData& data);

		void RemoveFromEmptyEntities(EntityData& data);

		void InvokeEntityComponentDestructionObservers(const Entity& entity);

		EntityData* CreateAliveEntityData(bool bIsActive);

	#pragma endregion

	#pragma region RESERVING ENTITIES:
	public:
		void ReserveEntities(uint32_t entitiesToReserve);

		void FreeReservedEntities();

	private:
		std::vector<EntityData*> m_ReservedEntityData;

	#pragma endregion

	#pragma region SPAWNING ENTITIES:
	private:
		struct SpawnComponentRefData
		{
		public:
			StableContainerBase* m_StableContainer = nullptr;
			ComponentBase* m_ComponentPtr = nullptr;

		public:
			SpawnComponentRefData()
			{

			}

			SpawnComponentRefData(
				StableContainerBase* stableContainer,
				ComponentBase* componentPtr
			):
				m_StableContainer(stableContainer), m_ComponentPtr(componentPtr)
			{

			}

			inline bool IsTag() const
			{
				return m_StableContainer == nullptr;
			}
		};

		struct SpawnData
		{
		public:
			std::vector<SpawnComponentRefData> m_PrefabComponentRefs;
			std::vector <Archetype*> m_SpawnArchetypes;
			std::vector<ComponentBase*> m_SpawnedEntityComponentPtrs;

		public:
			void Reserve(uint64_t size)
			{
				m_PrefabComponentRefs.reserve(size);
				m_SpawnedEntityComponentPtrs.reserve(size);
			}

			void Clear()
			{
				m_PrefabComponentRefs.clear();
				m_SpawnedEntityComponentPtrs.clear();
				m_SpawnArchetypes.clear();
			}

			void PopBackSpawnState(uint64_t archetypeIdx, uint64_t refsStartIdx)
			{
				if (archetypeIdx == 0)
				{
					Clear();
				}
				else
				{
					m_SpawnArchetypes.pop_back();

					auto pIt = m_PrefabComponentRefs.begin();
					std::advance(pIt, refsStartIdx);
					m_PrefabComponentRefs.erase(pIt, m_PrefabComponentRefs.end());

					auto eIt = m_SpawnedEntityComponentPtrs.begin();
					std::advance(eIt, refsStartIdx);
					m_SpawnedEntityComponentPtrs.erase(eIt, m_SpawnedEntityComponentPtrs.end());
				}
			}
		};

		struct SpawnDataState
		{
		public:
			uint32_t m_CompRefsStart;
			uint32_t m_ArchetypeIndex;

		public:
			SpawnDataState(SpawnData& spawnData):
				m_CompRefsStart((uint32_t)spawnData.m_SpawnedEntityComponentPtrs.size()),
				m_ArchetypeIndex((uint32_t)spawnData.m_SpawnArchetypes.size())
			{

			}
		};

	private:
		SpawnData m_SpawnData = {};

	public:
		Entity Spawn(
			const Entity& prefab,
			bool bIsActive = true,
			void* userData = nullptr
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

		Entity Spawn_WithCallback(
			SpawnEntityCallback& callback,
			const Entity& prefab,
			bool bIsActive = true,
			void* userData = nullptr
		);

		bool Spawn_WithCallback(
			SpawnEntityCallback& callback,
			const Entity& prefab,
			uint64_t spawnCount,
			bool bAreActive = true
		);

		bool Spawn_WithCallback(
			SpawnEntityCallback& callback,
			const Entity& prefab,
			std::vector<Entity>& spawnedEntities,
			uint64_t spawnCount,
			bool bAreActive = true
		);

	private:
		void PrepareSpawnDataFromPrefab(
			EntityData& prefabEntityData,
			Container* prefabContainer
		);

		void CreateEntityFromSpawnData(
			const Entity& entity,
			EntityData& spawnedEntityData,
			const SpawnDataState& spawnState
		);

		void InvokeComponentCreateAndEnableObserversOnSpawn(const Entity& entity, const Archetype& archetype, const SpawnDataState& spawnState);

	#pragma endregion

	#pragma region COMPONENTS:
	private:
		ComponentContextsManager m_ComponentContextManager = { 1000 };

	private:
		void OnAddComponentInvokeObservers(
			const Entity& entity,
			ComponentContextBase* componentContext,
			PackedContainerBase* packedContainer,
			TypeID compTypeID
		);

		template<typename TComponent, typename ...Args>
		TComponent* AddComponent(Entity entity, EntityData& entityData, Args&&... args)
		{
			if constexpr (is_tag_v<TComponent>)
			{
				return nullptr;
			}

			if (!m_CanAddComponents || !entityData.IsValidToPerformComponentOperation())
			{
				return nullptr;
			}

			TYPE_ID_CONSTEXPR TypeID copmonentTypeID = Type<TComponent>::ID();

			auto currentComponent = GetComponentWithoutCheckingIsAlive<TComponent>(entityData);
			if (currentComponent != nullptr)
			{
				return currentComponent;
			}

			uint32_t componentContainerIndex = 0;
			Archetype* entityNewArchetype = GetArchetypeAfterAddComponent<TComponent>(entityData.m_Archetype, componentContainerIndex);
			ArchetypeTypeData& archetypeTypeData = entityNewArchetype->m_TypeData[componentContainerIndex];

			// Adding component to stable component container
			StableContainer<TComponent>* stableContainer = static_cast<StableContainer<TComponent>*>(archetypeTypeData.m_StableContainer);
			TComponent* componentPtr = stableContainer->Emplace(std::forward<Args>(args)...);

			//StableComponentRef componentNodeInfo = {};
			// Adding component pointer to packed container in archetype
			archetypeTypeData.m_PackedContainer->PushBack(componentPtr);

			// Adding entity to archetype
			if (entityData.m_Archetype != nullptr)
			{
				if (m_PerformDelayedDestruction)
				{
					AddArchetypeRecordToDelayedRemove(entityData.m_Archetype, entityData.m_IndexInArchetype, false, copmonentTypeID);
					entityNewArchetype->MoveEntityAfterAddComponentWithoutDestroyingFromSource(
						entityData.m_Archetype,
						entityData.m_IndexInArchetype,
						copmonentTypeID,
						&entityData
					);
				}
				else
				{
					entityNewArchetype->MoveEntityComponentsAfterAddComponent<TComponent>(
						entityData.m_Archetype,
						entityData.m_IndexInArchetype,
						&entityData
					);
				}
			}
			else
			{
				RemoveFromEmptyEntities(entityData);
				entityNewArchetype->AddEntityData(&entityData);
			}

			static_cast<ComponentBase*>(componentPtr)->OnPreCreate(entity);

			OnAddComponentInvokeObservers(entity, archetypeTypeData.m_ComponentContext, archetypeTypeData.m_PackedContainer, copmonentTypeID);

			return componentPtr;
		}

		template<typename TComponent>
		bool RemoveComponent(Entity entity)
		{
			if constexpr (is_tag_v<TComponent>)
			{
				return false;
			}
			return RemoveComponent(entity, Type<TComponent>::ID());
		}

		bool RemoveComponent(const Entity& entity, TypeID componentTypeID);

		template<typename TComponent, typename TCallable>
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
			uint64_t entityIndexInOldArchetype = entityData.m_IndexInArchetype;

			ArchetypeTypeData& archetypeTypeData = oldArchetype->m_TypeData[compIdxInArch];
			if (archetypeTypeData.IsTag())
			{
				return false;
			}

			auto packedContainer = archetypeTypeData.m_PackedContainer;
			ComponentBase* componentBasePtr = packedContainer->GetComponentBasePtr(entityIndexInOldArchetype);
			if (componentBasePtr->GetDependecyCount() > 0)
			{
				return false;
			}

			const TComponent& compConstPtr = *static_cast<TComponent*>(componentBasePtr);
			if (!canRemoveFunc(compConstPtr))
			{
				return false;
			}

			Archetype* newEntityArchetype = m_ArchetypesMap.GetArchetypeAfterRemoveComponent(
				*entityData.m_Archetype,
				componentTypeID
			);

			if (newEntityArchetype != nullptr)
			{
				newEntityArchetype->MoveEntityAfterRemoveComponentWithoutDestroyingFromSource(
					componentTypeID,
					entityData.m_Archetype,
					entityData.m_IndexInArchetype,
					&entityData
				);
			}
			else
			{
				AddToEmptyEntities(entityData);
			}

			InvokeRemoveComponentObserverCallbacks(
				entityData,
				componentBasePtr,
				*oldArchetype,
				static_cast<uint32_t>(entityIndexInOldArchetype),
				*archetypeTypeData.m_ComponentContext
			);

			if (m_PerformDelayedDestruction)
			{
				AddArchetypeRecordToDelayedRemove(oldArchetype, static_cast<uint32_t>(entityIndexInOldArchetype), true, componentTypeID);
			}
			else
			{
				oldArchetype->RemoveSwapBackEntityAfterMoveEntityWithoutDestroyingSource(entityIndexInOldArchetype, componentTypeID);
			}

			return true;
		}

		void InvokeRemoveComponentObserverCallbacks(
			EntityData& entityData,
			ComponentBase* componentPtr,
			Archetype& oldArchetype,
			uint32_t entityIndexInOldArchetype,
			ComponentContextBase& componentContext
		);

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
				uint32_t removedComponents = currentArchetype->ComponentCount() - newArchetype->ComponentCount();
				newArchetype->MoveEntityComponentsAfterRemoveComponent(currentArchetype, entityData.m_IndexInArchetype, &entityData);

				return removedComponents;
			}
			else
			{
				// here new archetype is nullptr
				entityData.m_Archetype->RemoveSwapBackEntity(entityData.m_IndexInArchetype);
				AddToEmptyEntities(entityData);

				return currentArchetype->ComponentCount();
			}
		}*/

		template<typename TComponent>
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
					StablePackedContainer<TComponent>* container = static_cast<StablePackedContainer<TComponent>*>(entityData.m_Archetype->m_TypeData[findTypeIndex].m_PackedContainer);
					return container->GetAsPtr(entityData.m_IndexInArchetype);
				}
			}
			return nullptr;
		}

		ComponentBase* GetComponent(EntityData& entityData, TypeID componentType) const
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

		template<typename TComponent>
		TComponent* GetComponentDynamic(EntityData& entityData)
		{
			if (entityData.m_Archetype != nullptr && entityData.IsAlive())
			{
				uint32_t archetypeComponentCount = entityData.m_Archetype->ComponentCount();
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

		template<typename TComponent>
		TComponent* GetComponentWithoutCheckingIsAlive(EntityData& entityData) const
		{
			if (entityData.m_Archetype != nullptr)
			{
				uint32_t findTypeIndex = entityData.m_Archetype->FindTypeIndex<TComponent>();
				if (findTypeIndex != std::numeric_limits<uint32_t>::max())
				{
					StablePackedContainer<TComponent>* container = static_cast<StablePackedContainer<TComponent>*>(entityData.m_Archetype->m_TypeData[findTypeIndex].m_PackedContainer);
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

		template<typename TComponent>
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

		inline bool HasTag(EntityData& entityData, TypeID tagType)
		{
			if (!entityData.IsAlive() || entityData.m_Archetype == nullptr)
			{
				return false;
			}

			return entityData.m_Archetype->HasTag(tagType);
		}

		template<typename TTag>
		inline bool HasTag(EntityData& entityData)
		{
			if constexpr (!is_tag_v<TTag>)
			{
				return false;
			}
			return HasTag(entityData, Type<TTag>::ID());
		}

		template<typename TTag>
		bool AddTag(EntityData& entityData)
		{
			if constexpr (!is_tag_v<TTag>)
			{
				return false;
			}

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

			Archetype* newArchetype = GetArchetypeAfterAddTag(oldArchetype, tagTypeID);

			if (oldArchetype != nullptr)
			{
				if (m_PerformDelayedDestruction)
				{
					// Change to respect tag
					// AddArchetypeRecordToDelayedRemove(entityData.m_Archetype, entityData.m_IndexInArchetype, false, copmonentTypeID);

					// move entity to new archetype
					//entityNewArchetype->MoveEntityAfterAddComponentWithoutDestroyingFromSource(
					//	entityData.m_Archetype,
					//	entityData.m_IndexInArchetype,
					//	copmonentTypeID,
					//	&entityData
					//);
				}
				else
				{
					// move entity to new archetype
					//entityNewArchetype->MoveEntityComponentsAfterAddComponent<TComponent>(
					//	entityData.m_Archetype,
					//	entityData.m_IndexInArchetype,
					//	&entityData
					//);
				}
			}
			else
			{
				// means that archetype has one component/tag so we just need add entity to it
				//RemoveFromEmptyEntities(entityData);
				//newArchetype->AddEntityData(&entityData);
			}

			return true;
		}

		bool RemoveTag(EntityData& entityData, TypeID tagType);

		template<typename TTag>
		bool RemoveTag(EntityData& entityData)
		{
			if constexpr (!is_tag_v<TTag>)
			{
				return false;
			}

			return RemoveTag(entityData, Type<TTag>::ID());
		}

	#pragma endregion

	#pragma region STABLE COMPONENTS:
	public:
		template<typename T>
		bool SetComponentChunkSize(uint32_t chunkSize)
		{
			return m_ComponentContextManager.SetComponentChunkSize<T>(chunkSize);
		}

		bool SetComponentChunkSize(TypeID typeID, uint32_t chunkSize)
		{
			return m_ComponentContextManager.SetComponentChunkSize(typeID, chunkSize);
		}

		template<typename T>
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

	private:
		ArchetypesMap m_ArchetypesMap;

	private:
		template<typename TComponent>
		Archetype* GetArchetypeAfterAddComponent(
			Archetype* toArchetype,
			uint32_t& componentContainerIndex
		)
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

		void InvokeEntitesOnDestroyListeners();

		/// <summary>
		/// Changes order of invoking function of component observers. Callback for component with lower order will be invoked first.
		/// </summary>
		/// <typeparam name="ComponentType"></typeparam>
		/// <param name="order"></param>
		template<typename TComponent>
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

		template<typename TComponent>
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

		template<typename TComponent>
		void SetCreateComponentObserver(CreateComponentObserver<TComponent>* createObserver)
		{
			auto componentContext = m_ComponentContextManager.GetOrCreateComponentContext<TComponent>();
			componentContext->m_Observers.m_CreateObserver = createObserver;
		}

		template<typename TComponent>
		void SetDestroyComponentObserver(DestroyComponentObserver<TComponent>* destroyObserver)
		{
			auto componentContext = m_ComponentContextManager.GetOrCreateComponentContext<TComponent>();
			componentContext->m_Observers.m_DestroyObserver = destroyObserver;
		}

		template<typename TComponent>
		void SetCreateDestroyComponentObservers(
			CreateComponentObserver<TComponent>* createObserver,
			DestroyComponentObserver<TComponent>* destroyObserver
		)
		{
			auto componentContext = m_ComponentContextManager.GetOrCreateComponentContext<TComponent>();
			componentContext->m_Observers.m_CreateObserver = createObserver;
			componentContext->m_Observers.m_DestroyObserver = destroyObserver;
		}

		template<typename TComponent>
		void SetEnableComponentObserver(EnableComponentObserver<TComponent>* enableObserver)
		{
			auto componentContext = m_ComponentContextManager.GetOrCreateComponentContext<TComponent>();
			componentContext->m_Observers.m_EnableObserver = enableObserver;
		}

		template<typename TComponent>
		void SetDisableComponentObserver(DisableComponentObserver<TComponent>* disableObserver)
		{
			auto componentContext = m_ComponentContextManager.GetOrCreateComponentContext<TComponent>();
			componentContext->m_Observers.m_DisableObserver = disableObserver;
		}

		template<typename TComponent>
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
		void InvokeEntityObservers(const decs::Entity& entity);

	private:
		std::vector<ComponentBase*> m_ActivationChangeComponentPtrs = {};

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
			bool isActive = true
		);

		bool Spawn_NoCallback(
			const Entity& prefab,
			uint64_t spawnCount,
			bool areActive = true
		);

		bool Spawn_NoCallback(
			const Entity& prefab,
			std::vector<Entity>& spawnedEntities,
			uint64_t spawnCount,
			bool areActive = true
		);

	private:
		void SetEntityActive_NoCallback(const Entity& entity, bool bIsActive);

		template<typename TComponent, typename ...Args>
		TComponent* AddComponent_NoCallback(Entity entity, EntityData& entityData, Args&&... args)
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

			uint32_t componentContainerIndex = 0;
			Archetype* entityNewArchetype = GetArchetypeAfterAddComponent<TComponent>(entityData.m_Archetype, componentContainerIndex);
			ArchetypeTypeData& archetypeTypeData = entityNewArchetype->m_TypeData[componentContainerIndex];

			// Adding component to stable component container
			StableContainer<TComponent>* stableContainer = static_cast<StableContainer<TComponent>*>(archetypeTypeData.m_StableContainer);
			TComponent* componentPtr = stableContainer->Emplace(std::forward<Args>(args)...);

			//StableComponentRef componentNodeInfo = {};
			// Adding component pointer to packed container in archetype
			archetypeTypeData.m_PackedContainer->PushBack(componentPtr);

			// Adding entity to archetype
			uint32_t entityIndexBuffor = entityNewArchetype->EntityCount();
			if (entityData.m_Archetype != nullptr)
			{
				if (m_PerformDelayedDestruction)
				{
					AddArchetypeRecordToDelayedRemove(entityData.m_Archetype, entityData.m_IndexInArchetype, false, copmonentTypeID);
					entityNewArchetype->MoveEntityAfterAddComponentWithoutDestroyingFromSource(
						entityData.m_Archetype,
						entityData.m_IndexInArchetype,
						copmonentTypeID,
						&entityData
					);
				}
				else
				{
					entityNewArchetype->MoveEntityComponentsAfterAddComponent<TComponent>(
						entityData.m_Archetype,
						entityData.m_IndexInArchetype,
						&entityData
					);
				}
			}
			else
			{
				RemoveFromEmptyEntities(entityData);
				entityNewArchetype->AddEntityData(&entityData);
			}

			static_cast<ComponentBase*>(componentPtr)->OnPreCreate(entity);

			return componentPtr;
		}

		template<typename TComponent>
		bool RemoveComponent_NoCallback(Entity entity)
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

	#pragma endregion

	};
}