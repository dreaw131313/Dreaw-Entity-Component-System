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
	struct DelayedComponentPair
	{
	public:
		EntityData* m_EntityData = nullptr;
		TypeID m_TypeID = std::numeric_limits<TypeID>::max();
		bool m_IsStable = false;

	public:
		DelayedComponentPair()
		{

		}

		DelayedComponentPair(
			EntityData* entityData,
			TypeID typeID,
			bool isStable
		) :
			m_EntityData(entityData), m_TypeID(typeID), m_IsStable(isStable)
		{

		}

		bool operator==(const decs::DelayedComponentPair& rhs) const
		{
			return m_EntityData == rhs.m_EntityData && m_TypeID == rhs.m_TypeID;
		}
	};
}

template<>
struct ::std::hash<decs::DelayedComponentPair>
{
	std::size_t operator()(const decs::DelayedComponentPair& data) const
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

	private:
		static constexpr uint64_t m_DefaultEntitiesChunkSize = 1000;
		static constexpr uint64_t m_DefaultEmptyEntitiesChunkSize = 100;

	public:
		Container();

		Container(
			uint64_t enititesChunkSize,
			uint64_t stableComponentDefaultChunkSize,
			uint64_t m_EmptyEntitiesChunkSize = 100
		);

		Container(
			EntityManager* entityManager,
			uint64_t stableComponentDefaultChunkSize,
			uint64_t m_EmptyEntitiesChunkSize = 100
		);

		~Container();

		Container(const Container& other) = delete;
		Container(Container&& other) = delete;

		Container& operator = (const Container& other) = delete;
		Container& operator = (Container&& other) = delete;

#pragma region Extension data
	/*public:
		template<typename T> 
		T* GetExtensionData()
		{
			return static_cast<T*>(m_ExtensionData);
		}

	private:
		void* m_ExtensionData = nullptr;*/

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

		bool DestroyEntity(Entity& entity);

		void DestroyOwnedEntities(bool invokeOnDestroyListeners = true);

		inline uint64_t GetEmptyEntitiesCount() const
		{
			return m_EmptyEntities.Size();
		}

	private:
		bool DestroyEntityInternal(Entity& entity);

		void SetEntityActive(Entity& entity, bool isActive);

		inline void AddToEmptyEntities(EntityData& data)
		{
			data.m_IndexInArchetype = (uint32_t)m_EmptyEntities.Size();
			m_EmptyEntities.EmplaceBack(&data);
		}

		inline void RemoveFromEmptyEntities(EntityData& data)
		{
			if (data.m_IndexInArchetype < m_EmptyEntities.Size() - 1)
			{
				m_EmptyEntities[data.m_IndexInArchetype] = m_EmptyEntities.Back();
			}
			m_EmptyEntities.PopBack();
		}

		inline void ValidateEntityInArchetype(const EntityRemoveSwapBackResult& swapBackResult)
		{
			if (swapBackResult.IsValid())
			{
				m_EntityManager->GetEntityData(swapBackResult.ID).m_IndexInArchetype = swapBackResult.Index;
			}
		}

		void InvokeEntityComponentDestructionObservers(Entity& entity);

#pragma endregion

#pragma region SPAWNING ENTITIES
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
			std::vector<SpawnComponentRefData> m_PrefabComponentRefs; // bool - true is stable, false unstable
			std::vector <Archetype*> m_SpawnArchetypes;
			std::vector<ComponentRefAsVoid> m_EntityComponentRefs;

		public:
			void Reserve(uint64_t size)
			{
				m_PrefabComponentRefs.reserve(size);
				m_EntityComponentRefs.reserve(size);
			}

