#pragma once
#include "Core.h"
#include "ComponentContextsManager.h"
#include "Type.h"
#include "PackedComponentContainer.h"

namespace decs
{
	class Entity;
	class Archetype;

	class ArchetypeEdge final
	{
	public:
		Archetype* RemoveEdge = nullptr;
		Archetype* AddEdge = nullptr;
	public:
		ArchetypeEdge()
		{

		}

		ArchetypeEdge(Archetype* removeEdge, Archetype* addEdge) :
			RemoveEdge(removeEdge),
			AddEdge(addEdge)
		{

		}
	};

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
			return ID != std::numeric_limits<EntityID>::max() && Index != std::numeric_limits<uint64_t>::max();
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
		ecsMap<TypeID, ArchetypeEdge> m_Edges;

		std::vector<TypeID> m_TypeIDs;
		ecsMap<TypeID, uint64_t> m_TypeIDsIndexes;

		uint64_t m_EntitesCountToInitialize = 0;

	private:
		uint32_t m_ComponentsCount = 0; // number of components for each entity
		uint32_t m_EntitiesCount = 0;

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
		inline uint64_t EntitesCountToInvokeCallbacks() const { return m_EntitesCountToInitialize; }

		inline bool ContainType(const TypeID& typeID) const
		{
			return m_TypeIDsIndexes.find(typeID) != m_TypeIDsIndexes.end();
		}

		inline uint64_t FindTypeIndex(const TypeID& typeID) const
		{
			if (m_ComponentsCount < m_MinComponentsInArchetypeToPerformMapLookup)
			{
				for (uint64_t i = 0; i < m_ComponentsCount; i++)
					if (m_TypeIDs[i] == typeID) return i;

				return std::numeric_limits<uint64_t>::max();
			}

			auto it = m_TypeIDsIndexes.find(typeID);
			if (it == m_TypeIDsIndexes.end())
				return std::numeric_limits<uint64_t>::max();

			return it->second;
		}

		template<typename T>
		inline uint64_t FindTypeIndex() const
		{
			constexpr TypeID typeID = Type<T>::ID();
			if (m_ComponentsCount < m_MinComponentsInArchetypeToPerformMapLookup)
			{
				for (uint64_t i = 0; i < m_ComponentsCount; i++)
					if (m_TypeIDs[i] == typeID) return i;

				return std::numeric_limits<uint64_t>::max();
			}

			auto it = m_TypeIDsIndexes.find(typeID);
			if (it == m_TypeIDsIndexes.end())
				return std::numeric_limits<uint64_t>::max();

			return it->second;
		}

