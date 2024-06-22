#pragma once
#include "Core.h"
#include "Type.h"

#include "Archetypes\ArchetypesMap.h"
#include "EntityManager.h"
#include "ComponentContext\ComponentContextsManager.h"

#include "Observers\Observers.h"

#include "Component/Component.h"

#include "decs\ComponentContainers\PackedContainer.h"
#include "decs\ComponentContainers\StableContainer.h"
#include "ComponentRefs\ComponentBaseRef.h"

namespace decs
{
	struct ComponentDelayedDestroyData
	{
	public:
		EntityData* m_EntityData = nullptr;
		TypeID m_TypeID = std::numeric_limits<TypeID>::max();
		bool m_IsStable = false;

	public:
		ComponentDelayedDestroyData()
		{

		}

		ComponentDelayedDestroyData(
			EntityData* entityData,
			TypeID typeID,
			bool isStable
		) :
			m_EntityData(entityData), m_TypeID(typeID), m_IsStable(isStable)
		{

		}

		bool operator==(const decs::ComponentDelayedDestroyData& rhs) const
		{
			return m_EntityData == rhs.m_EntityData && m_TypeID == rhs.m_TypeID;
		}
	};
}

template<>
struct ::std::hash<decs::ComponentDelayedDestroyData>
{
	std::size_t operator()(const decs::ComponentDelayedDestroyData& data) const
	{
		uint64_t eIDHash = std::hash<decs::EntityData*>{}(data.m_EntityData);
		uint64_t typeIdHash = std::hash<decs::TypeID>{}(data.m_TypeID);
		return eIDHash ^ (typeIdHash + 0x9e3779b9 + (eIDHash << 6) + (eIDHash >> 2));
	}
};

namespace decs
{
	class Entity;

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
		std::vector<EntityData*> m_EmptyEntities = {}; //TODO: change to std::vector
		EntityManager* m_EntityManager = nullptr;
		uint32_t m_EntityCount = 0;
		bool m_HaveOwnEntityManager = false;

	public:
		Entity CreateEntity(bool isActive = true);

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
		uint32_t m_ReservedEntitiesCount = 0;

#pragma endregion

#pragma region SPAWNING ENTITIES:
	private:
		struct SpawnComponentRefData
		{
		public:
			bool m_IsStable = false;
			StableContainerBase* m_StableContainer = nullptr;
			ComponentBaseRef m_ComponentRef;

		public:
			SpawnComponentRefData()
			{

			}

			template<typename... Args>
			SpawnComponentRefData(
				const bool& isStable,
				StableContainerBase* stableContainer,
				Args&&... args
			) :
				m_IsStable(isStable), m_StableContainer(stableContainer), m_ComponentRef(std::forward<Args>(args)...)
			{

			}
		};

		struct SpawnData
		{
		public:
			std::vector<SpawnComponentRefData> m_PrefabComponentRefs;
			std::vector <Archetype*> m_SpawnArchetypes;
			std::vector<ComponentBaseRef> m_SpawnedEntityComponentRefs;

		public:
			void Reserve(uint64_t size)
			{
				m_PrefabComponentRefs.reserve(size);
				m_SpawnedEntityComponentRefs.reserve(size);
			}

			void Clear()
			{
				m_PrefabComponentRefs.clear();
				m_SpawnedEntityComponentRefs.clear();
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

					auto eIt = m_SpawnedEntityComponentRefs.begin();
					std::advance(eIt, refsStartIdx);
					m_SpawnedEntityComponentRefs.erase(eIt, m_SpawnedEntityComponentRefs.end());
				}
			}
		};

		struct SpawnDataState
		{
		public:
			uint32_t m_CompRefsStart;
			uint32_t m_ArchetypeIndex;

		public:
			SpawnDataState(SpawnData& spawnData) :
				m_CompRefsStart((uint32_t)spawnData.m_SpawnedEntityComponentRefs.size()),
				m_ArchetypeIndex((uint32_t)spawnData.m_SpawnArchetypes.size())
			{

			}
		};

	private:
		SpawnData m_SpawnData = {};

	public:
		Entity Spawn(
			const Entity& prefab,
			bool isActive = true
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
			EntityData& prefabEntityData,
			Container* prefabContainer
		);

		void CreateEntityFromSpawnData(
			EntityData& spawnedEntityData,
			const SpawnDataState& spawnState
		);

		void InvokeComponentCreateAndEnableObserversOnSpawn(const Entity& entity, Archetype* archetype, uint64_t componentsCount, const SpawnDataState& spawnState);

#pragma endregion