			void Clear()
			{
				m_PrefabComponentRefs.clear();
				m_EntityComponentRefs.clear();
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

					auto eIt = m_EntityComponentRefs.begin();
					std::advance(eIt, refsStartIdx);
					m_EntityComponentRefs.erase(eIt, m_EntityComponentRefs.end());
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
				m_CompRefsStart((uint32_t)spawnData.m_EntityComponentRefs.size()),
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

#pragma region COMPONENTS:
	private:
		bool m_HaveOwnComponentContextManager = true;
		ComponentContextsManager* m_ComponentContextManager = nullptr;
	
	private:
		template<typename ComponentType, typename ...Args>
		inline typename component_type<ComponentType>::Type* AddComponent(EntityData& entityData, Args&&... args)
		{
			if constexpr (is_stable<ComponentType>::value)
			{
				return AddStableComponent<typename component_type<ComponentType>::Type>(entityData, std::forward<Args>(args)...);
			}
			else
			{
				return AddUnstableComponent<ComponentType>(entityData, std::forward<Args>(args)...);
			}
		}

		template<typename ComponentType, typename ...Args>
		ComponentType* AddUnstableComponent(EntityData& entityData, Args&&... args)
		{
			TYPE_ID_CONSTEXPR TypeID copmonentTypeID = Type<ComponentType>::ID();
			if (!m_CanAddComponents) return nullptr;

			if (entityData.IsValidToPerformComponentOperation())
			{
				if (m_PerformDelayedDestruction)
				{
					ComponentType* delayedToDestroyComponent = TryAddUnstableComponentDelayedToDestroy<ComponentType>(entityData);
					if (delayedToDestroyComponent != nullptr) { return delayedToDestroyComponent; }
				}
				else
				{
					auto currentComponent = GetComponentWithoutCheckingIsAlive<ComponentType>(entityData);
					if (currentComponent != nullptr) return currentComponent;
				}

				uint32_t componentContainerIndex = 0;
				Archetype* entityNewArchetype = GetArchetypeAfterAddUnstableComponent<ComponentType>(
					entityData.m_Archetype,
					componentContainerIndex
					);

				ArchetypeTypeData& archetypeTypeData = entityNewArchetype->m_TypeData[componentContainerIndex];
				PackedContainer<ComponentType>* container = reinterpret_cast<PackedContainer<ComponentType>*>(archetypeTypeData.m_PackedContainer);
				ComponentType* createdComponent = &container->m_Data.emplace_back(std::forward<Args>(args)...);

				uint32_t entityIndexBuffor = entityNewArchetype->EntitiesCount();
				entityNewArchetype->AddEntityData(entityData.m_ID, entityData.m_bIsActive);
				if (entityData.m_Archetype != nullptr)
				{
					entityNewArchetype->MoveEntityAfterAddComponent<ComponentType>(
						*entityData.m_Archetype,
						entityData.m_IndexInArchetype
						);
					auto result = entityData.m_Archetype->RemoveSwapBackEntity(entityData.m_IndexInArchetype);
					ValidateEntityInArchetype(result);
				}
				else
				{
					RemoveFromEmptyEntities(entityData);
				}

				entityData.m_IndexInArchetype = entityIndexBuffor;
				entityData.m_Archetype = entityNewArchetype;

				InvokeOnCreateComponentFromEntityDataAndVoidComponentPtr(archetypeTypeData.m_ComponentContext, createdComponent, entityData);
				return createdComponent;
			}
			return nullptr;
		}

		template<typename ComponentType, typename ...Args>
		ComponentType* AddStableComponent(EntityData& entityData, Args&&... args)
		{
			TYPE_ID_CONSTEXPR TypeID copmonentTypeID = Type<Stable<ComponentType>>::ID();

			if (!m_CanAddComponents) return nullptr;

			if (entityData.IsValidToPerformComponentOperation())
			{
				if (m_PerformDelayedDestruction)
				{
					ComponentType* delayedToDestroyComponent = TryAddStableComponentDelayedToDestroy<ComponentType>(entityData);
					if (delayedToDestroyComponent != nullptr) { return delayedToDestroyComponent; }
				}
				else
				{
					auto currentComponent = GetStableComponentWithoutCheckingIsAlive<ComponentType>(entityData);
					if (currentComponent != nullptr) return currentComponent;
				}

				uint32_t componentContainerIndex = 0;
				Archetype* entityNewArchetype = GetArchetypeAfterAddStableComponent<ComponentType>(entityData.m_Archetype, componentContainerIndex);
				ArchetypeTypeData& archetypeTypeData = entityNewArchetype->m_TypeData[componentContainerIndex];

				// Adding component to stable component container
				StableContainer<ComponentType>* stableContainer = static_cast<StableContainer<ComponentType>*>(archetypeTypeData.m_StableContainer);
				StableComponentRef componentNodeInfo = stableContainer->Emplace(std::forward<Args>(args)...);

				// Adding component pointer to packed container in archetype
				PackedContainer<Stable<ComponentType>>* packedPointersContainer = reinterpret_cast<PackedContainer<Stable<ComponentType>>*>(archetypeTypeData.m_PackedContainer);

				packedPointersContainer->EmplaceBack(
					static_cast<ComponentType*>(componentNodeInfo.m_ComponentPtr),
					componentNodeInfo.m_ChunkIndex,
					componentNodeInfo.m_Index
				);
				ComponentType* componentPtr = static_cast<ComponentType*>(componentNodeInfo.m_ComponentPtr);

				// Adding entity to archetype
				uint32_t entityIndexBuffor = entityNewArchetype->EntitiesCount();
				entityNewArchetype->AddEntityData(entityData.m_ID, entityData.m_bIsActive);
				if (entityData.m_Archetype != nullptr)
				{
					entityNewArchetype->MoveEntityAfterAddComponent<Stable<ComponentType>>(
						*entityData.m_Archetype,
						entityData.m_IndexInArchetype
						);
					auto result = entityData.m_Archetype->RemoveSwapBackEntity(entityData.m_IndexInArchetype);
					ValidateEntityInArchetype(result);
				}
				else
				{
					RemoveFromEmptyEntities(entityData);
				}

				entityData.m_IndexInArchetype = entityIndexBuffor;
				entityData.m_Archetype = entityNewArchetype;

				InvokeOnCreateComponentFromEntityDataAndVoidComponentPtr(archetypeTypeData.m_ComponentContext, componentPtr, entityData);
				return componentPtr;
			}
			return nullptr;
		}

		template<typename ComponentType>
		bool RemoveComponent(Entity& entity)
		{
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

		bool RemoveStableComponent(Entity& entity,  TypeID componentTypeID);

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
				uint32_t findTypeIndex = entityData.m_Archetype->FindTypeIndex<Stable<ComponentType>>();
				if (findTypeIndex != Limits::MaxComponentCount)
				{
					PackedContainer<Stable<ComponentType>>* container = static_cast<PackedContainer<Stable<ComponentType>>*>(entityData.m_Archetype->m_TypeData[findTypeIndex].m_PackedContainer);
					return container->GetAsPtr(entityData.m_IndexInArchetype);
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

		void InvokeOnCreateComponentFromEntityDataAndVoidComponentPtr(ComponentContextBase* componentContext, void* componentPtr, EntityData& entityData);

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
			TYPE_ID_CONSTEXPR TypeID copmonentTypeID = Type<Stable<ComponentType>>::ID();
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
		bool SetStableComponentChunkSize(uint64_t chunkSize)
		{
			return m_StableContainers.SetStableComponentChunkSize<T>(chunkSize);
		}

		template<typename T>
		uint64_t GetStableComponentChunkSize()
		{
			return m_StableContainers.GetStableComponentChunkSize<T>();
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
			TYPE_ID_CONSTEXPR TypeID id = Type<Stable<ComponentType>>::ID();

			Archetype* entityNewArchetype = nullptr;
			if (toArchetype == nullptr)
			{
				entityNewArchetype = m_ArchetypesMap.GetSingleComponentArchetype<Stable<ComponentType>>();
				if (entityNewArchetype == nullptr)
				{
					entityNewArchetype = m_ArchetypesMap.CreateSingleComponentArchetype<Stable<ComponentType>>(
						m_ComponentContextManager->GetOrCreateComponentContext<Stable<ComponentType>>(),
						m_StableContainers.GetOrCreateStableContainer<ComponentType>()
						);
				}
			}
			else
			{
				entityNewArchetype = m_ArchetypesMap.GetArchetypeAfterAddComponent<Stable<ComponentType>>(*toArchetype);
				if (entityNewArchetype == nullptr)
				{
					entityNewArchetype = m_ArchetypesMap.CreateArchetypeAfterAddComponent<Stable<ComponentType>>(
						*toArchetype,
						m_ComponentContextManager->GetOrCreateComponentContext<Stable<ComponentType>>(),
						m_StableContainers.GetOrCreateStableContainer<ComponentType>()
						);
				}

				componentContainerIndex = entityNewArchetype->FindTypeIndex<Stable<ComponentType>>();
			}

			return entityNewArchetype;
		}

#pragma endregion

#pragma region OBSERVERS
	private:
		std::vector<ComponentRefAsVoid> m_ComponentRefsToInvokeObserverCallbacks;

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

		void InvokeArchetypeOnCreateListeners(Archetype& archetype);

		void InvokeArchetypeOnDestroyListeners(Archetype& archetype);

#pragma endregion

#pragma region DELAYED DESTROY:
	private:
		std::vector<EntityData*> m_DelayedEntitiesToDestroy;
		ecsSet<DelayedComponentPair> m_DelayedComponentsToDestroy;

		bool m_PerformDelayedDestruction = false;

	private:
		void DestroyDelayedEntities();

		void DestroyDelayedEntity(EntityData& entityData);

		void DestroyDelayedComponents();

		void AddEntityToDelayedDestroy(Entity& entity);

		inline bool AddComponentToDelayedDestroy(EntityData* entityData, TypeID typeID, bool isStable)
		{
			auto pair = m_DelayedComponentsToDestroy.emplace(entityData, typeID, isStable);
			return pair.second;
		}

		inline bool RemoveComponentFromDelayedToDestroy(EntityData* entityData, TypeID typeID, bool isStable)
		{
			return m_DelayedComponentsToDestroy.erase({ entityData, typeID, isStable });
		}
#pragma endregion
	};
}