#pragma once
#include "decspch.h"

#include "Core.h"
#include "Type.h"
#include "EntityData.h"
#include "ComponentContext.h"
#include "Archetype.h"
#include "ComponentContainer.h"
#include "Observers.h"

namespace decs
{
	class Entity;

	enum class ContainerBucketSizeType
	{
		ElementsCount,
		BytesCount
	};

	class Container final
	{
		template<typename ...Types>
		friend class View;
		template<typename ...Types>
		friend class ViewBaseWithEntity;
		template<typename ...Types>
		friend class ViewBase;
		template<typename ...Types>
		friend class ComplexView;

	public:
		Container();

		Container(
			const uint64_t& initialEntitiesCapacity,
			const ContainerBucketSizeType& componentContainerBucketSizeType,
			const uint64_t& componentContainerBucketSize
		);

		~Container();

	private:
		uint64_t m_ComponentContainerBucketSize = decs::MemorySize::KiloByte * 16;
		ContainerBucketSizeType m_ContainerSizeType = ContainerBucketSizeType::BytesCount;

	public:
		inline void SetDefaultComponentContainerBucketSize(const uint64_t& size, const ContainerBucketSizeType& sizeType)
		{
			m_ContainerSizeType = sizeType;
			m_ComponentContainerBucketSize = size;
		}

		// Entities:
	private:
		std::vector<EntityData> m_EntityData;
		EntityID m_CreatedEntitiesCount = 0;
		uint64_t m_enitiesDataCount = 0;

		std::vector<uint64_t> m_FreeEntities;
		uint64_t m_FreeEntitiesCount = 0;

	public:
		Entity CreateEntity(bool isActive = true);

		bool DestroyEntity(const EntityID& entity);

		inline bool IsEntityAlive(const EntityID& entity) const
		{
			if (entity >= m_enitiesDataCount) return false;
			return m_EntityData[entity].IsAlive;
		}

		inline uint32_t GetEntityVersion(const EntityID& entity) const
		{
			if (IsEntityAlive(entity))
				return m_EntityData[entity].Version;

			return 0;
		}

		inline void SetEntityActive(const EntityID& entity, const bool& isActive)
		{
			auto& data = m_EntityData[entity];
			if (data.IsActive != isActive)
			{
				data.IsActive = isActive;
				auto& archEntityData = data.CurrentArchetype->m_EntitiesData[data.IndexInArchetype];
				archEntityData.IsActive = isActive;
			}
		}

		inline bool IsEntityActive(const EntityID& entity)
		{
			return m_EntityData[entity].IsActive;
		}

		// Entities - end

		// Components containers:
	public:

		template<typename ComponentType, typename ...Args>
		ComponentType* CreateComponent(const EntityID& e, Args&&... args)
		{
			if (IsEntityAlive(e))
			{
				EntityData& entityData = m_EntityData[e];

				constexpr auto copmonentTypeID = Type<ComponentType>::ID();

				{
					auto currentComponent = GetComponent<ComponentType>(e);
					if (currentComponent != nullptr) return currentComponent;
				}

				auto componentContext = GetOrCreateComponentContext<ComponentType>();

				StableComponentAllocator<ComponentType>* container = &componentContext->Allocator;

				auto createResult = container->EmplaceBack(e, std::forward<Args>(args)...);

				Archetype* archetype;
				// making operations on archetypes:
				if (entityData.CurrentArchetype == nullptr)
				{
					archetype = m_ArchetypesMap.GetSingleComponentArchetype<ComponentType>(componentContext);
					AddEntityToSingleComponentArchetype(
						*archetype,
						entityData,
						createResult.BucketIndex,
						createResult.ElementIndex,
						createResult.Component
					);
				}
				else
				{
					archetype = m_ArchetypesMap.GetArchetypeAfterAddComponent<ComponentType>(
						*entityData.CurrentArchetype,
						componentContext
						);
					AddEntityToArchetype(
						*archetype,
						entityData,
						createResult.BucketIndex,
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
		ComponentType* GetComponent(const EntityID& e)
		{
			if (IsEntityAlive(e))
			{
				EntityData& data = m_EntityData[e];

				if (data.CurrentArchetype == nullptr) return nullptr;

				auto it = data.CurrentArchetype->m_TypeIDsIndexes.find(Type<ComponentType>::ID());
				if (it != data.CurrentArchetype->m_TypeIDsIndexes.end())
				{
					ComponentContext<ComponentType>* componentContext = GetOrCreateComponentContext<ComponentType>();

					auto compAccesData = GetEntityComponentBucketElementIndex(data, it->second);
					return componentContext->Allocator.GetComponent(compAccesData.BucketIndex, compAccesData.ElementIndex);
				}
			}

			return nullptr;
		}

		template<typename ComponentType>
		bool HasComponent(const EntityID& e)
		{
			if (IsEntityAlive(e))
			{
				EntityData& data = m_EntityData[e];

				if (data.CurrentArchetype == nullptr) return false;

				auto it = data.CurrentArchetype->m_TypeIDsIndexes.find(Type<ComponentType>::ID());
				return it != data.CurrentArchetype->m_TypeIDsIndexes.end();
			}

			return false;
		}

	private:
		ecsMap<TypeID, ComponentContextBase*> m_ComponentContexts;

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
					case decs::ContainerBucketSizeType::ElementsCount:
					{
						bucektSize = m_ComponentContainerBucketSize;
						break;
					}
					case decs::ContainerBucketSizeType::BytesCount:
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
			uint64_t dataIndex = data.CurrentArchetype->GetComponentsCount() * data.IndexInArchetype + typeIndex;
			return data.CurrentArchetype->m_ComponentsRefs[dataIndex];
		}

		// Components - end

		// Archetypes:
	private:
		ArchetypesMap m_ArchetypesMap;

	private:
		void AddEntityToArchetype(
			Archetype& newArchetype,
			EntityData& entityData,
			const uint64_t& newCompBucketIndex,
			const uint64_t& newCompElementIndex,
			const TypeID& compTypeID,
			void* newCompPtr = nullptr,
			const bool& bIsNewComponentAdded = true
		);

		void AddEntityToSingleComponentArchetype(
			Archetype& newArchetype,
			EntityData& entityData,
			const uint64_t& newCompBucketIndex,
			const uint64_t& newCompElementIndex,
			void* componentPointer
		);

		void RemoveEntityFromArchetype(Archetype& archetype, EntityData& entityData);

		void UpdateEntityComponentAccesDataInArchetype(
			const EntityID& entityID,
			const uint64_t& compBucketIndex,
			const uint64_t& compElementIndex,
			void* compPtr,
			const uint64_t& typeIndex
		);

		template<typename ComponentType>
		void UpdateEntityComponentAccesDataInArchetype(
			const EntityID& entityID,
			const uint64_t& compBucketIndex,
			const uint64_t& compElementIndex,
			void* compPtr
		)
		{
			EntityData& data = m_EntityData[entityID];

			uint64_t compDataIndex = data.CurrentArchetype->m_TypeIDsIndexes[Type<ComponentType>::ID()];

			auto& compData = data.CurrentArchetype->m_ComponentsRefs[compDataIndex];

			compData.BucketIndex = compBucketIndex;
			compData.ElementIndex = compElementIndex;
			compData.ComponentPointer = compPtr;
		}

		// Archetypes - end

		// Observers:
	private:
		std::vector<CreateEntityObserver*> m_EntityCreationObservers;
		std::vector<DestroyEntityObserver*> m_EntittyDestructionObservers;

	public:
		bool AddEntityCreationObserver(CreateEntityObserver* observer);
		
		bool RemoveEntityCreationObserver(CreateEntityObserver* observer);
		
		bool AddEntityDestructionObserver(DestroyEntityObserver* observer);
		
		bool RemoveEntityDestructionObserver(DestroyEntityObserver* observer);
		
	private:
		inline void InvokeCreateEntityObservers(const EntityID& entity)
		{
			for (auto observer : m_EntityCreationObservers)
				observer->OnCreateEntity(entity, *this);
		}

		inline void InvokeDestroyEntityObservers(const EntityID& entity)
		{
			for (auto observer : m_EntittyDestructionObservers)
				observer->OnDestroyEntity(entity, *this);
		}

		// Observers - end


	};
}

