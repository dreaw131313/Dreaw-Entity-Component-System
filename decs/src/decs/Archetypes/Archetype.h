#pragma once
#include "decs\Core.h"
#include "decs\ComponentContextsManager.h"
#include "decs\Type.h"
#include "decs\ComponentContainers\PackedContainer.h"

namespace decs
{
	class Entity;
	class Archetype;

	struct ArchetypeEntityData
	{
	public:
		EntityID m_ID = std::numeric_limits<EntityID>::max();
		bool m_IsActive = false;

	public:
		ArchetypeEntityData()
		{

		}

		ArchetypeEntityData(
			const EntityID& id,
			const bool& isActive
		) :
			m_ID(id),
			m_IsActive(isActive)
		{

		}

		inline EntityID ID() const noexcept { return m_ID; }
		inline bool IsActive() const noexcept { return m_IsActive; }
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

	public:
		ArchetypeTypeData()
		{

		}

		ArchetypeTypeData(TypeID typeID, PackedContainerBase* packedContainer, ComponentContextBase* componentContext) :
			m_TypeID(typeID), m_PackedContainer(packedContainer), m_ComponentContext(componentContext)
		{

		}

	};

	class Archetype final
	{
		friend class Container;
		friend class EntitytData;
		friend class EntityManager;
		friend class ArchetypesMap;

		template<typename... ComponentTypes>
		friend 	class View;

		template<typename... ComponentTypes>
		friend class BatchIterator;

		template<typename ComponentType>
		friend class ComponentRef;
		friend class ComponentRefAsVoid;

	public:
		static constexpr uint64_t m_MinComponentsInArchetypeToPerformMapLookup = 20;

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
		inline TypeID GetTypeID(const uint64_t& index) const { return m_TypeData[index].m_TypeID; }

		inline uint32_t EntitiesCount() const { return m_EntitiesCount; }
		inline uint32_t EntitesCountToInvokeCallbacks() const { return m_EntitesCountToInitialize; }

		inline float GetLoadFactor()const
		{
			if (m_EntitiesData.capacity() == 0) return 1.f;
			return (float)m_EntitiesCount / (float)m_EntitiesData.capacity();
		}

		inline bool ContainType(const TypeID& typeID) const
		{
			return m_TypeIDsIndexes.find(typeID) != m_TypeIDsIndexes.end();
		}

		inline uint32_t FindTypeIndex(const TypeID& typeID) const
		{
			if (m_ComponentsCount < m_MinComponentsInArchetypeToPerformMapLookup)
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
			constexpr TypeID typeID = Type<T>::ID();
			if (m_ComponentsCount < m_MinComponentsInArchetypeToPerformMapLookup)
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
		template<typename ComponentType>
		inline void AddTypeToAddingComponentOrder()
		{
			m_AddingOrderTypeIDs.push_back(Type<ComponentType>::ID());
		}

		inline void AddTypeToAddingComponentOrder(const TypeID& id)
		{
			m_AddingOrderTypeIDs.push_back(id);
		}

		template<typename ComponentType>
		void AddTypeID(ComponentContextBase* componentContext)
		{
			constexpr TypeID id = Type<ComponentType>::ID();
			auto it = m_TypeIDsIndexes.find(id);
			if (it == m_TypeIDsIndexes.end())
			{
				m_ComponentsCount += 1;
				m_TypeIDsIndexes[id] = (uint32_t)m_TypeData.size();
				m_TypeData.emplace_back(
					id,
					new PackedContainer<ComponentType>(),
					componentContext
				);
			}
		}

		void AddTypeID(const TypeID& id, PackedContainerBase* frompackedContainer, ComponentContextBase* componentContext)
		{
			auto it = m_TypeIDsIndexes.find(id);
			if (it == m_TypeIDsIndexes.end())
			{
				m_ComponentsCount += 1;
				m_TypeIDsIndexes[id] = (uint32_t)m_TypeData.size();
				m_TypeData.emplace_back(
					id,
					frompackedContainer->CreateOwnEmptyCopy(),
					componentContext
				);
			}
		}

		void AddEntityData(const EntityID& id, const bool& isActive)
		{
			m_EntitiesData.emplace_back(id, isActive);
			m_EntitiesCount += 1;
		}

		EntityRemoveSwapBackResult RemoveSwapBackEntity(const uint64_t index)
		{
			if (m_EntitiesCount == 0) return EntityRemoveSwapBackResult();

			if (index == m_EntitiesCount - 1)
			{
				m_EntitiesData.pop_back();

				for (uint64_t i = 0; i < m_ComponentsCount; i++)
				{
					m_TypeData[i].m_PackedContainer->PopBack();
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
					m_TypeData[i].m_PackedContainer->RemoveSwapBack(index);
				}

				m_EntitiesCount -= 1;
				return EntityRemoveSwapBackResult(m_EntitiesData[index].m_ID, (uint32_t)index);
			}
		}

		void MoveEntityAfterRemoveComponent(const TypeID& componentTypeID, Archetype& fromArchetype, const uint64_t& fromIndex)
		{
			uint64_t thisArchetypeIndex = 0;
			uint64_t fromArchetypeIndex = 0;

			for (; thisArchetypeIndex < m_ComponentsCount; thisArchetypeIndex++, fromArchetypeIndex++)
			{
				if (fromArchetype.m_TypeData[thisArchetypeIndex].m_TypeID == componentTypeID)
				{
					fromArchetypeIndex += 1;
				}
				m_TypeData[thisArchetypeIndex].m_PackedContainer->EmplaceFromVoid(
					fromArchetype.m_TypeData[fromArchetypeIndex].m_PackedContainer->GetComponentDataAsVoid(fromIndex)
				);
			}
		}

		template<typename ComponentType>
		void MoveEntityAfterAddComponent(Archetype& fromArchetype, const uint64_t& fromIndex)
		{
			constexpr TypeID newComponentTypeID = Type<ComponentType>::ID();

			uint64_t thisArchetypeIndex = 0;
			uint64_t fromArchetypeIndex = 0;

			for (; thisArchetypeIndex < m_ComponentsCount; thisArchetypeIndex++)
			{
				ArchetypeTypeData& archetypeTypeData = m_TypeData[thisArchetypeIndex];
				if (archetypeTypeData.m_TypeID == newComponentTypeID)
				{
					continue;
				}

				archetypeTypeData.m_PackedContainer->EmplaceFromVoid(
					fromArchetype.m_TypeData[fromArchetypeIndex].m_PackedContainer->GetComponentDataAsVoid(fromIndex)
				);

				fromArchetypeIndex++;
			}
		}

		template<typename ComponentType>
		inline PackedContainer<ComponentType>* GetContainerAt(const uint64_t& index)
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

		void ReserveSpaceInArchetype(const uint64_t& desiredCapacity)
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

		void InitEmptyFromOther(Archetype& other, ecsMap<TypeID, ComponentContextBase*>& contextsMap)
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
					contextsMap[otherTypeData.m_TypeID]
				);

				m_AddingOrderTypeIDs.push_back(other.m_AddingOrderTypeIDs[i]);
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
	};

}