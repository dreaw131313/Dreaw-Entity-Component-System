#pragma once
#include "Core.h"
#include "ComponentContextsManager.h"
#include "Type.h"
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

	class Archetype final
	{
		friend class Container;
		friend class ArchetypesMap;
		template<typename... ComponentTypes>
		friend 	class ViewIterator;
		template<typename... ComponentTypes>
		friend 	class ViewBatchIterator;

	public:
		static constexpr uint64_t m_MinComponentsInArchetypeToPerformMapLookup = 20;
		static constexpr uint64_t DefaultChunkSize = 1000;

	public:
		std::vector<ArchetypeEntityData> m_EntitiesData;
		std::vector<PackedContainerBase*> m_PackedContainers;

		std::vector<ComponentContextBase*> m_ComponentContexts;

		ecsMap<TypeID, Archetype*> m_AddEdges;
		ecsMap<TypeID, Archetype*> m_RemoveEdges;

		std::vector<TypeID> m_TypeIDs;
		ecsMap<TypeID, uint32_t> m_TypeIDsIndexes;

		uint32_t m_EntitesCountToInitialize = 0;

	private:
		uint32_t m_ComponentsCount = 0; // number of components for each entity
		uint32_t m_EntitiesCount = 0;
		std::vector<TypeID> m_AddingOrderTypeIDs;

	public:
		Archetype()
		{
		}

		~Archetype()
		{
			for (auto& container : m_PackedContainers)
			{
				delete container;
			}
		}

		inline uint32_t ComponentsCount() const { return m_ComponentsCount; }
		inline const TypeID* const ComponentsTypes() const { return m_TypeIDs.data(); }
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
					if (m_TypeIDs[i] == typeID) return i;

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
					if (m_TypeIDs[i] == typeID) return i;

				return Limits::MaxComponentCount;
			}

			auto it = m_TypeIDsIndexes.find(typeID);
			if (it == m_TypeIDsIndexes.end())
				return Limits::MaxComponentCount;

			return it->second;
		}

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
				m_TypeIDsIndexes[id] = (uint32_t)m_TypeIDs.size();
				m_TypeIDs.push_back(id);
				m_ComponentContexts.push_back(componentContext);
				m_PackedContainers.push_back(new PackedContainer<ComponentType>(componentContext->GetChunkCapacity()));
			}
		}

		void AddTypeID(const TypeID& id, PackedContainerBase* frompackedContainer, ComponentContextBase* componentContext)
		{
			auto it = m_TypeIDsIndexes.find(id);
			if (it == m_TypeIDsIndexes.end())
			{
				m_ComponentsCount += 1;
				m_TypeIDsIndexes[id] = (uint32_t)m_TypeIDs.size();
				m_TypeIDs.push_back(id);
				m_ComponentContexts.push_back(componentContext);
				m_PackedContainers.push_back(frompackedContainer->CreateOwnEmptyCopy());
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
					m_PackedContainers[i]->PopBack();
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
					m_PackedContainers[i]->RemoveSwapBack(index);
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
				if (fromArchetype.m_TypeIDs[thisArchetypeIndex] == componentTypeID)
				{
					fromArchetypeIndex += 1;
				}
				m_PackedContainers[thisArchetypeIndex]->EmplaceFromVoid(
					fromArchetype.m_PackedContainers[fromArchetypeIndex]->GetComponentAsVoid(fromIndex)
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
				if (m_TypeIDs[thisArchetypeIndex] == newComponentTypeID)
				{
					continue;
				}

				m_PackedContainers[thisArchetypeIndex]->EmplaceFromVoid(
					fromArchetype.m_PackedContainers[fromArchetypeIndex]->GetComponentAsVoid(fromIndex)
				);

				fromArchetypeIndex++;
			}
		}

		template<typename ComponentType>
		inline PackedContainer<ComponentType>* GetContainerAt(const uint64_t& index)
		{
			return dynamic_cast<PackedContainer<ComponentType>*>(m_PackedContainers[index]);
		}

		inline void* GetComponentVoidPtr(const uint64_t& entityIndex, const uint64_t& componentIndex = 0)const noexcept
		{
			return m_PackedContainers[componentIndex]->GetComponentAsVoid(entityIndex);
		}

		template<typename ComponentType>
		ComponentType* GetEntityComponent(const uint64_t& entityIndex, uint64_t& componentIndex)
		{
			return dynamic_cast<PackedContainer<ComponentType>*>(m_PackedContainers[componentIndex])->m_Data[entityIndex];
		}

		template<typename ComponentType>
		ComponentType* GetEntityComponent(const uint64_t& entityIndex)
		{
			const uint32_t componentIndex = FindTypeIndex<ComponentType>();
			if (componentIndex != Limits::MaxComponentCount)
			{
				return dynamic_cast<PackedContainer<ComponentType>*>(m_PackedContainers[componentIndex])->m_Data[entityIndex];
			}
			return nullptr;
		}

		void ShrinkToFit()
		{
			m_EntitiesData.shrink_to_fit();
			for (uint64_t idx = 0; idx < m_ComponentsCount; idx++)
			{
				m_PackedContainers[idx]->ShrinkToFit();
			}
		}

		void ReserveSpaceInArchetype(const uint64_t& desiredCapacity)
		{
			if (m_EntitiesData.capacity() < desiredCapacity)
			{
				m_EntitiesData.reserve(desiredCapacity);
				for (auto& packedComponent : m_PackedContainers)
				{
					packedComponent->Reserve(desiredCapacity);
				}
			}
		}

		void InitEmptyFromOther(const Archetype& other, ecsMap<TypeID, ComponentContextBase*>& contextsMap)
		{
			m_ComponentsCount = other.m_ComponentsCount;
			m_PackedContainers.reserve(m_ComponentsCount);
			m_ComponentContexts.reserve(m_ComponentsCount);
			m_TypeIDs.reserve(m_ComponentsCount);
			m_AddingOrderTypeIDs.reserve(m_ComponentsCount);

			for (uint32_t i = 0; i < m_ComponentsCount; i++)
			{
				TypeID id = other.m_TypeIDs[i];
				m_TypeIDs.push_back(id);
				m_TypeIDsIndexes[id] = i;
				m_PackedContainers.push_back(other.m_PackedContainers[i]->CreateOwnEmptyCopy());
				m_ComponentContexts.push_back(contextsMap[id]);
				m_AddingOrderTypeIDs.push_back(other.m_AddingOrderTypeIDs[i]);
			}
		}

	private:
		void Reset()
		{
			m_EntitiesCount = 0;
			m_EntitiesData.clear();
			for (uint64_t compIdx = 0; compIdx < m_ComponentsCount; compIdx++)
			{
				m_PackedContainers[compIdx]->Clear();
			}
		}

		inline void ValidateEntitiesCountToInitialize()
		{
			m_EntitesCountToInitialize = m_EntitiesCount;
		}
	};

}