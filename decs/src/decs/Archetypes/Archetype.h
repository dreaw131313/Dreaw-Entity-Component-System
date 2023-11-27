#pragma once
#include "decs/Core.h"
#include "decs/ComponentContext/ComponentContextsManager.h"
#include "decs/Type.h"
#include "decs/ComponentContainers/PackedContainer.h"
#include "decs/ComponentContainers/StableContainer.h"
#include "decs/EntityData.h"


namespace decs
{
	class Entity;
	class Archetype;

	struct ArchetypeEntityData
	{
	public:
		EntityData* m_EntityData = nullptr;
		bool m_bIsActive = false;

	public:
		ArchetypeEntityData()
		{

		}

		ArchetypeEntityData(
			EntityData* entityData
		) :
			m_EntityData(entityData),
			m_bIsActive(entityData->m_bIsActive)
		{

		}

		inline EntityData* GetEntityData()
		{
			return m_EntityData;
		}

		inline bool IsActive() const noexcept
		{
			return m_bIsActive;
		}
	};

	struct ArchetypeTypeData
	{
	public:
		TypeID m_TypeID = std::numeric_limits<TypeID>::max();
		PackedContainerBase* m_PackedContainer = nullptr;
		ComponentContextBase* m_ComponentContext = nullptr;
		StableContainerBase* m_StableContainer = nullptr;

	public:
		ArchetypeTypeData()
		{

		}

		ArchetypeTypeData(
			TypeID typeID,
			PackedContainerBase* packedContainer,
			ComponentContextBase* componentContext,
			StableContainerBase* stableContainer
		) :
			m_TypeID(typeID), m_PackedContainer(packedContainer), m_ComponentContext(componentContext), m_StableContainer(stableContainer)
		{

		}

	};

	class Archetype final
	{
		friend class Container;
		friend class EntityData;
		friend class EntityManager;
		friend class ArchetypesMap;
		template<typename>
		friend class ContainerSerializer;

		template<typename...>
		friend class Query;
		template<typename...>
		friend class MultiQuery;
		template<typename, typename...>
		friend class IterationContainerContext;

		template<typename...>
		friend class BatchIterator;

		template<typename>
		friend class ComponentRef;
		friend class ComponentRefAsVoid;

	private:
		ecsMap<TypeID, uint32_t> m_TypeIDsIndexes;

		ecsMap<TypeID, Archetype*> m_AddEdges;
		ecsMap<TypeID, Archetype*> m_RemoveEdges;

		std::vector<ArchetypeEntityData> m_EntitiesData;
		std::vector<ArchetypeTypeData> m_TypeData;

		struct OrderData
		{
		public:
			ComponentContextBase* m_ComponentContext = nullptr;
			uint32_t m_ComponentIndex = std::numeric_limits<uint32_t>::max();
		};

		std::vector<OrderData> m_ComponentContextsInOrder = {};

		uint32_t m_EntitesCountToInitialize = 0;
		uint32_t m_ComponentsCount = 0; // number of components for each entity
		uint32_t m_EntitiesCount = 0;

	public:
		Archetype();

		~Archetype();

		inline uint32_t ComponentCount() const
		{
			return m_ComponentsCount;
		}

		inline TypeID GetTypeID(uint64_t index) const
		{
			return m_TypeData[index].m_TypeID;
		}

		inline std::string GetComponentTypeName(uint64_t componentIndex) const
		{
			if (componentIndex < m_TypeData.size())
			{
				auto& typeData = m_TypeData[componentIndex];
				if (typeData.m_ComponentContext != nullptr)
				{
					return typeData.m_ComponentContext->GetComponentName();
				}
			}

			return std::string();
		}

		inline uint32_t EntityCount() const
		{
			return m_EntitiesCount;
		}

		inline uint32_t EntitesCountToInvokeCallbacks() const
		{
			return m_EntitesCountToInitialize;
		}

		inline float GetLoadFactor()const
		{
			if (m_EntitiesData.capacity() == 0) return 1.f;
			return (float)m_EntitiesCount / (float)m_EntitiesData.capacity();
		}

		inline bool ContainType(TypeID typeID) const
		{
			return m_TypeIDsIndexes.find(typeID) != m_TypeIDsIndexes.end();
		}

		uint32_t FindTypeIndex(TypeID typeID) const;

		template<typename T>
		inline uint32_t FindTypeIndex() const
		{
			TYPE_ID_CONSTEXPR TypeID typeID = Type<T>::ID();
			if (m_ComponentsCount < Limits::MinComponentsInArchetypeToPerformMapLookup)
			{
				for (uint32_t i = 0; i < m_ComponentsCount; i++)
					if (m_TypeData[i].m_TypeID == typeID) return i;

				return Limits::MaxComponentCount;
			}

			auto it = m_TypeIDsIndexes.find(typeID);
			if (it == m_TypeIDsIndexes.end())
				return Limits::MaxComponentCount;

			return it->second;
		}

