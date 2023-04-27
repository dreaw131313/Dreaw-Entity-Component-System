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
		EntityID m_ID = std::numeric_limits<EntityID>::max();
		bool m_bIsActive = false;

	public:
		ArchetypeEntityData()
		{

		}

		ArchetypeEntityData(
			EntityID id,
			bool isActive
		) :
			m_ID(id),
			m_bIsActive(isActive)
		{

		}

		inline EntityID ID() const noexcept { return m_ID; }
		inline bool IsActive() const noexcept { return m_bIsActive; }
	};

	struct EntityRemoveSwapBackResult
	{
	public:
		EntityID ID = std::numeric_limits<EntityID>::max();
		uint32_t Index = std::numeric_limits<uint32_t>::max();

		EntityRemoveSwapBackResult()
		{

		}

		EntityRemoveSwapBackResult(EntityID id, uint32_t index) : ID(id), Index(index)
		{

		}

		inline bool IsValid() const noexcept
		{
			return ID != std::numeric_limits<EntityID>::max() && Index != std::numeric_limits<uint32_t>::max();
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

		template<typename...>
		friend 	class Query;
		template<typename...>
		friend 	class MultiQuery;
		template<typename, typename...>
		friend class IterationContainerContext;

		template<typename... ComponentTypes>
		friend class BatchIterator;

		template<typename ComponentType>
		friend class ComponentRef;
		friend class ComponentRefAsVoid;

	private:
		std::vector<ArchetypeEntityData> m_EntitiesData;
		std::vector<ArchetypeTypeData> m_TypeData;
		ecsMap<TypeID, uint32_t> m_TypeIDsIndexes;
		std::vector<TypeID> m_AddingOrderTypeIDs;

		ecsMap<TypeID, Archetype*> m_AddEdges;
		ecsMap<TypeID, Archetype*> m_RemoveEdges;

		uint32_t m_EntitesCountToInitialize = 0;
		uint32_t m_ComponentsCount = 0; // number of components for each entity
		uint32_t m_EntitiesCount = 0;

	public:
		Archetype()
		{
		}

		~Archetype()
		{
			for (auto& data : m_TypeData)
			{
				delete data.m_PackedContainer;
			}
		}

		inline uint32_t ComponentsCount() const { return m_ComponentsCount; }
		inline TypeID GetTypeID(uint64_t index) const { return m_TypeData[index].m_TypeID; }
		inline uint32_t EntitiesCount() const { return m_EntitiesCount; }
		inline uint32_t EntitesCountToInvokeCallbacks() const { return m_EntitesCountToInitialize; }

		inline float GetLoadFactor()const
		{
			if (m_EntitiesData.capacity() == 0) return 1.f;
			return (float)m_EntitiesCount / (float)m_EntitiesData.capacity();
		}

		inline bool ContainType(TypeID typeID) const
		{
			return m_TypeIDsIndexes.find(typeID) != m_TypeIDsIndexes.end();
		}

		inline uint32_t FindTypeIndex(TypeID typeID) const
		{
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
		inline void SetEntityActiveState(uint32_t index, bool isActive)
		{
			if (index < m_EntitiesCount)
			{
				m_EntitiesData[index].m_bIsActive = isActive;
			}
		}

		template<typename ComponentType>
		inline void AddTypeToAddingComponentOrder()
		{
			m_AddingOrderTypeIDs.push_back(Type<ComponentType>::ID());
		}

		inline void AddTypeToAddingComponentOrder(TypeID id)
		{
			m_AddingOrderTypeIDs.push_back(id);
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
				m_TypeData.emplace_back(
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
		)
		{
			auto it = m_TypeIDsIndexes.find(id);
			if (it == m_TypeIDsIndexes.end())
			{
				m_ComponentsCount += 1;
				m_TypeIDsIndexes[id] = (uint32_t)m_TypeData.size();
				m_TypeData.emplace_back(
					id,
					frompackedContainer->CreateOwnEmptyCopy(),
					componentContext,
					stableContainer
				);
			}
		}

		void AddEntityData(EntityID id, const bool& isActive)
		{
			m_EntitiesData.emplace_back(id, isActive);
			m_EntitiesCount += 1;
		}

		EntityRemoveSwapBackResult RemoveSwapBackEntity(uint64_t index)
		{
			if (m_EntitiesCount == 0) return EntityRemoveSwapBackResult();

			if (index == m_EntitiesCount - 1)
			{
				m_EntitiesData.pop_back();

				for (uint64_t i = 0; i < m_ComponentsCount; i++)
				{
					auto& typeData = m_TypeData[i];
					if (typeData.m_StableContainer != nullptr)
					{
						StableComponentRef* compRef = static_cast<StableComponentRef*>(typeData.m_PackedContainer->GetComponentDataAsVoid(index));
						typeData.m_StableContainer->Remove(compRef->m_ChunkIndex, compRef->m_Index);
					}

					typeData.m_PackedContainer->PopBack();
				}

				m_EntitiesCount -= 1;
				return EntityRemoveSwapBackResult();
			}
			else
			{
				m_EntitiesData[index] = m_EntitiesData.back();
				m_EntitiesData.pop_back();

				for (uint64_t i = 0; i < m_ComponentsCount; i++)
				{
					auto& typeData = m_TypeData[i];
					if (typeData.m_StableContainer != nullptr)
					{
						StableComponentRef* compRef = static_cast<StableComponentRef*>(typeData.m_PackedContainer->GetComponentDataAsVoid(index));
						typeData.m_StableContainer->Remove(compRef->m_ChunkIndex, compRef->m_Index);
					}

					typeData.m_PackedContainer->RemoveSwapBack(index);
				}

				m_EntitiesCount -= 1;
				return EntityRemoveSwapBackResult(m_EntitiesData[index].m_ID, (uint32_t)index);
			}
		}

		template<typename ComponentType>
		inline PackedContainer<ComponentType>* GetContainerAt(uint64_t index)
		{
			return dynamic_cast<PackedContainer<ComponentType>*>(m_TypeData[index].m_PackedContainer);
		}

		void ShrinkToFit()
		{
			m_EntitiesData.shrink_to_fit();
			for (uint64_t idx = 0; idx < m_ComponentsCount; idx++)
			{
				m_TypeData[idx].m_PackedContainer->ShrinkToFit();
			}
		}

		void ReserveSpaceInArchetype(uint64_t desiredCapacity)
		{
			if (m_EntitiesData.capacity() < desiredCapacity)
			{
				m_EntitiesData.reserve(desiredCapacity);

				for (uint64_t idx = 0; idx < m_ComponentsCount; idx++)
				{
					m_TypeData[idx].m_PackedContainer->Reserve(desiredCapacity);
				}
			}
		}

		void Reset()
		{
			m_EntitiesCount = 0;
			m_EntitiesData.clear();
			for (uint64_t idx = 0; idx < m_ComponentsCount; idx++)
			{
				m_TypeData[idx].m_PackedContainer->Clear();
			}
		}

		inline void ValidateEntitiesCountToInitialize()
		{
			m_EntitesCountToInitialize = m_EntitiesCount;
		}

		void InitEmptyFromOther(Archetype& other, ComponentContextsManager* componentContexts, StableContainersManager* stableComponentsManager)
		{
			m_ComponentsCount = other.m_ComponentsCount;
			m_TypeData.reserve(m_ComponentsCount);
			m_AddingOrderTypeIDs.reserve(m_ComponentsCount);

			for (uint32_t i = 0; i < m_ComponentsCount; i++)
			{
				ArchetypeTypeData& otherTypeData = other.m_TypeData[i];
				otherTypeData.m_TypeID;
				m_TypeIDsIndexes[otherTypeData.m_TypeID] = i;

				m_TypeData.emplace_back(
					otherTypeData.m_TypeID,
					otherTypeData.m_PackedContainer->CreateOwnEmptyCopy(),
					componentContexts->GetComponentContext(otherTypeData.m_TypeID),
					stableComponentsManager->GetStableContainer(otherTypeData.m_TypeID)
				);

				m_AddingOrderTypeIDs.push_back(other.m_AddingOrderTypeIDs[i]);
			}
		}

		/// <summary>
		/// Moves entity components from "fromArchetype" to this archetype.
		/// </summary>
		/// <param name="componentTypeID"></param>
		/// <param name="fromArchetype"></param>
		/// <param name="fromIndex"></param>
		void MoveEntityAfterRemoveComponent(TypeID removedComponentTypeID, Archetype& fromArchetype, uint64_t fromIndex)
		{
			uint64_t thisArchetypeIndex = 0;
			uint64_t fromArchetypeIndex = 0;

			for (; thisArchetypeIndex < m_ComponentsCount; thisArchetypeIndex++, fromArchetypeIndex++)
			{
				ArchetypeTypeData& thisTypeData = m_TypeData[thisArchetypeIndex];
				if (fromArchetype.m_TypeData[fromArchetypeIndex].m_TypeID == removedComponentTypeID)
				{
					fromArchetypeIndex += 1;
				}

				thisTypeData.m_PackedContainer->MoveEmplaceFromVoid(
					fromArchetype.m_TypeData[fromArchetypeIndex].m_PackedContainer->GetComponentDataAsVoid(fromIndex)
				);
			}
		}

		/// <summary>
		/// Moves entity components from "fromArchetype" to this archetype.
		/// </summary>
		/// <typeparam name="ComponentType"></typeparam>
		/// <param name="fromArchetype"></param>
		/// <param name="fromIndex"></param>
		template<typename ComponentType>
		void MoveEntityAfterAddComponent(Archetype& fromArchetype, uint64_t fromIndex)
		{
			TYPE_ID_CONSTEXPR TypeID newComponentTypeID = Type<ComponentType>::ID();

			uint64_t thisArchetypeIndex = 0;
			uint64_t fromArchetypeIndex = 0;

			for (; thisArchetypeIndex < m_ComponentsCount; thisArchetypeIndex++)
			{
				ArchetypeTypeData& thisTypeData = m_TypeData[thisArchetypeIndex];
				if (thisTypeData.m_TypeID == newComponentTypeID)
				{
					continue;
				}

				thisTypeData.m_PackedContainer->MoveEmplaceFromVoid(
					fromArchetype.m_TypeData[fromArchetypeIndex].m_PackedContainer->GetComponentDataAsVoid(fromIndex)
				);

				fromArchetypeIndex++;
			}
		}

	};
}