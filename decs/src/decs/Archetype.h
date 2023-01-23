#pragma once
#include "Core.h"
#include "ComponentContext.h"
#include "Type.h"
#include "TypeGroup.h"
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

	class Archetype final
	{
		friend class Container;
		template<typename... ComponentTypes>
		friend 	class ViewIterator;
		template<typename... ComponentTypes>
		friend 	class ViewBatchIterator;

	private:
		static constexpr uint64_t m_MinComponentsInArchetypeToPerformMapLookup = 20;
		static constexpr uint64_t DefaultChunkSize = 1000;

	public:
		uint32_t m_ComponentsCount = 0; // number of components for each entity
		uint32_t m_EntitiesCount = 0;

		std::vector<ArchetypeEntityData> m_EntitiesData;
		std::vector<PackedContainerBase*> m_PackedContainers;

		std::vector<ComponentContextBase*> m_ComponentContexts;
		ecsMap<TypeID, ArchetypeEdge> m_Edges;

		std::vector<TypeID> m_TypeIDs;
		ecsMap<TypeID, uint64_t> m_TypeIDsIndexes;

		uint64_t m_EntitesCountToInitialize = 0;

	public:
		Archetype()
		{
		}

		Archetype(TypeID componentTypeID, ComponentContextBase* componentContext)
		{
			m_ComponentsCount = 1;
			m_TypeIDs.emplace_back(componentTypeID);
			m_TypeIDsIndexes[componentTypeID] = 0;
			m_ComponentContexts.push_back(componentContext);
		}

		~Archetype()
		{

		}

		inline uint32_t GetComponentsCount() const { return m_ComponentsCount; }
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
			constexpr id = Type<ComponentType>::ID();
			auto it = m_TypeIDsIndexes.find(id);
			if (it == m_TypeIDsIndexes.end())
			{
				m_ComponentsCount += 1;
				m_TypeIDsIndexes[id] = m_TypeIDs.size();
				m_TypeIDs.push_back(id);
				m_ComponentContexts.push_back(componentContext);
				m_PackedContainers.push_back(new PackedContainer<ComponentType>(DefaultChunkSize));
			}
		}

		void AddTypeID(const TypeID id, PackedContainerBase* frompackedContainer, ComponentContextBase* componentContext)
		{
			auto it = m_TypeIDsIndexes.find(id);
			if (it == m_TypeIDsIndexes.end())
			{
				m_ComponentsCount += 1;
				m_TypeIDsIndexes[id] = m_TypeIDs.size();
				m_TypeIDs.push_back(id);
				m_ComponentContexts.push_back(componentContext);
				m_PackedContainers.push_back(frompackedContainer->CreateOwnEmptyCopy(DefaultChunkSize));
			}
		}

	private:
		void Reset()
		{
			m_EntitiesCount = 0;
			m_EntitiesData.clear();
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
				const uint64_t componentCountsMinusOne = archetype.GetComponentsCount() - 1;

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

						for (uint64_t typeIdx = 0; typeIdx < archetype.GetComponentsCount(); typeIdx++)
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
				const uint64_t componentCountsPlusOne = (uint64_t)archetype.GetComponentsCount() + 1;

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

						for (uint64_t typeIdx = 0; typeIdx < testArchetype.GetComponentsCount(); typeIdx++)
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
			if (bTryAddToSingleComponentsMap && archetype.GetComponentsCount() == 1)
			{
				m_SingleComponentArchetypes[archetype.ComponentsTypes()[0]] = &archetype;
			}

			if (archetype.GetComponentsCount() > m_ArchetypesGroupedByComponentsCount.size())
			{
				m_ArchetypesGroupedByComponentsCount.resize(archetype.GetComponentsCount());
			}

			m_ArchetypesGroupedByComponentsCount[archetype.GetComponentsCount() - 1].push_back(&archetype);
		}

		Archetype* GetSingleComponentArchetype(ComponentContextBase* componentContext, const TypeID& componentTypeID)
		{
			const TypeID typeID = componentTypeID;
			auto& archetype = m_SingleComponentArchetypes[typeID];

			if (archetype == nullptr)
			{
				archetype = &m_Archetypes.EmplaceBack(typeID, componentContext);
				MakeArchetypeEdges(*archetype);
				AddArchetypeToCorrectContainers(*archetype, false);
			}

			return archetype;
		}

		template<typename T>
		inline Archetype* GetSingleComponentArchetype(ComponentContextBase* componentContext)
		{
			return GetSingleComponentArchetype(componentContext, Type<T>::ID());
		}

		template<typename T>
		inline Archetype* GetArchetypeAfterAddComponent(Archetype& toArchetype, ComponentContextBase* componentContext)
		{
			constexpr TypeID& addedComponentTypeID = Type<T>::ID();
			ArchetypeEdge& edge = toArchetype.m_Edges[addedComponentTypeID];

			if (edge.AddEdge != nullptr)
			{
				return edge.AddEdge;
			}

			Archetype& newArchetype = m_Archetypes.EmplaceBack();
			bool isNewComponentTypeAdded = false;

			for (uint32_t i = 0; i < toArchetype.GetComponentsCount(); i++)
			{
				TypeID currentTypeID = toArchetype.m_TypeIDs[i];
				if (!isNewComponentTypeAdded && currentTypeID > addedComponentTypeID)
				{
					isNewComponentTypeAdded = true;
					newArchetype.AddTypeID(addedComponentTypeID, componentContainer);
				}
				newArchetype.AddTypeID(toArchetype.m_TypeIDs[i], toArchetype.m_ComponentContexts[i]);
			}
			if (!isNewComponentTypeAdded)
			{
				newArchetype.AddTypeID(addedComponentTypeID, componentContainer);
			}

			MakeArchetypeEdges(newArchetype);
			AddArchetypeToCorrectContainers(newArchetype);

			return &newArchetype;
		}


		Archetype* GetArchetypeAfterRemoveComponent(Archetype& fromArchetype, const TypeID& removedComponentTypeID)
		{
			if (fromArchetype.GetComponentsCount() == 1 && fromArchetype.m_TypeIDs[0] == removedComponentTypeID)
			{
				return nullptr;
			}

			ArchetypeEdge& edge = fromArchetype.m_Edges[removedComponentTypeID];
			if (edge.RemoveEdge != nullptr)
			{
				return edge.RemoveEdge;
			}

			Archetype& newArchetype = m_Archetypes.EmplaceBack();
			for (uint32_t i = 0; i < fromArchetype.GetComponentsCount(); i++)
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

		Archetype* FindArchetype(TypeID* types, const uint64_t typesCount)
		{


			return nullptr;
		}

	};
}