	private:
		// instead of using "m_TypeData.emplace_back"
		inline ArchetypeTypeData& AddTypeData(
			TypeID typeID,
			PackedContainerBase* packedContainer,
			ComponentContextBase* componentContext,
			StableContainerBase* stableContainer
		)
		{
			InsertComponentContextInCorrectPlace(componentContext, static_cast<uint32_t>(m_TypeData.size()));
			return m_TypeData.emplace_back(typeID, packedContainer, componentContext, stableContainer);
		}

		void UpdateOrderOfComponentContexts();

		inline void SetEntityActiveState(uint32_t index, bool isActive)
		{
			if (index < m_EntitiesCount)
			{
				m_EntitiesData[index].m_bIsActive = isActive;
			}
		}

		void InsertComponentContextInCorrectPlace(ComponentContextBase* componentContext, uint32_t typeDataIndex)
		{
			// TODO: find better way to insert new elements,
			uint64_t size = m_ComponentContextsInOrder.size();
			for (uint32_t i = 0; i < size; i++)
			{
				if (m_ComponentContextsInOrder[i].m_ComponentContext->GetObserverOrder() >= componentContext->GetObserverOrder())
				{
					auto insertPos = m_ComponentContextsInOrder.begin();
					std::advance(insertPos, i);
					m_ComponentContextsInOrder.insert(insertPos, { componentContext, typeDataIndex });
					return;
				}
			}
			m_ComponentContextsInOrder.push_back({ componentContext, typeDataIndex });
		}

		template<typename ComponentType>
		void AddTypeID(ComponentContextBase* componentContext, StableContainerBase* stableContainer)
		{
			TYPE_ID_CONSTEXPR TypeID id = Type<ComponentType>::ID();
			auto it = m_TypeIDsIndexes.find(id);
			if (it == m_TypeIDsIndexes.end())
			{
				m_ComponentsCount += 1;
				m_TypeIDsIndexes[id] = (uint32_t)m_TypeData.size();
				AddTypeData(
					id,
					new PackedContainer<ComponentType>(),
					componentContext,
					stableContainer
				);
			}
		}

		void AddTypeID(
			const TypeID& id,
			PackedContainerBase* frompackedContainer,
			ComponentContextBase* componentContext,
			StableContainerBase* stableContainer
		);

		void AddEntityData(EntityData* entityData);

		void RemoveSwapBackEntityData(uint64_t index);

		void RemoveSwapBackEntity(uint64_t index);

		template<typename ComponentType>
		inline PackedContainer<ComponentType>* GetContainerAt(uint64_t index)
		{
			return dynamic_cast<PackedContainer<ComponentType>*>(m_TypeData[index].m_PackedContainer);
		}

		inline PackedContainerBase* GetPackedContainerAt(uint64_t index)
		{
			return m_TypeData[index].m_PackedContainer;
		}

		void ReserveSpaceInArchetype(uint64_t desiredCapacity);

		void Reset();

		inline void ValidateEntitiesCountToInitialize()
		{
			m_EntitesCountToInitialize = m_EntitiesCount;
		}

		void InitEmptyFromOther(Archetype& other, ComponentContextsManager* componentContexts, StableContainersManager* stableComponentsManager);

		/// <summary>
		/// Moves entity components from "fromArchetype" to this archetype.
		/// </summary>
		/// <param name="componentTypeID"></param>
		/// <param name="fromArchetype"></param>
		/// <param name="fromIndex"></param>
		void MoveEntityComponentsAfterRemoveComponent(
			TypeID removedComponentTypeID,
			Archetype* fromArchetype,
			uint64_t fromIndex,
			EntityData* entityData
		);

		/// <summary>
		/// Moves entity components from "fromArchetype" to this archetype.
		/// </summary>
		/// <typeparam name="ComponentType"></typeparam>
		/// <param name="fromArchetype"></param>
		/// <param name="fromIndex"></param>
		template<typename ComponentType>
		void MoveEntityComponentsAfterAddComponent(Archetype* fromArchetype, uint64_t fromIndex, EntityData* entityData)
		{
			TYPE_ID_CONSTEXPR TypeID newComponentTypeID = Type<ComponentType>::ID();

			this->AddEntityData(entityData);

			uint64_t thisArchetypeIndex = 0;
			uint64_t fromArchetypeIndex = 0;

			for (; thisArchetypeIndex < m_ComponentsCount; thisArchetypeIndex++)
			{
				ArchetypeTypeData& thisTypeData = m_TypeData[thisArchetypeIndex];
				if (thisTypeData.m_TypeID == newComponentTypeID)
				{
					continue;
				}


				ArchetypeTypeData& fromArchetypeData = fromArchetype->m_TypeData[fromArchetypeIndex];
				thisTypeData.m_PackedContainer->MoveEmplaceBackFromVoid(
					fromArchetypeData.m_PackedContainer->GetComponentDataAsVoid(fromIndex)
				);
				fromArchetypeData.m_PackedContainer->RemoveSwapBack(fromIndex);

				fromArchetypeIndex++;
			}

			fromArchetype->RemoveSwapBackEntityData(fromIndex);
		}

		void ShrinkToFit();

	};
}