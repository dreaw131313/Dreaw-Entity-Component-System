#pragma once
#include "decspch.h"

#include "Core.h"
#include "Type.h"
#include "EntityManager.h"
#include "ComponentContext.h"
#include "ComponentContainer.h"
#include "Observers.h"

namespace decs
{
	class Entity;

	enum class BucketSizeType
	{
		ElementsCount,
		BytesCount
	};

	class ContainerCreateInfo
	{
	public:
		EntityManager* entityManager = nullptr;
		BucketSizeType ComponentBucektSizeType = BucketSizeType::BytesCount;
		uint64_t DefaultComponentBucketSize = 16 * MemorySize::KiloByte;
		bool InvokeEntityActiveationStateListeners = true;
	};

	class Container
	{
		template<typename ...Types>
		friend class View;

	public:
		Container();

		Container(
			EntityManager* entityManager,
			const BucketSizeType& componentContainerBucketSizeType,
			const uint64_t& componentContainerBucketSize,
			const bool invokeEntityActivationStateListeners
		);

		Container(const ContainerCreateInfo& createInfo);

		~Container();

	private:
		uint64_t m_ComponentContainerBucketSize = decs::MemorySize::KiloByte * 16;
		BucketSizeType m_ContainerSizeType = BucketSizeType::BytesCount;

	public:
		inline void SetDefaultComponentBucketSize(const uint64_t& size, const BucketSizeType& sizeType)
		{
			m_ContainerSizeType = sizeType;
			m_ComponentContainerBucketSize = size;
		}

#pragma region ENTITIES:
	protected:
		EntityManager* m_EntityManager;

	private:
		bool m_bInvokeEntityActivationStateListeners = true;

	public:
		inline uint64_t GetAliveEntitesCount() const
		{
			return m_EntityManager->GetCreatedEntitiesCount();
		}

		Entity& CreateEntity(const bool& isActive = true);

		bool DestroyEntity(const EntityID& entityID);

		bool DestroyEntity(Entity& entity);

		void DestroyOwnedEntities();
		 
		// Entity utiliti methods:

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

	private:
		void DestroyEntitesInArchetypes(Archetype& archetype);

#pragma endregion

#pragma region SPAWNING ENTIES
	private:
		struct PrefabSpawnComponentContext
		{
		public:
			TypeID ComponentType = std::numeric_limits<TypeID>::max();
			ComponentContextBase* PrefabComponentContext = nullptr;
			ComponentContextBase* ComponentContext = nullptr;
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
			std::vector<PrefabSpawnComponentContext> ComponentContexts;

		public:
			void Clear()
			{
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
		PrefabSpawnData m_SpawnData = {};

	public:
		Entity* Spawn(const Entity& prefab, const bool& isActive = true);

		bool Spawn(const Entity& prefab, std::vector<Entity*>& spawnedEntities, const uint64_t& spawnCount, const bool& areActive = true);

		bool Spawn(const Entity& prefab, const uint64_t& spawnCount, const bool& areActive = true);

	private:
		/// <summary>
		/// 
		/// </summary>
		/// <returns>True if spawnd data preparation succeded else false.</returns>
		void PreapareSpawnData(const uint64_t& componentsCount, Archetype* prefabArchetype);

#pragma endregion

#pragma region COMPONENTS:
	public:
		/// <summary>
		/// Must be invoked before addaing any component of type ComponentType in this container
		/// </summary>
		/// <typeparam name="ComponentType"></typeparam>
		/// <param name="bucketSize">Size of one bucket in compnent allocator.</param>
		/// <returns>True if set bucket size is successful, else false.</returns>
		template<typename ComponentType>
		bool SetComponentContainerBucketSize(const uint64_t& bucketSize)
		{
			if (bucketSize == 0) return false;
			auto& context = m_ComponentContexts[Type<ComponentType>::ID()];
			if (context != nullptr) return false;

			context = new ComponentContext<ComponentType>(bucketSize);
			return true;
		}

		template<typename ComponentType, typename ...Args>
		ComponentType* AddComponent(const EntityID& e, Args&&... args)
		{
			if (IsEntityAlive(e))
			{
				EntityData& entityData = m_EntityManager->GetEntityData(e);
				constexpr auto copmonentTypeID = Type<ComponentType>::ID();

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
						true
					);
				}

				componentContext->InvokeOnCreateComponent(*createResult.Component, e, *this);

				return createResult.Component;
			}
			return nullptr;
		}

		template<typename ComponentType>
		inline bool RemoveComponent(const EntityID& e)
		{
			return RemoveComponent(e, Type<ComponentType>::ID());
		}

		bool RemoveComponent(const EntityID& e, const TypeID& componentTypeID);

		template<typename ComponentType>
		ComponentType* GetComponent(const EntityID& e) const
		{
			if (IsEntityAlive(e))
			{
				const EntityData& data = m_EntityManager->GetConstEntityData(e);
				return GetComponentWithoutCheckingIsAlive<ComponentType>(data);
			}
			return nullptr;
		}

		template<typename ComponentType>
		bool HasComponent(const EntityID& e) const
		{
			if (IsEntityAlive(e))
			{
				const EntityData& data = m_EntityManager->GetConstEntityData(e);

				if (data.m_CurrentArchetype == nullptr) return false;

				auto it = data.m_CurrentArchetype->m_TypeIDsIndexes.find(Type<ComponentType>::ID());
				return it != data.m_CurrentArchetype->m_TypeIDsIndexes.end();
			}

			return false;
		}