		template<typename ComponentType>
		void AddTypeID(ComponentContextBase* componentContext)
		{
			constexpr TypeID id = Type<ComponentType>::ID();
			auto it = m_TypeIDsIndexes.find(id);
			if (it == m_TypeIDsIndexes.end())
			{
				m_ComponentsCount += 1;
				m_TypeIDsIndexes[id] = m_TypeIDs.size();
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
				m_TypeIDsIndexes[id] = m_TypeIDs.size();
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
			const uint64_t componentIndex = FindTypeIndex<ComponentType>();
			if (componentIndex != std::numeric_limits<uint64_t>::max())
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

			for (uint64_t i = 0; i < m_ComponentsCount; i++)
			{
				TypeID id = other.m_TypeIDs[i];
				m_TypeIDs.push_back(id);
				m_TypeIDsIndexes[id] = i;
				m_PackedContainers.push_back(other.m_PackedContainers[i]->CreateOwnEmptyCopy());
				m_ComponentContexts.push_back(contextsMap[id]);
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

	class ArchetypesMap
	{
		friend class Container;
		template<typename ...Types>
		friend class View;

	public:
		ArchetypesMap()
		{

		}

		~ArchetypesMap()
		{
		}

		inline uint64_t ArchetypesCount() const noexcept
		{
			return m_Archetypes.Size();
		}

		inline uint64_t EmptyArchetypesCount() const
		{
			uint64_t emptyArchetypesCount = 0;
			uint64_t archetypesCount = m_Archetypes.Size();

			for (uint64_t i = 0; i < archetypesCount; i++)
			{
				if (m_Archetypes[i].EntitiesCount() == 0)
				{
					emptyArchetypesCount += 1;
				}
			}

			return emptyArchetypesCount;
		}

	private:
		ChunkedVector<Archetype> m_Archetypes;
		ecsMap<TypeID, Archetype*> m_SingleComponentArchetypes;
		std::vector<std::vector<Archetype*>> m_ArchetypesGroupedByComponentsCount;

	private:
		void MakeArchetypeEdges(Archetype& archetype)
		{
			// edges with archetypes with less components:
			{
				const uint64_t componentCountsMinusOne = archetype.ComponentsCount() - 1;

				if (componentCountsMinusOne > 0)
				{
					const uint64_t archetypeListIndex = componentCountsMinusOne - 1;
					auto& archetypesListToCreateEdges = m_ArchetypesGroupedByComponentsCount[archetypeListIndex];

					uint64_t archCount = archetypesListToCreateEdges.size();
					for (uint64_t archIdx = 0; archIdx < archCount; archIdx++)
					{
						auto& testArchetype = *archetypesListToCreateEdges[archIdx];

						uint64_t incorrectTests = 0;
						TypeID notFindedType;
						bool isArchetypeValid = true;

						for (uint64_t typeIdx = 0; typeIdx < archetype.ComponentsCount(); typeIdx++)
						{
							TypeID typeID = archetype.ComponentsTypes()[typeIdx];

							auto it = testArchetype.m_TypeIDsIndexes.find(typeID);

							if (it == testArchetype.m_TypeIDsIndexes.end())
							{
								incorrectTests += 1;
								if (incorrectTests > 1)
								{
									isArchetypeValid = false;
									break;
								}
								else
								{
									notFindedType = typeID;
								}
							}
						}

						if (isArchetypeValid)
						{
							ArchetypeEdge& testArchetypeEdge = testArchetype.m_Edges[notFindedType];
							testArchetypeEdge.AddEdge = &archetype;

							ArchetypeEdge& archetypeEdge = archetype.m_Edges[notFindedType];
							archetypeEdge.RemoveEdge = &testArchetype;
						}
					}
				}
			}

			// edges with archetype with more components:
			{
				const uint64_t componentCountsPlusOne = (uint64_t)archetype.ComponentsCount() + 1;

				if (componentCountsPlusOne <= m_ArchetypesGroupedByComponentsCount.size())
				{
					const uint64_t archetypeListIndex = componentCountsPlusOne - 1;
					auto& archetypesListToCreateEdges = m_ArchetypesGroupedByComponentsCount[archetypeListIndex];

					uint64_t archCount = archetypesListToCreateEdges.size();
					for (uint64_t archIdx = 0; archIdx < archCount; archIdx++)
					{
						auto& testArchetype = *archetypesListToCreateEdges[archIdx];

						uint64_t incorrectTests = 0;
						TypeID lastIncorrectType;
						bool isArchetypeValid = true;

						for (uint64_t typeIdx = 0; typeIdx < testArchetype.ComponentsCount(); typeIdx++)
						{
							TypeID typeID = testArchetype.ComponentsTypes()[typeIdx];
							auto it = archetype.m_TypeIDsIndexes.find(typeID);
							if (it == archetype.m_TypeIDsIndexes.end())
							{
								incorrectTests += 1;
								if (incorrectTests > 1)
								{
									isArchetypeValid = false;
									break;
								}
								else
								{
									lastIncorrectType = typeID;
								}
							}
						}

						if (isArchetypeValid)
						{
							ArchetypeEdge& testArchetypeEdge = testArchetype.m_Edges[lastIncorrectType];
							testArchetypeEdge.RemoveEdge = &archetype;

							ArchetypeEdge& archetypeEdge = archetype.m_Edges[lastIncorrectType];
							archetypeEdge.AddEdge = &testArchetype;
						}
					}
				}
			}


		}

		void AddArchetypeToCorrectContainers(Archetype& archetype, const bool& bTryAddToSingleComponentsMap = true)
		{
			if (bTryAddToSingleComponentsMap && archetype.ComponentsCount() == 1)
			{
				m_SingleComponentArchetypes[archetype.ComponentsTypes()[0]] = &archetype;
			}

			if (archetype.ComponentsCount() > m_ArchetypesGroupedByComponentsCount.size())
			{
				m_ArchetypesGroupedByComponentsCount.resize(archetype.ComponentsCount());
			}

			m_ArchetypesGroupedByComponentsCount[archetype.ComponentsCount() - 1].push_back(&archetype);
		}

		template<typename ComponentType>
		Archetype* GetSingleComponentArchetype()
		{
			constexpr TypeID typeID = Type<ComponentType>::ID();
			auto it = m_SingleComponentArchetypes.find(typeID);

			return it != m_SingleComponentArchetypes.end() ? it->second : nullptr;
		}

		template<typename ComponentType>
		Archetype* CreateSingleComponentArchetype(ComponentContextBase* componentContext)
		{
			constexpr uint64_t typeID = Type<ComponentType>::ID();
			auto& archetype = m_SingleComponentArchetypes[typeID];
			if (archetype != nullptr) return archetype;
			archetype = &m_Archetypes.EmplaceBack();
			archetype->AddTypeID<ComponentType>(componentContext);
			MakeArchetypeEdges(*archetype);
			AddArchetypeToCorrectContainers(*archetype, false);
			return archetype;
		}

		Archetype* CreateSingleComponentArchetype(Archetype& from)
		{
			uint64_t typeID = from.m_TypeIDs[0];
			auto& archetype = m_SingleComponentArchetypes[typeID];
			if (archetype != nullptr) return archetype;
			archetype = &m_Archetypes.EmplaceBack();
			archetype->AddTypeID(typeID, from.m_PackedContainers[0], from.m_ComponentContexts[0]);
			MakeArchetypeEdges(*archetype);
			AddArchetypeToCorrectContainers(*archetype, false);
			return archetype;
		}

		template<typename T>
		inline Archetype* GetArchetypeAfterAddComponent(Archetype& toArchetype)
		{
			constexpr TypeID addedComponentTypeID = Type<T>::ID();
			auto edge = toArchetype.m_Edges.find(addedComponentTypeID);

			if (edge == toArchetype.m_Edges.end()) return nullptr;
			return edge->second.AddEdge;
		}

		template<typename T>
		inline Archetype* CreateArchetypeAfterAddComponent(Archetype& toArchetype, ComponentContextBase* componentContext)
		{
			constexpr TypeID addedComponentTypeID = Type<T>::ID();
			ArchetypeEdge& edge = toArchetype.m_Edges[addedComponentTypeID];
			if (edge.AddEdge != nullptr)
			{
				return edge.AddEdge;
			}

			Archetype& newArchetype = m_Archetypes.EmplaceBack();
			bool isNewComponentTypeAdded = false;

			for (uint32_t i = 0; i < toArchetype.ComponentsCount(); i++)
			{
				TypeID currentTypeID = toArchetype.m_TypeIDs[i];
				if (!isNewComponentTypeAdded && currentTypeID > addedComponentTypeID)
				{
					isNewComponentTypeAdded = true;
					newArchetype.AddTypeID<T>(componentContext);
				}

				newArchetype.AddTypeID(currentTypeID, toArchetype.m_PackedContainers[i], toArchetype.m_ComponentContexts[i]);
			}
			if (!isNewComponentTypeAdded)
			{
				newArchetype.AddTypeID<T>(componentContext);
			}

			MakeArchetypeEdges(newArchetype);
			AddArchetypeToCorrectContainers(newArchetype);

			return &newArchetype;
		}

		inline Archetype* CreateArchetypeAfterAddComponent(
			Archetype& toArchetype,
			Archetype& archetypeToGetContainer
		)
		{
			uint64_t componentIndexToAdd = toArchetype.ComponentsCount();
			TypeID addedComponentTypeID = archetypeToGetContainer.m_TypeIDs[componentIndexToAdd];

			ArchetypeEdge& edge = toArchetype.m_Edges[addedComponentTypeID];
			if (edge.AddEdge != nullptr)
			{
				return edge.AddEdge;
			}

			Archetype& newArchetype = m_Archetypes.EmplaceBack();
			bool isNewComponentTypeAdded = false;

			for (uint32_t i = 0; i < toArchetype.ComponentsCount(); i++)
			{
				TypeID currentTypeID = toArchetype.m_TypeIDs[i];
				if (!isNewComponentTypeAdded && currentTypeID > addedComponentTypeID)
				{
					isNewComponentTypeAdded = true;
					newArchetype.AddTypeID(
						addedComponentTypeID,
						archetypeToGetContainer.m_PackedContainers[componentIndexToAdd],
						archetypeToGetContainer.m_ComponentContexts[componentIndexToAdd]
					);
				}

				newArchetype.AddTypeID(currentTypeID, toArchetype.m_PackedContainers[i], toArchetype.m_ComponentContexts[i]);
			}
			if (!isNewComponentTypeAdded)
			{
				newArchetype.AddTypeID(
					addedComponentTypeID,
					archetypeToGetContainer.m_PackedContainers[componentIndexToAdd],
					archetypeToGetContainer.m_ComponentContexts[componentIndexToAdd]
				);
			}

			MakeArchetypeEdges(newArchetype);
			AddArchetypeToCorrectContainers(newArchetype);

			return &newArchetype;
		}

		Archetype* GetArchetypeAfterRemoveComponent(Archetype& fromArchetype, const TypeID& removedComponentTypeID)
		{
			if (fromArchetype.ComponentsCount() == 1 && fromArchetype.m_TypeIDs[0] == removedComponentTypeID)
			{
				return nullptr;
			}

			ArchetypeEdge& edge = fromArchetype.m_Edges[removedComponentTypeID];
			if (edge.RemoveEdge != nullptr)
			{
				return edge.RemoveEdge;
			}

			Archetype& newArchetype = m_Archetypes.EmplaceBack();
			for (uint32_t i = 0; i < fromArchetype.ComponentsCount(); i++)
			{
				TypeID typeID = fromArchetype.ComponentsTypes()[i];
				if (typeID != removedComponentTypeID)
				{
					newArchetype.AddTypeID(typeID, fromArchetype.m_PackedContainers[i], fromArchetype.m_ComponentContexts[i]);
				}
			}

			MakeArchetypeEdges(newArchetype);
			AddArchetypeToCorrectContainers(newArchetype);

			return &newArchetype;
		}

		template<typename T>
		inline Archetype* GetArchetypeAfterRemoveComponent(Archetype& fromArchetype)
		{
			return GetArchetypeAfterRemoveComponent(fromArchetype, Type<T>::ID());
		}

		Archetype* GetSingleComponentArchetype(const TypeID& typeID)
		{
			auto it = m_SingleComponentArchetypes.find(typeID);
			if (it != m_SingleComponentArchetypes.end())
			{
				return it->second;
			}
			return nullptr;
		}

		Archetype* FindArchetype(TypeID* types, const uint64_t typesCount)
		{
			Archetype* archetype = GetSingleComponentArchetype(types[0]);
			uint64_t typeIndex = 1;

			if (archetype == nullptr) return nullptr;

			while (typeIndex < typesCount)
			{
				auto it = archetype->m_Edges.find(types[typeIndex]);
				if (it != archetype->m_Edges.end() && it->second.AddEdge != nullptr)
				{
					archetype = it->second.AddEdge;
					typeIndex += 1;
					continue;
				}
				return nullptr;
			}

			return archetype;
		}

		Archetype* FindArchetypeFromOther(
			Archetype& fromArchetype, 
			ComponentContextsManager* componentContextsManager,
			ObserversManager* observerManager,
			const bool& isArchetypeFromSameArchetypesMap
		)
		{
			Archetype* archetype = FindArchetype(fromArchetype.m_TypeIDs.data(), fromArchetype.m_TypeIDs.size());
			if (archetype == nullptr)
			{
				archetype = &m_Archetypes.EmplaceBack();

				// take care that componentContextsManager have the same component contexts that component contextManager which have "fromArchetype" archetype
				for (uint64_t i = 0; i < fromArchetype.ComponentsCount(); i++)
				{
					auto& context = componentContextsManager->m_Contexts[fromArchetype.m_TypeIDs[i]];
					if (context == nullptr)
					{
						context = fromArchetype.m_ComponentContexts[i]->CreateOwnEmptyCopy(observerManager);
					}
				}

				archetype->InitEmptyFromOther(fromArchetype, componentContextsManager->m_Contexts);
				AddArchetypeToCorrectContainers(*archetype, true);

				Archetype* archetypeBuffor = CreateSingleComponentArchetype(*archetype);

				for (uint64_t i = 1; i < archetype->ComponentsCount() - 1; i++)
				{
					archetypeBuffor = CreateArchetypeAfterAddComponent(*archetypeBuffor, *archetype);
				}

				MakeArchetypeEdges(*archetype);
			}

			return archetype;
		}


	};
}