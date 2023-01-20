#pragma once
#include "Core.h"
#include "Type.h"
#include "EntityManager.h"
#include "ComponentContext.h"
#include "ComponentContainer.h"
#include "Observers.h"

#include "ObserversManager.h"


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
struct std::hash<decs::DelayedComponentPair>
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

	enum class ChunkSizeType
	{
		ElementsCount,
		BytesCount
	};


	class Container
	{
		template<typename ...Types>
		friend class View;
		friend class Entity;

	public:
		Container();

		Container(const Container& other) = delete;
		Container(Container&& other) = delete;

		Container(
			const uint64_t& enititesChunkSize,
			const ChunkSizeType& componentContainerChunkSizeType,
			const uint64_t& componentContainerChunkSize,
			const bool& invokeEntityActivationStateListeners,
			const uint64_t m_EmptyEntitiesChunkSize = 100
		);

		Container(
			EntityManager* entityManager,
			const ChunkSizeType& componentContainerChunkSizeType,
			const uint64_t& componentContainerChunkSize,
			const bool& invokeEntityActivationStateListeners,
			const uint64_t m_EmptyEntitiesChunkSize = 100
		);

		~Container();

		Container& operator = (const Container& other) = delete;
		Container& operator = (Container&& other) = delete;

	private:
		uint64_t m_ComponentContainerChunkSize = 1000;
		ChunkSizeType m_ContainerSizeType = ChunkSizeType::ElementsCount;

	public:
		inline void SetDefaultComponentChunkSize(const uint64_t& size, const ChunkSizeType& sizeType)
		{
			m_ContainerSizeType = sizeType;
			m_ComponentContainerChunkSize = size;
		}

#pragma region FLAGS:
	private:
		struct BoolSwitch final
		{
		public:
			BoolSwitch(bool& boolToSwitch) :
				m_Bool(boolToSwitch)
			{
			}

			BoolSwitch(bool& boolToSwitch, const bool& startValue) :
				m_Bool(boolToSwitch)
			{
				m_Bool = startValue;
			}

			BoolSwitch(const BoolSwitch&) = delete;
			BoolSwitch(BoolSwitch&&) = delete;

			BoolSwitch& operator=(const BoolSwitch&) = delete;
			BoolSwitch& operator=(BoolSwitch&&) = delete;

			~BoolSwitch()
			{
				m_Bool = !m_Bool;
			}

		private:
			bool& m_Bool;
		};
	private:
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

		bool m_bInvokeEntityActivationStateListeners = true;

		ChunkedVector<EntityData*> m_EmptyEntities = { 100 };

	public:
		inline uint64_t GetAliveEntitesCount() const
		{
			return m_EntityManager->GetCreatedEntitiesCount();
		}

		Entity CreateEntity(const bool& isActive = true);

		bool DestroyEntity(Entity& entity);

		void DestroyOwnedEntities(const bool& invokeOnDestroyListeners = true);

		inline uint64_t GetEmptyEntitiesCount() const
		{
			return m_EmptyEntities.Size();
		}

	private:

		inline bool IsEntityAlive(const EntityID& entity) const
		{
			return m_EntityManager->IsEntityAlive(entity);
		}

		inline uint32_t GetEntityVersion(const EntityID& entity) const
		{
			return m_EntityManager->GetEntityVersion(entity);
		}

		void SetEntityActive(const EntityID& entity, const bool& isActive);

		bool IsEntityActive(const EntityID& entity) const
		{
			return m_EntityManager->IsEntityActive(entity);
		}

		inline uint32_t GetComponentsCount(const EntityID& entity) const
		{
			return m_EntityManager->GetComponentsCount(entity);
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
#pragma endregion

#pragma region SPAWNING ENTIES
	private:
		struct PrefabSpawnComponentContext
		{
		public:
			TypeID ComponentType = std::numeric_limits<TypeID>::max();
			ComponentContextBase* PrefabComponentContext = nullptr;
			ComponentContextBase* ComponentContext = nullptr;

			void* CopiedComponentPointer = nullptr;
			ComponentCopyData ComponentData = {};
		public:
			PrefabSpawnComponentContext()
			{

			}

			PrefabSpawnComponentContext(
				const TypeID& componentType,
				ComponentContextBase* prefabContext,
				ComponentContextBase* context
			) :
				ComponentType(componentType), PrefabComponentContext(prefabContext), ComponentContext(context)
			{

			}
		};

		struct PrefabSpawnData
		{
		public:
			Archetype* m_SpawnArchetype = nullptr;
			std::vector<PrefabSpawnComponentContext> ComponentContexts;

		public:
			void Clear()
			{
				m_SpawnArchetype = nullptr;
				ComponentContexts.clear();
			}

			void Reserve(const uint64_t& capacity)
			{
				ComponentContexts.reserve(capacity);
			}

			inline bool IsValid()const
			{
				return ComponentContexts.size() > 0;
			}
		};

	private:
		ChunkedVector<PrefabSpawnData> m_SpawnDataPerSpawn = { 20 };

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
		/// <summary>
		/// 
		/// </summary>
		/// <returns>True if spawnd data preparation succeded else false.</returns>
		void PreapareSpawnData(
			PrefabSpawnData& spawnData,
			EntityData& prefabEntityData,
			Container* prefabContainer
		);

		void CreateEntityFromSpawnData(
			Entity& entity,
			PrefabSpawnData& spawnData,
			const uint64_t componentsCount,
			const EntityData& prefabEntityData,
			Container* prefabContainer
		);

#pragma endregion

#pragma region COMPONENTS:
	public:
		/// <summary>
		/// Must be invoked before addaing any component of type ComponentType in this container
		/// </summary>
		/// <typeparam name="ComponentType"></typeparam>
		/// <param name="chunkSizeSize">Size of one chunkSize in compnent allocator.</param>
		/// <returns>True if set chunkSize size is successful, else false.</returns>
		template<typename ComponentType>
		bool SetComponentChunkSize(const uint64_t& chunkSize, const decs::ChunkSizeType& chunkSizeType)
		{
			if (chunkSize == 0) return false;
			auto& context = m_ComponentContexts[Type<ComponentType>::ID()];
			if (context != nullptr) return false;

			uint64_t newChunkSize = 0;
			switch (chunkSizeType)
			{
				case decs::ChunkSizeType::ElementsCount:
				{
					newChunkSize = chunkSize;
					break;
				}
				case decs::ChunkSizeType::BytesCount:
				{
					newChunkSize = chunkSize / sizeof(ComponentType);
					if ((chunkSize % sizeof(ComponentType)) > 0)
					{
						newChunkSize += 1;
					}
					break;
				}
				default:
				{
					newChunkSize = m_ComponentContainerChunkSize;
					break;
				}
			}

			if (m_ObserversManager != nullptr)
			{
				context = new ComponentContext<ComponentType>(newChunkSize, m_ObserversManager->GetComponentObserver<ComponentType>());
			}
			else
			{
				context = new ComponentContext<ComponentType>(newChunkSize, nullptr);
			}
			return true;
		}

	private:
		ecsMap<TypeID, ComponentContextBase*> m_ComponentContexts;

	private:
		template<typename ComponentType, typename ...Args>
		ComponentType* AddComponent(const EntityID& e, Args&&... args)
		{
			constexpr TypeID copmonentTypeID = Type<ComponentType>::ID();
			if (!m_CanAddComponents) return nullptr;
			if (e >= m_EntityManager->GetEntitiesDataCount()) return nullptr;
			EntityData& entityData = m_EntityManager->GetEntityData(e);

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

				auto componentContext = GetOrCreateComponentContext<ComponentType>();
				StableComponentAllocator<ComponentType>* container = &componentContext->Allocator;

				auto createResult = container->EmplaceBack(e, std::forward<Args>(args)...);
				if (!createResult.IsValid()) return nullptr;

				Archetype* archetype;
				// making operations on archetypes:
				if (entityData.m_CurrentArchetype == nullptr)
				{
					RemoveFromEmptyEntities(entityData);
					archetype = m_ArchetypesMap.GetSingleComponentArchetype<ComponentType>(componentContext);
					AddEntityToSingleComponentArchetype(
						*archetype,
						entityData,
						createResult.ChunkIndex,
						createResult.ElementIndex,
						createResult.Component
					);
				}
				else
				{
					archetype = m_ArchetypesMap.GetArchetypeAfterAddComponent<ComponentType>(
						*entityData.m_CurrentArchetype,
						componentContext
						);
					AddEntityToArchetype(
						*archetype,
						entityData,
						createResult.ChunkIndex,
						createResult.ElementIndex,
						copmonentTypeID,
						createResult.Component,
						true,
						std::numeric_limits<uint64_t>::max()
					);
				}
				InvokeOnCreateComponentFromEntityID(componentContext, createResult.Component, e);

				return createResult.Component;
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
				const EntityData& data = m_EntityManager->GetConstEntityData(e);
				if (data.IsValidToPerformComponentOperation())
				{
					return GetComponentWithoutCheckingIsAlive<ComponentType>(data);
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
				if (data.IsValidToPerformComponentOperation())
				{
					if (data.m_CurrentArchetype == nullptr) return false;

					uint64_t index = data.m_CurrentArchetype->FindTypeIndex(Type<ComponentType>::ID());
					return index != std::numeric_limits<uint64_t>::max();
				}
			}
			return false;
		}

		template<typename ComponentType>
		ComponentContext<ComponentType>* GetOrCreateComponentContext()
		{
			constexpr TypeID id = Type<ComponentType>::ID();

			auto& componentContext = m_ComponentContexts[id];
			if (componentContext == nullptr)
			{
				uint64_t chunkSize = 0;
				switch (m_ContainerSizeType)
				{
					case decs::ChunkSizeType::ElementsCount:
					{
						chunkSize = m_ComponentContainerChunkSize;
						break;
					}
					case decs::ChunkSizeType::BytesCount:
					{
						chunkSize = m_ComponentContainerChunkSize / sizeof(ComponentType);
						if ((m_ComponentContainerChunkSize % sizeof(ComponentType)) > 0)
						{
							chunkSize += 1;
						}
						break;
					}
					default:
					{
						chunkSize = m_ComponentContainerChunkSize;
						break;
					}
				}
				ComponentContext<ComponentType>* context;
				if (m_ObserversManager != nullptr)
				{
					context = new ComponentContext<ComponentType>(chunkSize, m_ObserversManager->GetComponentObserver<ComponentType>());
				}
				else
				{
					context = new ComponentContext<ComponentType>(chunkSize, nullptr);
				}
				componentContext = context;
				return context;
			}
			else
			{
				ComponentContext<ComponentType>* containedContext = dynamic_cast<ComponentContext<ComponentType>*>(componentContext);

				if (containedContext == nullptr)
				{
					std::string errorMessage = "decs::Container contains component context with id " + std::to_string(id) + " to type other than " + typeid(ComponentType).name();
					throw new std::runtime_error(errorMessage.c_str());
				}

				return containedContext;
			}
		}

		inline void DestroyComponentsContexts()
		{
			for (auto& [key, value] : m_ComponentContexts)
				delete value;
		}

		template<typename ComponentType>
		inline ComponentType* GetComponentWithoutCheckingIsAlive(const EntityData& data) const
		{
			Archetype* archetype = data.m_CurrentArchetype;
			if (archetype == nullptr) return nullptr;

			uint64_t compIndex = archetype->FindTypeIndex<ComponentType>();
			if (compIndex == std::numeric_limits<uint64_t>::max()) return nullptr;

			uint64_t dataIndex = archetype->GetComponentsCount() * data.m_IndexInArchetype + compIndex;
			ComponentRef& componentRef = archetype->m_ComponentsRefs[dataIndex];
			return reinterpret_cast<ComponentType*>(componentRef.GetComponentPointer());
		}

		void ClearComponentsContainers();

		void InvokeOnCreateComponentFromEntityID(ComponentContextBase* componentContext, void* componentPtr, const EntityID& id);

	private:
		template<typename ComponentType, typename ...Args>
		ComponentType* TryAddComponentDelayedToDestroy(
			EntityData& entityData
		)
		{
			constexpr TypeID copmonentTypeID = Type<ComponentType>::ID();
			if (entityData.m_CurrentArchetype != nullptr)
			{
				uint64_t componentIndex = entityData.m_CurrentArchetype->FindTypeIndex(copmonentTypeID);
				if (componentIndex != std::numeric_limits<uint64_t>::max())
				{
					auto& componentRef = entityData.GetComponentRef(componentIndex);
					if (componentRef.m_DelayedToDestroy)
					{
						RemoveComponentFromDelayedToDestroy(entityData.m_ID, copmonentTypeID);
						componentRef.m_DelayedToDestroy = false;
					}
					return reinterpret_cast<ComponentType*>(componentRef.ComponentPointer);
				}
			}

			return nullptr;
		}

		bool RemoveComponentWithoutInvokingListener(const EntityID& e, const TypeID& componentTypeID);
#pragma endregion

#pragma region ARCHETYPES:
	public:
		inline const ArchetypesMap& ArchetypesData() const { return m_ArchetypesMap; }

	private:
		ArchetypesMap m_ArchetypesMap;

	private:
		void AddEntityToArchetype(
			Archetype& newArchetype,
			EntityData& entityData,
			const uint64_t& newCompChunkIndex,
			const uint64_t& newCompElementIndex,
			const TypeID& compTypeID,
			void* newCompPtr,
			const bool& bIsNewComponentAdded,
			const uint64_t& removedComponentIndex
		);

		void AddEntityToSingleComponentArchetype(
			Archetype& newArchetype,
			EntityData& entityData,
			const uint64_t& newCompChunkIndex,
			const uint64_t& newCompElementIndex,
			void* componentPointer
		);

		void RemoveEntityFromArchetype(Archetype& archetype, EntityData& entityData);

		void UpdateEntityComponentAccesDataInArchetype(
			EntityData& data,
			const uint64_t& compChunkIndex,
			const uint64_t& compElementIndex,
			void* compPtr,
			const uint64_t& typeIndex
		);

		template<typename ComponentType>
		void UpdateEntityComponentAccesDataInArchetype(
			const EntityID& entityID,
			const uint64_t& compChunkIndex,
			const uint64_t& compElementIndex,
			void* compPtr
		)
		{
			EntityData& data = m_EntityManager->GetEntityData(entityID);

			uint64_t compDataIndex = data.m_CurrentArchetype->FindTypeIndex(Type<ComponentType>::ID());

			auto& compData = data.m_CurrentArchetype->m_ComponentsRefs[compDataIndex];

			compData.ChunkIndex = compChunkIndex;
			compData.ElementIndex = compElementIndex;
			compData.ComponentPointer = compPtr;
		}

		void AddSpawnedEntityToArchetype(
			PrefabSpawnData& spawnData,
			EntityData& data,
			Archetype* spawnArchetype
		);

		void DestroyEntitesInArchetypes(Archetype& archetype, const bool& invokeOnDestroyListeners = true);

#pragma endregion

#pragma region OBSERVERS
	private:
		ObserversManager* m_ObserversManager = nullptr;
	public:
		bool SetObserversManager(ObserversManager* observersManager);

		void ReassignObservers();

		void InvokeEntitesOnCreateListeners();

		void InvokeEntitesOnDestroyListeners();

	private:
		void InvokeEntitesOnCreateListeners_2();

		void InvokeEntitesOnDestroyListeners_2();

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

		void InvokeComponentOnCreateListeners(ComponentContextBase* coponentContext);

		void InvokeComponentOnDestroyListeners(ComponentContextBase* coponentContext);

#pragma endregion

#pragma region DELAYED DESTROY:
	private:

		ecsSet<DelayedComponentPair> m_DelayedComponentsToDestroy;
		std::vector<EntityID> m_DelayedEntitiesToDestroy;

		bool m_PerformDelayedDestruction = false;

	private:
		void DestroyDelayedEntities();

		void DestroyDelayedComponents();

		void AddEntityToDelayedDestroy(const Entity& entity);

		void AddEntityToDelayedDestroy(const EntityID& entityID);

		void AddComponentToDelayedDestroy(const EntityID& entityID, const TypeID typeID);

		inline void RemoveComponentFromDelayedToDestroy(const EntityID& entityID, const TypeID& typeID)
		{
			m_DelayedComponentsToDestroy.erase({ entityID, typeID });
		}
#pragma endregion
	};
}