	private:
		ecsMap<TypeID, ComponentContextBase*> m_ComponentContexts;

	private:
		template<typename ComponentType>
		ComponentContext<ComponentType>* GetOrCreateComponentContext()
		{
			constexpr TypeID id = Type<ComponentType>::ID();

			auto& componentContext = m_ComponentContexts[id];
			if (componentContext == nullptr)
			{
				uint64_t bucektSize = 0;
				switch (m_ContainerSizeType)
				{
					case decs::BucketSizeType::ElementsCount:
					{
						bucektSize = m_ComponentContainerBucketSize;
						break;
					}
					case decs::BucketSizeType::BytesCount:
					{
						bucektSize = m_ComponentContainerBucketSize / sizeof(ComponentType);
						if ((m_ComponentContainerBucketSize % sizeof(ComponentType)) > 0)
						{
							bucektSize += 1;
						}
						break;
					}
					default:
					{
						bucektSize = m_ComponentContainerBucketSize;
						break;
					}
				}

				ComponentContext<ComponentType>* context = new ComponentContext<ComponentType>(bucektSize);
				componentContext = context;
				return context;
			}
			else
			{
				return static_cast<ComponentContext<ComponentType>*>(componentContext);
			}
		}

		inline void DestroyComponentsContexts()
		{
			for (auto [key, value] : m_ComponentContexts)
				delete value;
		}

		inline ComponentRef& GetEntityComponentBucketElementIndex(const EntityData& data, const uint64_t& typeIndex)
		{
			uint64_t dataIndex = data.m_CurrentArchetype->GetComponentsCount() * data.m_IndexInArchetype + typeIndex;
			return data.m_CurrentArchetype->m_ComponentsRefs[dataIndex];
		}

		template<typename ComponentType>
		ComponentType* GetComponentWithoutCheckingIsAlive(const EntityData& data) const
		{
			Archetype* archetype = data.m_CurrentArchetype;
			if (archetype == nullptr) return nullptr;

			auto it = archetype->m_TypeIDsIndexes.find(Type<ComponentType>::ID());
			if (it == archetype->m_TypeIDsIndexes.end()) return nullptr;

			uint64_t dataIndex = archetype->GetComponentsCount() * data.m_IndexInArchetype + it->second;
			return reinterpret_cast<ComponentType*>(archetype->m_ComponentsRefs[dataIndex].ComponentPointer);
		}

		void ClearComponentsContainers();
#pragma endregion

#pragma region ARCHETYPES:
	private:
		ArchetypesMap m_ArchetypesMap;

	private:
		void AddEntityToArchetype(
			Archetype& newArchetype,
			EntityData& entityData,
			const uint64_t& newCompChunkIndex,
			const uint64_t& newCompElementIndex,
			const TypeID& compTypeID,
			void* newCompPtr = nullptr,
			const bool& bIsNewComponentAdded = true
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
			const EntityID& entityID,
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

			uint64_t compDataIndex = data.m_CurrentArchetype->m_TypeIDsIndexes[Type<ComponentType>::ID()];

			auto& compData = data.m_CurrentArchetype->m_ComponentsRefs[compDataIndex];

			compData.ChunkIndex = compChunkIndex;
			compData.ElementIndex = compElementIndex;
			compData.ComponentPointer = compPtr;
		}

		void AddSpawnedEntityToArchetype(
			const EntityID& entityID,
			Archetype* toArchetype,
			Archetype* prefabArchetype
		);

		Entity* CreateEntityFromSpawnData(
			const bool& isActive,
			const uint64_t componentsCount,
			const EntityData& prefabEntityData,
			Archetype* prefabArchetype,
			Container* prefabContainer
		);

#pragma endregion

#pragma region OBSERVERS:
	private:
		std::vector<CreateEntityObserver*> m_EntityCreationObservers;
		std::vector<DestroyEntityObserver*> m_EntittyDestructionObservers;

		std::vector<ActivateEntityObserver*> m_EntittyActivateObservers;
		std::vector<DeactivateEntityObserver*> m_EntittyDeactivateObservers;

	public:
		bool AddEntityCreationObserver(CreateEntityObserver* observer);

		bool RemoveEntityCreationObserver(CreateEntityObserver* observer);

		bool AddEntityDestructionObserver(DestroyEntityObserver* observer);

		bool RemoveEntityDestructionObserver(DestroyEntityObserver* observer);

	private:
		inline void InvokeEntityCreationObservers(const EntityID& entity)
		{
			for (auto observer : m_EntityCreationObservers)
				observer->OnCreateEntity(entity, *this);
		}

		inline void InvokeEntityDestructionObservers(const EntityID& entity)
		{
			for (auto observer : m_EntittyDestructionObservers)
				observer->OnDestroyEntity(entity, *this);
		}

		inline void InvokeEntityActivationObservers(Entity& entity)
		{
			for (auto observer : m_EntittyActivateObservers)
				observer->OnSetEntityActive(entity);
		}

		inline void InvokeEntityDeactivationObservers(Entity& entity)
		{
			for (auto observer : m_EntittyDeactivateObservers)
				observer->OnSetEntityInactive(entity);
		}
#pragma endregion

	};

	class FullContainer : public Container
	{
	public:
		FullContainer();

		FullContainer(
			const uint64_t& intialEntitiesCapacity,
			const BucketSizeType& componentContainerBucketSizeType,
			const uint64_t& componentContainerBucketSize,
			const bool invokeEntityActivationStateListeners
		);

	private:
		EntityManager m_OwnEntityManager = { 1000 };
	};
}