#pragma region COMPONENTS:
	private:
		ComponentContextsManager m_ComponentContextManager = {};

	private:
		void OnAddComponentInvokeObservers(
			const Entity& entity,
			ComponentContextBase* componentContext,
			PackedContainerBase* packedContainer,
			TypeID compTypeID
		);

		template<typename TComponent, typename ...Args>
		inline TComponent* AddComponent(Entity entity, EntityData& entityData, Args&&... args)
		{
			if (!m_CanAddComponents) return nullptr;

			return AddStableComponent<TComponent>(entity, entityData, std::forward<Args>(args)...);
		}

		template<typename TComponent, typename ...Args>
		TComponent* AddStableComponent(const Entity& entity, EntityData& entityData, Args&&... args)
		{
			TYPE_ID_CONSTEXPR TypeID copmonentTypeID = Type<TComponent>::ID();

			if (!entityData.IsValidToPerformComponentOperation())
			{
				return nullptr;
			}

			auto currentComponent = GetStableComponentWithoutCheckingIsAlive<TComponent>(entityData);
			if (currentComponent != nullptr)
			{
				return currentComponent;
			}

			uint32_t componentContainerIndex = 0;
			Archetype* entityNewArchetype = GetArchetypeAfterAddComponent<TComponent>(entityData.m_Archetype, componentContainerIndex);
			ArchetypeTypeData& archetypeTypeData = entityNewArchetype->m_TypeData[componentContainerIndex];

			// Adding component to stable component container
			StableContainer<TComponent>* stableContainer = static_cast<StableContainer<TComponent>*>(archetypeTypeData.m_StableContainer);
			StableComponentRef componentNodeInfo = stableContainer->Emplace(std::forward<Args>(args)...);

			//StableComponentRef componentNodeInfo = {};
			// Adding component pointer to packed container in archetype
			archetypeTypeData.m_PackedContainer->EmplaceFromVoid(&componentNodeInfo);

			TComponent* componentPtr = static_cast<TComponent*>(componentNodeInfo.m_ComponentPtr);

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

			OnAddComponentInvokeObservers(entity, archetypeTypeData.m_ComponentContext, archetypeTypeData.m_PackedContainer, copmonentTypeID);

			return componentPtr;
		}

		template<typename TComponent>
		bool RemoveComponent(Entity entity)
		{
			if (!m_CanRemoveComponents)
			{
				return false;
			}

			return RemoveComponent(entity, Type<TComponent>::ID());
		}

		bool RemoveComponent(const Entity& entity, TypeID componentTypeID);

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
						if (typeIdx != Limits::MaxComponentCount)
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
		TComponent* GetComponent(EntityID e) const
		{
			if (e < m_EntityManager->GetEntitiesDataCount())
			{
				EntityData& entityData = m_EntityManager->GetEntityData(e);
				return GetComponent<TComponent>(entityData);
			}

			return nullptr;
		}

		template<typename TComponent>
		TComponent* GetComponent(EntityData& entityData) const
		{
			if (entityData.m_Archetype != nullptr && entityData.IsAlive())
			{
				uint32_t findTypeIndex = entityData.m_Archetype->FindTypeIndex<TComponent>();
				if (findTypeIndex != Limits::MaxComponentCount)
				{
					StablePackedContainer<TComponent>* container = static_cast<StablePackedContainer<TComponent>*>(entityData.m_Archetype->m_TypeData[findTypeIndex].m_PackedContainer);
					return static_cast<TComponent*>(container->m_Data[entityData.m_IndexInArchetype].m_ComponentPtr);
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
					auto& componentData = typeDataVector[i];
					auto componentPtr = componentData.m_PackedContainer->GetComponentBasePtr(entityData.m_IndexInArchetype);

					TComponent* casted = dynamic_cast<TComponent*>(componentPtr);
					if (casted != nullptr)
					{
						return casted;
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
				if (findTypeIndex != Limits::MaxComponentCount)
				{
					StablePackedContainer<TComponent>* container = static_cast<StablePackedContainer<TComponent>*>(entityData.m_Archetype->m_TypeData[findTypeIndex].m_PackedContainer);
					return static_cast<TComponent*>(container->m_Data[entityData.m_IndexInArchetype].m_ComponentPtr);
				}
			}
			return nullptr;
		}

		template<typename TComponent>
		TComponent* GetStableComponentWithoutCheckingIsAlive(EntityData& entityData) const
		{
			if (entityData.m_Archetype != nullptr)
			{
				uint32_t findTypeIndex = entityData.m_Archetype->FindTypeIndex<TComponent>();
				if (findTypeIndex != Limits::MaxComponentCount)
				{
					StablePackedContainer<TComponent>* container = static_cast<StablePackedContainer<TComponent>*>(entityData.m_Archetype->m_TypeData[findTypeIndex].m_PackedContainer);
					return static_cast<TComponent*>(container->m_Data[entityData.m_IndexInArchetype].m_ComponentPtr);
				}
			}
			return nullptr;
		}

		bool HasComponentInternal(EntityData& entityData, TypeID typeID) const
		{
			if (entityData.m_Archetype != nullptr)
			{
				return entityData.m_Archetype->ContainType(typeID);
			}
			return false;
		}

		template<typename TComponent>
		bool HasComponent(EntityData& entityData) const
		{
			return HasComponentInternal(entityData, Type<TComponent>::ID());
		}

#pragma endregion

#pragma region STABLE COMPONENTS
	private:
		StableContainersManager m_StableContainers = { 1000 };
	public:
		template<typename T>
		bool SetStableComponentChunkSize(uint32_t chunkSize)
		{
			return m_StableContainers.SetStableComponentChunkSize<T>(chunkSize);
		}

		bool SetStableComponentChunkSize(TypeID typeID, uint32_t chunkSize)
		{
			return m_StableContainers.SetStableComponentChunkSize(typeID, chunkSize);
		}

		template<typename T>
		uint64_t GetStableComponentChunkSize()
		{
			return m_StableContainers.GetStableComponentChunkSize<T>();
		}

		uint64_t GetStableComponentChunkSize(TypeID typeID)
		{
			return m_StableContainers.GetStableComponentChunkSize(typeID);
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
			TYPE_ID_CONSTEXPR TypeID id = Type<TComponent>::ID();

			Archetype* entityNewArchetype = nullptr;
			if (toArchetype == nullptr)
			{
				entityNewArchetype = m_ArchetypesMap.GetSingleComponentArchetype<TComponent>();
				if (entityNewArchetype == nullptr)
				{
					entityNewArchetype = m_ArchetypesMap.CreateSingleComponentArchetype<TComponent>(
						m_ComponentContextManager.GetOrCreateComponentContext<TComponent>(),
						m_StableContainers.GetOrCreateStableContainer<TComponent>()
					);
				}
			}
			else
			{
				entityNewArchetype = m_ArchetypesMap.GetArchetypeAfterAddComponent<TComponent>(*toArchetype);
				if (entityNewArchetype == nullptr)
				{
					entityNewArchetype = m_ArchetypesMap.CreateArchetypeAfterAddComponent<TComponent>(
						*toArchetype,
						m_ComponentContextManager.GetOrCreateComponentContext<TComponent>(),
						m_StableContainers.GetOrCreateStableContainer<TComponent>()
					);
				}

				componentContainerIndex = entityNewArchetype->FindTypeIndex<TComponent>();
			}

			return entityNewArchetype;
		}

#pragma endregion

#pragma region OBSERVERS
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
	private:
		std::vector<ComponentBaseRef> m_ActivationChangeComponentRefs = {};

		CreateEntityObserver* m_CreateEntityObserver = nullptr;
		DestroyEntityObserver* m_DestroyEntityObserver = nullptr;
		EnableEntityObserver* m_EnableEntityObserver = nullptr;
		DisableEntityObserver* m_DisableEntityObserver = nullptr;


	private:
		inline void InvokeEntityCreateObserver(const Entity& entity);

		inline void InvokeEntityDestroyObserver(const Entity& entity);

		inline void InvokeEntityEnableObserver(const Entity& entity);

		inline void InvokeEntityDisableObserver(const Entity& entity);

		void InvokeEntityAndComponentEnableObservers(const Entity& entity);

		void InvokeEntityAndComponentsDisableObservers(const Entity& entity);

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
			bool bRemove;
		};

		std::vector<ArchetypeRecordDelayedDestroyData> m_ArchetypesRecordsToDelayedRemove = {};

		bool m_PerformDelayedDestruction = false;

	private:
		void PerformDelayedDestruction();

		void DestroyDelayedEntities();

		void RemoveArchetypesRecordsDelayedToRemove();

		void DestroyDelayedEntity(const Entity& entity, bool bInvokeCallbacks);

		void AddEntityToDelayedDestroy(const Entity& entity, bool bInvokeCallbacks);

		void AddArchetypeRecordToDelayedRemove(Archetype* archetype, uint32_t index, bool bRemove, TypeID removedComponentTypeID)
		{
			archetype->SetRecordAsIntendedToDelayedDestroy(index);

			m_ArchetypesRecordsToDelayedRemove.push_back({ archetype, removedComponentTypeID, index, bRemove });
		}

#pragma endregion

#pragma region FLAGS:
	private:
		struct BoolSwitch final
		{
		public:
			BoolSwitch(bool& boolToSwitch) :
				m_Bool(boolToSwitch),
				m_FinalValue(!boolToSwitch)
			{
			}

			BoolSwitch(bool& boolToSwitch, const bool& startValue) :
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

#pragma region ITERATION:
	public:
		/// <summary>
		/// Helper methods for iterating over entites which containe component of type TComponentType.
		/// Iterates over entities in archetypes from first to last. During iteration with this method creating, destroying and adding or removing component is forbidden on all entities, because it can cause undefined behavior. 
		/// Destroying entites and adding or removing component to any entity, can cause that iteration index will go out of bound. 
		/// Creating new entities will not cause index out of bound but if created entity has component of type "TComponentType", it is undefined if that entity will be iterated or not in this function. If created entity will be placed in archetype that does not contain component of type "TComponentType" it is safe to create this entity.
		/// </summary>
		/// <typeparam name="Callable"></typeparam>
		/// <param name="func"></param>
		template<typename TComponent, typename Callable>
		void ForEach(Callable&& func)
		{
			decs::Entity entityBuffor = {};
			constexpr TypeID componentID = Type<TComponent>::ID();

			m_ArchetypesMap.IterateOverArchetypesWithType(componentID, [&](Archetype* archetype)
			{
				uint64_t entityCount = archetype->EntityCount();
				if (entityCount == 0)
				{
					return;
				}

				uint64_t compIdx = archetype->FindTypeIndex(componentID);

				auto& entitiesData = archetype->m_EntitiesData;
				auto packedContainer = archetype->m_TypeData[compIdx].m_PackedContainer;

				for (uint64_t idx = 0; idx < entityCount; idx++)
				{
					const auto& entityData = entitiesData[idx];
					if (entityData.IsActive())
					{
						if constexpr (std::is_invocable<Callable, Entity&, TComponent&>())
						{
							entityBuffor.Set(entityData.m_EntityData, this);
							func(entityBuffor, *static_cast<TComponent*>(packedContainer->GetComponentBasePtr(idx)));
						}
						else
						{
							func(*static_cast<TComponent*>(packedContainer->GetComponentBasePtr(idx)));
						}
					}
				}
			});
		}

#pragma endregion

#pragma region NO CALLBACKS methods:
	public:
		Entity CreateEntity_NoCallbacks(bool bIsActive = true);

		bool DestroyEntity_NoCallback(const Entity& entity);

	private:
		void SetEntityActive_NoCallback(const Entity& entity, bool bIsActive);

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

		template<typename TComponent, typename ...Args>
		inline TComponent* AddComponent_NoCallback(Entity entity, EntityData& entityData, Args&&... args)
		{
			if (!m_CanAddComponents) return nullptr;

			return AddStableComponent_NoCallback<TComponent>(entity, entityData, std::forward<Args>(args)...);
		}

		template<typename TComponent, typename ...Args>
		TComponent* AddStableComponent_NoCallback(const Entity& entity, EntityData& entityData, Args&&... args)
		{
			TYPE_ID_CONSTEXPR TypeID copmonentTypeID = Type<TComponent>::ID();

			if (!entityData.IsValidToPerformComponentOperation())
			{
				return nullptr;
			}

			auto currentComponent = GetStableComponentWithoutCheckingIsAlive<TComponent>(entityData);
			if (currentComponent != nullptr)
			{
				return currentComponent;
			}

			uint32_t componentContainerIndex = 0;
			Archetype* entityNewArchetype = GetArchetypeAfterAddComponent<TComponent>(entityData.m_Archetype, componentContainerIndex);
			ArchetypeTypeData& archetypeTypeData = entityNewArchetype->m_TypeData[componentContainerIndex];

			// Adding component to stable component container
			StableContainer<TComponent>* stableContainer = static_cast<StableContainer<TComponent>*>(archetypeTypeData.m_StableContainer);
			StableComponentRef componentNodeInfo = stableContainer->Emplace(std::forward<Args>(args)...);

			//StableComponentRef componentNodeInfo = {};
			// Adding component pointer to packed container in archetype
			archetypeTypeData.m_PackedContainer->EmplaceFromVoid(&componentNodeInfo);

			TComponent* componentPtr = static_cast<TComponent*>(componentNodeInfo.m_ComponentPtr);

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

			return componentPtr;
		}

		template<typename TComponent>
		bool RemoveComponent_NoCallback(Entity entity)
		{
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