#pragma once
#include "Core.h"
#include "Enums.h"
#include "Type.h"

#include "Archetypes\ArchetypesMap.h"
#include "EntityManager.h"
#include "ComponentContextsManager.h"
#include "Observers.h"

#include "ObserversManager.h"
#include "decs\ComponentContainers\PackedContainer.h"
#include "ComponentRefs\ComponentRefAsVoid.h"

namespace decs
{
	struct DelayedComponentPair
	{
	public:
		EntityID m_EntityID = std::numeric_limits<EntityID>::max();
		TypeID m_TypeID = std::numeric_limits<TypeID>::max();

	public:
		DelayedComponentPair()
		{

		}

		DelayedComponentPair(
			const EntityID& entityID,
			const TypeID& typeID
		) :
			m_EntityID(entityID), m_TypeID(typeID)
		{

		}

		bool operator==(const decs::DelayedComponentPair& rhs) const
		{
			return m_EntityID == rhs.m_EntityID && m_TypeID == rhs.m_TypeID;
		}
	};
}

template<>
struct ::std::hash<decs::DelayedComponentPair>
{
	std::size_t operator()(const decs::DelayedComponentPair& data) const
	{
		uint64_t eIDHash = std::hash<decs::EntityID>{}(data.m_EntityID);
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
		friend class View;
		friend class Entity;

	public:
		Container();

		Container(
			const uint64_t& enititesChunkSize,
			const uint64_t& componentContainerChunkSize,
			const uint64_t& m_EmptyEntitiesChunkSize = 100
		);

		Container(
			EntityManager* entityManager,
			ComponentContextsManager* componentContextsManager,
			const uint64_t& componentContainerChunkSize,
			const uint64_t& m_EmptyEntitiesChunkSize = 100
		);

		~Container();

		Container(const Container& other) = delete;
		Container(Container&& other) = delete;

		Container& operator = (const Container& other) = delete;
		Container& operator = (Container&& other) = delete;

#pragma region UTILITY
	public:
		/// <summary>
		/// Some functions change internal state of container during invocation and if error will be thrown in this functions they can leave invalid internal state of Container object. This function brings back container to valid state.
		/// </summary>
		void ValidateInternalState();
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
		bool m_HaveOwnEntityManager = false;
		EntityManager* m_EntityManager = nullptr;
		ChunkedVector<EntityData*> m_EmptyEntities = { 100 };

	public:
		Entity CreateEntity(const bool& isActive = true);

		bool DestroyEntity(Entity& entity);

		void DestroyOwnedEntities(const bool& invokeOnDestroyListeners = true);

		inline uint64_t GetEmptyEntitiesCount() const
		{
			return m_EmptyEntities.Size();
		}

	private:
		void SetEntityActive(const EntityID& entity, const bool& isActive);

		bool IsEntityActive(const EntityID& entity) const
		{
			return m_EntityManager->IsEntityActive(entity);
		}

		inline uint32_t ComponentsCount(const EntityID& entity) const
		{
			return m_EntityManager->ComponentsCount(entity);
		}

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

#pragma endregion

#pragma region SPAWNING ENTIES
	private:
		struct SpawnData
		{
		public:
			std::vector<ComponentRefAsVoid> m_PrefabComponentRefs;
			std::vector < Archetype*> m_SpawnArchetypes;
			std::vector<ComponentRefAsVoid> m_EntityComponentRefs;

		public:
			void Reserve(const uint64_t& size)
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

			void PopBackSpawnState(const uint64_t& archetypeIdx, const uint64_t& refsStartIdx)
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
			const bool& isActive = true
		);

		bool Spawn(
			const Entity& prefab,
			std::vector<Entity>& spawnedEntities,
			const uint64_t& spawnCount,
			const bool& areActive = true
		);

		bool Spawn(
			const Entity& prefab,
			const uint64_t& spawnCount,
			const bool& areActive = true
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
		ComponentType* AddComponent(const EntityID& e, Args&&... args)
		{
			constexpr TypeID copmonentTypeID = Type<ComponentType>::ID();
			if (e >= m_EntityManager->GetEntitiesDataCount()) return nullptr;
			EntityData& entityData = m_EntityManager->GetEntityData(e);

			return AddComponent(entityData, std::forward<Args>(args)...);
		}

		template<typename ComponentType, typename ...Args>
		ComponentType* AddComponent(EntityData& entityData, Args&&... args)
		{
			constexpr TypeID copmonentTypeID = Type<ComponentType>::ID();
			if (!m_CanAddComponents) return nullptr;

			if (entityData.IsValidToPerformComponentOperation())
			{
				if (m_PerformDelayedDestruction)
				{
					ComponentType* delayedToDestroyComponent = TryAddComponentDelayedToDestroy<ComponentType>(entityData);
					if (delayedToDestroyComponent != nullptr) { return delayedToDestroyComponent; }
				}
				else
				{
					auto currentComponent = GetComponentWithoutCheckingIsAlive<ComponentType>(entityData);
					if (currentComponent != nullptr) return currentComponent;
				}

				ComponentContextBase* newComponentContext;
				uint32_t componentContainerIndex = 0;
				Archetype* entityNewArchetype = GetArchetypeAfterAddComponent<ComponentType>(
					entityData.m_Archetype,
					newComponentContext,
					componentContainerIndex
					);

				PackedContainer<ComponentType>* container = reinterpret_cast<PackedContainer<ComponentType>*>(entityNewArchetype->m_PackedContainers[componentContainerIndex]);
				ComponentType* createdComponent = &container->m_Data.emplace_back(std::forward<Args>(args)...);

				uint32_t entityIndexBuffor = entityNewArchetype->EntitiesCount();
				entityNewArchetype->AddEntityData(entityData.m_ID, entityData.m_IsActive);
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

				InvokeOnCreateComponentFromEntityDataAndVoidComponentPtr(newComponentContext, createdComponent, entityData);
				return createdComponent;
			}
			return nullptr;
		}

		bool RemoveComponent(Entity& e, const TypeID& componentTypeID);

		bool RemoveComponent(const EntityID& e, const TypeID& componentTypeID);

		template<typename ComponentType>
		ComponentType* GetComponent(const EntityID& e) const
		{
			if (e < m_EntityManager->GetEntitiesDataCount())
			{
				EntityData& entityData = m_EntityManager->GetEntityData(e);
				return GetComponent(entityData);
			}

			return nullptr;
		}

		template<typename ComponentType>
		ComponentType* GetComponent(EntityData& entityData) const
		{
			if (entityData.m_Archetype != nullptr && entityData.IsValidToPerformComponentOperation())
			{
				uint32_t findTypeIndex = entityData.m_Archetype->FindTypeIndex<ComponentType>();
				if (findTypeIndex != Limits::MaxComponentCount)
				{
					PackedContainer<ComponentType>* container = (PackedContainer<ComponentType>*)entityData.m_Archetype->m_PackedContainers[findTypeIndex];

					return &container->m_Data[entityData.m_IndexInArchetype];
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
					PackedContainer<ComponentType>* container = (PackedContainer<ComponentType>*)entityData.m_Archetype->m_PackedContainers[findTypeIndex];
					return &container->m_Data[entityData.m_IndexInArchetype];
				}
			}
			return nullptr;
		}

		template<typename ComponentType>
		bool HasComponent(const EntityID& e) const
		{
			if (e < m_EntityManager->GetEntitiesDataCount())
			{
				const EntityData& data = m_EntityManager->GetConstEntityData(e);
				return HasComponent(data);
			}
			return false;
		}

		template<typename ComponentType>
		bool HasComponent(EntityData& entityData) const
		{
			if (entityData.m_Archetype != nullptr && entityData.IsValidToPerformComponentOperation())
			{
				uint32_t index = entityData.m_Archetype->FindTypeIndex(Type<ComponentType>::ID());
				return index != Limits::MaxComponentCount;
			}
			return false;
		}

		void InvokeOnCreateComponentFromEntityDataAndVoidComponentPtr(ComponentContextBase* componentContext, void* componentPtr, EntityData& entityData);

		template<typename ComponentType>
		ComponentType* TryAddComponentDelayedToDestroy(
			EntityData& entityData
		)
		{
			constexpr TypeID copmonentTypeID = Type<ComponentType>::ID();
			if (entityData.m_Archetype != nullptr)
			{
				uint32_t componentIndex = entityData.m_Archetype->FindTypeIndex<ComponentType>();
				if (componentIndex != Limits::MaxComponentCount)
				{
					RemoveComponentFromDelayedToDestroy(entityData.m_ID, copmonentTypeID);
					PackedContainer<ComponentType>* container = (PackedContainer<ComponentType>*)entityData.m_Archetype->m_PackedContainers[componentIndex];
					return &container->m_Data[entityData.m_IndexInArchetype];
				}
			}

			return nullptr;
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
		void DestroyEntitesInArchetypes(Archetype& archetype, const bool& invokeOnDestroyListeners = true);

		template<typename ComponentType>
		Archetype* GetArchetypeAfterAddComponent(
			Archetype* toArchetype,
			ComponentContextBase*& newComponentContext,
			uint32_t& componentContainerIndex
		)
		{
			Archetype* entityNewArchetype = nullptr;
			if (toArchetype == nullptr)
			{
				entityNewArchetype = m_ArchetypesMap.GetSingleComponentArchetype<ComponentType>();
				if (entityNewArchetype == nullptr)
				{
					auto context = m_ComponentContextManager->GetOrCreateComponentContext<ComponentType>();
					newComponentContext = context;
					entityNewArchetype = m_ArchetypesMap.CreateSingleComponentArchetype<ComponentType>(
						newComponentContext
						);
				}
				else
				{
					newComponentContext = entityNewArchetype->m_ComponentContexts[componentContainerIndex];
				}
			}
			else
			{
				entityNewArchetype = m_ArchetypesMap.GetArchetypeAfterAddComponent<ComponentType>(
					*toArchetype
					);
				if (entityNewArchetype == nullptr)
				{
					auto context = m_ComponentContextManager->GetOrCreateComponentContext<ComponentType>();
					newComponentContext = context;
					entityNewArchetype = m_ArchetypesMap.CreateArchetypeAfterAddComponent<ComponentType>(
						*toArchetype,
						newComponentContext
						);
				}
				else
				{
					newComponentContext = entityNewArchetype->m_ComponentContexts[componentContainerIndex];
				}
				componentContainerIndex = entityNewArchetype->FindTypeIndex<ComponentType>();
			}

			return entityNewArchetype;
		}

#pragma endregion

#pragma region OBSERVERS
	private:
		ObserversManager* m_ObserversManager = nullptr;
		std::vector<ComponentRefAsVoid> m_ComponentRefsToInvokeObserverCallbacks;

	public:
		bool SetObserversManager(ObserversManager* observersManager);

		void InvokeEntitesOnCreateListeners();

		void InvokeEntitesOnDestroyListeners();

	private:
		inline void InvokeEntityCreationObservers(Entity& entity)
		{
			if (m_ObserversManager != nullptr)
			{
				m_ObserversManager->InvokeEntityCreationObservers(entity);
			}
		}

		inline void InvokeEntityDestructionObservers(Entity& entity)
		{
			if (m_ObserversManager != nullptr)
			{
				m_ObserversManager->InvokeEntityDestructionObservers(entity);
			}
		}

		inline void InvokeEntityActivationObservers(Entity& entity)
		{
			if (m_ObserversManager != nullptr)
			{
				m_ObserversManager->InvokeEntityActivationObservers(entity);
			}
		}

		inline void InvokeEntityDeactivationObservers(Entity& entity)
		{
			if (m_ObserversManager != nullptr)
			{
				m_ObserversManager->InvokeEntityDeactivationObservers(entity);
			}
		}

		void InvokeArchetypeOnCreateListeners(Archetype& archetype);

		void InvokeArchetypeOnDestroyListeners(Archetype& archetype);

#pragma endregion

#pragma region DELAYED DESTROY:
	private:
		std::vector<EntityID> m_DelayedEntitiesToDestroy;
		ecsSet<DelayedComponentPair> m_DelayedComponentsToDestroy;

		bool m_PerformDelayedDestruction = false;

	private:
		void DestroyDelayedEntities();

		void DestroyDelayedComponents();

		void AddEntityToDelayedDestroy(const Entity& entity);

		void AddEntityToDelayedDestroy(const EntityID& entityID);

		inline bool AddComponentToDelayedDestroy(const EntityID& entityID, const TypeID typeID)
		{
			auto pair = m_DelayedComponentsToDestroy.insert({ entityID, typeID });
			return pair.second;
		}

		inline bool RemoveComponentFromDelayedToDestroy(const EntityID& entityID, const TypeID& typeID)
		{
			return m_DelayedComponentsToDestroy.erase({ entityID, typeID });
		}
#pragma endregion
	};
}