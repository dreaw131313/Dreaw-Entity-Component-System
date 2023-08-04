#pragma once
#include "Core.h"
#include "Type.h"

#include "Archetypes\ArchetypesMap.h"
#include "EntityManager.h"
#include "ComponentContext\ComponentContextsManager.h"

#include "Observers\Observers.h"
#include "Observers\ObserversManager.h"

#include "decs\ComponentContainers\PackedContainer.h"
#include "decs\ComponentContainers\StableContainer.h"
#include "ComponentRefs\ComponentRefAsVoid.h"

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
		template<typename T>
		friend class ContainerSerializer;

		NON_COPYABLE(Container);
		NON_MOVEABLE(Container);

	private:
		static constexpr uint64_t m_DefaultEntitiesChunkSize = 1000;
		static constexpr uint64_t m_DefaultEmptyEntitiesChunkSize = 100;

	public:
		Container();

		Container(
			uint64_t enititesChunkSize,
			uint32_t stableComponentDefaultChunkSize,
			uint64_t m_EmptyEntitiesChunkSize = 100
		);

		Container(
			EntityManager* entityManager,
			uint32_t stableComponentDefaultChunkSize,
			uint64_t m_EmptyEntitiesChunkSize = 100
		);

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
		/// Some functions change internal state of container during invocation and if error will be thrown in this functions they can leave invalid internal state of Container object. This function brings back container to valid state.
		/// </summary>
		void ValidateInternalState();

		uint64_t GetCreatedEntitiesCount()
		{
			return m_EntityManager->GetCreatedEntitiesCount();
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
		bool m_IsDestroyingOwnedEntities = false;
		bool m_CanCreateEntities = true;
		bool m_CanDestroyEntities = true;
		bool m_CanSpawn = true;
		bool m_CanAddComponents = true;
		bool m_CanRemoveComponents = true;
#pragma endregion

#pragma region ENTITIES:
	private:
		TChunkedVector<EntityData*> m_EmptyEntities = { m_DefaultEmptyEntitiesChunkSize };
		EntityManager* m_EntityManager = nullptr;
		bool m_HaveOwnEntityManager = false;

	public:
		Entity CreateEntity(bool isActive = true);

	public:
		bool DestroyEntity(Entity& entity);

		void DestroyOwnedEntities(bool invokeOnDestroyListeners = true);

		inline uint64_t GetEmptyEntitiesCount() const
		{
			return m_EmptyEntities.Size();
		}

	private:
		bool DestroyEntityInternal(Entity& entity);

		void SetEntityActive(Entity& entity, bool bIsActive);

		void AddToEmptyEntitiesRightAfterNewEntityCreation(EntityData& data);

		void AddToEmptyEntities(EntityData& data);

		void RemoveFromEmptyEntities(EntityData& data);

		void InvokeEntityComponentDestructionObservers(Entity& entity);

		EntityData* CreateAliveEntityData(bool bIsActive);

#pragma endregion

#pragma region RESERVING ENTITIES:
	public:
		void ReserveEntities(uint32_t entitiesToReserve);

		void FreeReservedEntities();

	private:
		uint32_t m_ReservedEntitiesCount = 0;
		std::vector<EntityData*> m_ReservedEntityData;

#pragma endregion

#pragma region SPAWNING ENTITIES:
	private:
		struct SpawnComponentRefData
		{
		public:
			bool m_IsStable = false;
			StableContainerBase* m_StableContainer = nullptr;
			ComponentRefAsVoid m_ComponentRef;

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
			std::vector<ComponentRefAsVoid> m_SpawnedEntityComponentRefs;

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
		SpawnData m_SpawnData;

	public:
		Entity Spawn(
			const Entity& prefab,
			bool isActive = true
		);

		bool Spawn(
			const Entity& prefab,
			std::vector<Entity>& spawnedEntities,
			uint64_t spawnCount,
			bool areActive = true
		);

		bool Spawn(
			const Entity& prefab,
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

#pragma endregion

#pragma region Entity operation data
		/*
	private:
		enum class EEntityComponentOperationType
		{
			None = 0,
			AddComponent,
			RemoveComponent,
		};

		struct EntityComponentOperationData
		{
		public:
			EntityData* m_EntityData = nullptr;
			TypeID m_ComponentTypeID = 0;
			EEntityComponentOperationType m_OperationType = EEntityComponentOperationType::None;

		public:
		};

		EntityComponentOperationData m_EntityComponentOperationData = {};

	private:
		inline void BeginEntityComponentOperationData(
			EntityData* entityData,
			TypeID componentTypeID,
			EEntityComponentOperationType operationType
		)
		{
			m_EntityComponentOperationData.m_EntityData = entityData;
			m_EntityComponentOperationData.m_ComponentTypeID = componentTypeID;
			m_EntityComponentOperationData.m_OperationType = operationType;
		}

		inline void EndEntityComponentOperationData()
		{
			m_EntityComponentOperationData.m_EntityData = nullptr;
			m_EntityComponentOperationData.m_ComponentTypeID = 0;
			m_EntityComponentOperationData.m_OperationType = EEntityComponentOperationType::None;
		}

		*/
#pragma endregion

#pragma region COMPONENTS:
	private:
		bool m_HaveOwnComponentContextManager = true;
		ComponentContextsManager* m_ComponentContextManager = nullptr;

	private:
		template<typename ComponentType, typename ...Args>
		inline typename component_type<ComponentType>::Type* AddComponent(Entity& entity, EntityData& entityData, Args&&... args)
		{
			if (!m_CanAddComponents) return nullptr;

			if constexpr (is_stable<ComponentType>::value)
			{
				return AddStableComponent<typename component_type<ComponentType>::Type>(entity, entityData, std::forward<Args>(args)...);
			}
			else
			{
				return AddUnstableComponent<ComponentType>(entity, entityData, std::forward<Args>(args)...);
			}
		}

		template<typename ComponentType, typename ...Args>
		ComponentType* AddUnstableComponent(Entity& entity, EntityData& entityData, Args&&... args)
		{
			TYPE_ID_CONSTEXPR TypeID copmonentTypeID = Type<ComponentType>::ID();

			if (entityData.IsValidToPerformComponentOperation())
			{
				auto currentComponent = GetComponentWithoutCheckingIsAlive<ComponentType>(entityData);
				if (currentComponent != nullptr)
				{
					return currentComponent;
				}

				uint32_t componentContainerIndex = 0;
				Archetype* entityNewArchetype = GetArchetypeAfterAddUnstableComponent<ComponentType>(
					entityData.m_Archetype,
					componentContainerIndex
				);

				ArchetypeTypeData& archetypeTypeData = entityNewArchetype->m_TypeData[componentContainerIndex];
				PackedContainer<ComponentType>* container = reinterpret_cast<PackedContainer<ComponentType>*>(archetypeTypeData.m_PackedContainer);
				ComponentType* createdComponent = &container->m_Data.emplace_back(std::forward<Args>(args)...);

				uint32_t entityIndexBuffor = entityNewArchetype->EntityCount();
				if (entityData.m_Archetype != nullptr)
				{
					entityNewArchetype->MoveEntityComponentsAfterAddComponent<ComponentType>(
						entityData.m_Archetype,
						entityData.m_IndexInArchetype,
						&entityData
					);
				}
				else
				{
					RemoveFromEmptyEntities(entityData);
					entityNewArchetype->AddEntityData(&entityData);
				}

				InvokeOnCreateComponentFromEntityDataAndVoidComponentPtr(entity, archetypeTypeData.m_ComponentContext, createdComponent, entityData);
				return createdComponent;
			}
			return nullptr;
		}

		template<typename ComponentType, typename ...Args>
		ComponentType* AddStableComponent(Entity& entity, EntityData& entityData, Args&&... args)
		{
			TYPE_ID_CONSTEXPR TypeID copmonentTypeID = Type<stable<ComponentType>>::ID();

			if (entityData.IsValidToPerformComponentOperation())
			{
				auto currentComponent = GetStableComponentWithoutCheckingIsAlive<ComponentType>(entityData);
				if (currentComponent != nullptr)
				{
					return currentComponent;
				}

				uint32_t componentContainerIndex = 0;
				Archetype* entityNewArchetype = GetArchetypeAfterAddStableComponent<ComponentType>(entityData.m_Archetype, componentContainerIndex);
				ArchetypeTypeData& archetypeTypeData = entityNewArchetype->m_TypeData[componentContainerIndex];

				// Adding component to stable component container
				StableContainer<ComponentType>* stableContainer = static_cast<StableContainer<ComponentType>*>(archetypeTypeData.m_StableContainer);
				StableComponentRef componentNodeInfo = stableContainer->Emplace(std::forward<Args>(args)...);

				//StableComponentRef componentNodeInfo = {};
				// Adding component pointer to packed container in archetype
				archetypeTypeData.m_PackedContainer->EmplaceFromVoid(&componentNodeInfo);

				ComponentType* componentPtr = static_cast<ComponentType*>(componentNodeInfo.m_ComponentPtr);

				// Adding entity to archetype
				uint32_t entityIndexBuffor = entityNewArchetype->EntityCount();
				if (entityData.m_Archetype != nullptr)
				{
					entityNewArchetype->MoveEntityComponentsAfterAddComponent<stable<ComponentType>>(
						entityData.m_Archetype,
						entityData.m_IndexInArchetype,
						&entityData
					);
				}
				else
				{
					RemoveFromEmptyEntities(entityData);
					entityNewArchetype->AddEntityData(&entityData);
				}

				InvokeOnCreateComponentFromEntityDataAndVoidComponentPtr(entity, archetypeTypeData.m_ComponentContext, componentPtr, entityData);
				return componentPtr;
			}
			return nullptr;
		}

		template<typename ComponentType>
		bool RemoveComponent(Entity& entity)
		{
			if (!m_CanRemoveComponents)
			{
				return false;
			}

			if constexpr (is_stable<ComponentType>::value)
			{
				return RemoveStableComponent(entity, Type<ComponentType>::ID());
			}
			else
			{
				return RemoveUnstableComponent(entity, Type<ComponentType>::ID());
			}
		}

		bool RemoveUnstableComponent(Entity& entity, TypeID componentTypeID);

		bool RemoveStableComponent(Entity& entity, TypeID componentTypeID);

		template<typename ComponentType>
		typename component_type<ComponentType>::Type* GetComponent(EntityID e) const
		{
			if (e < m_EntityManager->GetEntitiesDataCount())
			{
				EntityData& entityData = m_EntityManager->GetEntityData(e);
				return GetComponent<ComponentType>(entityData);
			}

			return nullptr;
		}

		template<typename ComponentType>
		typename component_type<ComponentType>::Type* GetComponent(EntityData& entityData) const
		{
			if (entityData.m_Archetype != nullptr && entityData.IsValidToPerformComponentOperation())
			{
				uint32_t findTypeIndex = entityData.m_Archetype->FindTypeIndex<ComponentType>();
				if (findTypeIndex != Limits::MaxComponentCount)
				{
					PackedContainer<ComponentType>* container = (PackedContainer<ComponentType>*)entityData.m_Archetype->m_TypeData[findTypeIndex].m_PackedContainer;
					if constexpr (is_stable<ComponentType>::value)
					{
						return (typename component_type<ComponentType>::Type*)container->m_Data[entityData.m_IndexInArchetype].m_ComponentPtr;
					}
					else
					{
						return &container->m_Data[entityData.m_IndexInArchetype];
					}
				}
			}
			return nullptr;
		}

		template<typename ComponentType>
		ComponentType* GetComponentWithoutCheckingIsAlive(EntityData& entityData) const
		{
			if (entityData.m_Archetype != nullptr)
			{
				uint32_t findTypeIndex = entityData.m_Archetype->FindTypeIndex<ComponentType>();
				if (findTypeIndex != Limits::MaxComponentCount)
				{
					PackedContainer<ComponentType>* container = (PackedContainer<ComponentType>*)entityData.m_Archetype->m_TypeData[findTypeIndex].m_PackedContainer;
					return &container->m_Data[entityData.m_IndexInArchetype];
				}
			}
			return nullptr;
		}

		template<typename ComponentType>
		ComponentType* GetStableComponentWithoutCheckingIsAlive(EntityData& entityData) const
		{
			if (entityData.m_Archetype != nullptr)
			{
				uint32_t findTypeIndex = entityData.m_Archetype->FindTypeIndex<stable<ComponentType>>();
				if (findTypeIndex != Limits::MaxComponentCount)
				{
					PackedContainer<stable<ComponentType>>* container = static_cast<PackedContainer<stable<ComponentType>>*>(entityData.m_Archetype->m_TypeData[findTypeIndex].m_PackedContainer);
					return static_cast<ComponentType*>(container->m_Data[entityData.m_IndexInArchetype].m_ComponentPtr);
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

		template<typename ComponentType>
		bool HasComponent(EntityData& entityData) const
		{
			return HasComponentInternal(entityData, Type<ComponentType>::ID());
		}

		void InvokeOnCreateComponentFromEntityDataAndVoidComponentPtr(Entity& entity, ComponentContextBase* componentContext, void* componentPtr, EntityData& entityData);

		template<typename ComponentType>
		ComponentType* TryAddUnstableComponentDelayedToDestroy(
			EntityData& entityData
		)
		{
			TYPE_ID_CONSTEXPR TypeID copmonentTypeID = Type<ComponentType>::ID();
			if (entityData.m_Archetype != nullptr)
			{
				uint32_t componentIndex = entityData.m_Archetype->FindTypeIndex<ComponentType>();
				if (componentIndex != Limits::MaxComponentCount)
				{
					RemoveComponentFromDelayedToDestroy(&entityData, copmonentTypeID, false);
					PackedContainer<ComponentType>* container = (PackedContainer<ComponentType>*)entityData.m_Archetype->m_TypeData[componentIndex].m_PackedContainer;
					return &container->m_Data[entityData.m_IndexInArchetype];
				}
			}

			return nullptr;
		}

		template<typename ComponentType>
		ComponentType* TryAddStableComponentDelayedToDestroy(
			EntityData& entityData
		)
		{
			TYPE_ID_CONSTEXPR TypeID copmonentTypeID = Type<stable<ComponentType>>::ID();
			if (entityData.m_Archetype != nullptr)
			{
				uint32_t componentIndex = entityData.m_Archetype->FindTypeIndex<ComponentType>();
				if (componentIndex != Limits::MaxComponentCount)
				{
					RemoveComponentFromDelayedToDestroy(&entityData, copmonentTypeID, true);
					return static_cast<ComponentType*>(entityData.m_Archetype->m_TypeData[componentIndex].m_PackedContainer->GetComponentPtrAsVoid(entityData.m_IndexInArchetype));
				}
			}

			return nullptr;
		}

#pragma endregion

#pragma region STABLE COMPONENTS
	private:
		StableContainersManager m_StableContainers = { 100 };
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
		void DestroyEntitesInArchetypes(Archetype& archetype, bool invokeOnDestroyListeners = true);

		template<typename ComponentType>
		Archetype* GetArchetypeAfterAddUnstableComponent(
			Archetype* toArchetype,
			uint32_t& componentContainerIndex
		)
		{
			TYPE_ID_CONSTEXPR TypeID id = Type<ComponentType>::ID();

			Archetype* entityNewArchetype = nullptr;
			if (toArchetype == nullptr)
			{
				entityNewArchetype = m_ArchetypesMap.GetSingleComponentArchetype<ComponentType>();
				if (entityNewArchetype == nullptr)
				{
					entityNewArchetype = m_ArchetypesMap.CreateSingleComponentArchetype<ComponentType>(
						m_ComponentContextManager->GetOrCreateComponentContext<ComponentType>(),
						nullptr
					);
				}
			}
			else
			{
				entityNewArchetype = m_ArchetypesMap.GetArchetypeAfterAddComponent<ComponentType>(*toArchetype);
				if (entityNewArchetype == nullptr)
				{
					entityNewArchetype = m_ArchetypesMap.CreateArchetypeAfterAddComponent<ComponentType>(
						*toArchetype,
						m_ComponentContextManager->GetOrCreateComponentContext<ComponentType>(),
						nullptr
					);
				}

				componentContainerIndex = entityNewArchetype->FindTypeIndex<ComponentType>();
			}

			return entityNewArchetype;
		}

		template<typename ComponentType>
		Archetype* GetArchetypeAfterAddStableComponent(
			Archetype* toArchetype,
			uint32_t& componentContainerIndex
		)
		{
			TYPE_ID_CONSTEXPR TypeID id = Type<stable<ComponentType>>::ID();

			Archetype* entityNewArchetype = nullptr;
			if (toArchetype == nullptr)
			{
				entityNewArchetype = m_ArchetypesMap.GetSingleComponentArchetype<stable<ComponentType>>();
				if (entityNewArchetype == nullptr)
				{
					entityNewArchetype = m_ArchetypesMap.CreateSingleComponentArchetype<stable<ComponentType>>(
						m_ComponentContextManager->GetOrCreateComponentContext<stable<ComponentType>>(),
						m_StableContainers.GetOrCreateStableContainer<ComponentType>()
					);
				}
			}
			else
			{
				entityNewArchetype = m_ArchetypesMap.GetArchetypeAfterAddComponent<stable<ComponentType>>(*toArchetype);
				if (entityNewArchetype == nullptr)
				{
					entityNewArchetype = m_ArchetypesMap.CreateArchetypeAfterAddComponent<stable<ComponentType>>(
						*toArchetype,
						m_ComponentContextManager->GetOrCreateComponentContext<stable<ComponentType>>(),
						m_StableContainers.GetOrCreateStableContainer<ComponentType>()
					);
				}

				componentContainerIndex = entityNewArchetype->FindTypeIndex<stable<ComponentType>>();
			}

			return entityNewArchetype;
		}

#pragma endregion

#pragma region OBSERVERS
	public:
		bool SetObserversManager(ObserversManager* observersManager);

		void InvokeEntitesOnCreateListeners();

		void InvokeEntitesOnDestroyListeners();

	private:
		inline void InvokeEntityCreationObservers(Entity& entity)
		{
			if (m_ComponentContextManager->m_ObserversManager != nullptr)
			{
				m_ComponentContextManager->m_ObserversManager->InvokeEntityCreationObservers(entity);
			}
		}

		inline void InvokeEntityDestructionObservers(Entity& entity)
		{
			if (m_ComponentContextManager->m_ObserversManager != nullptr)
			{
				m_ComponentContextManager->m_ObserversManager->InvokeEntityDestructionObservers(entity);
			}
		}

		inline void InvokeEntityActivationObservers(Entity& entity)
		{
			if (m_ComponentContextManager->m_ObserversManager != nullptr)
			{
				m_ComponentContextManager->m_ObserversManager->InvokeEntityActivationObservers(entity);
			}
		}

		inline void InvokeEntityDeactivationObservers(Entity& entity)
		{
			if (m_ComponentContextManager->m_ObserversManager != nullptr)
			{
				m_ComponentContextManager->m_ObserversManager->InvokeEntityDeactivationObservers(entity);
			}
		}

		void InvokeArchetypeOnCreateListeners(Archetype& archetype, std::vector<ComponentRefAsVoid>& componentRefsToInvokeObserverCallbacks);

		void InvokeArchetypeOnDestroyListeners(Archetype& archetype);

#pragma endregion

#pragma region DELAYED DESTROY:
	private:
		std::vector<EntityData*> m_DelayedEntitiesToDestroy;

		bool m_PerformDelayedDestruction = false;

	private:
		void DestroyDelayedEntities();

		void DestroyDelayedEntity(Entity& entity);

		void AddEntityToDelayedDestroy(Entity& entity);
#pragma endregion
	};
}