#pragma once
#include "decspch.h"

#include "Core.h"
#include "ComponentContext.h"
#include "Type.h"
#include "TypeGroup.h"

namespace decs
{
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

		~ArchetypeEdge() {}
	};

	struct ComponentRef
	{
	public:
		uint32_t BucketIndex = std::numeric_limits< uint32_t>::max();
		uint32_t ElementIndex = std::numeric_limits< uint32_t>::max();
		void* ComponentPointer = nullptr;

	public:
		ComponentRef()
		{

		}

		ComponentRef(
			const uint32_t& bucketIndex,
			const uint32_t& elementIndex,
			void*& componentPointer
		) :
			BucketIndex(bucketIndex),
			ElementIndex(elementIndex),
			ComponentPointer(componentPointer)
		{

		}
	};

	struct ArchetypeEntityData
	{
	public:
		EntityID ID = std::numeric_limits<EntityID>::max();
		bool IsActive = false;

	public:
		ArchetypeEntityData()
		{

		}

		ArchetypeEntityData(
			const EntityID& id,
			const bool& isActive
		) :
			ID(id),
			IsActive(isActive)
		{

		}
	};

	class Archetype final
	{
		friend class Container;
	public:
		uint32_t m_ComponentsCount = 0;
		uint32_t m_EntitiesCount = 0;

		std::vector<ArchetypeEntityData> m_EntitiesData;
		std::vector<ComponentRef> m_ComponentsRefs;

		std::vector<ComponentContextBase*> m_ComponentContexts;
		ecsMap<TypeID, ArchetypeEdge> m_Edges;

		std::vector<TypeID> m_TypeIDs;
		ecsMap<TypeID, uint32_t> m_TypeIDsIndexes;

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

		void AddTypeID(TypeID id, ComponentContextBase* componentContext)
		{
			auto it = m_TypeIDsIndexes.find(id);
			if (it == m_TypeIDsIndexes.end())
			{
				m_ComponentsCount += 1;
				m_TypeIDsIndexes[id] = m_TypeIDs.size();
				m_TypeIDs.push_back(id);
				m_ComponentContexts.push_back(componentContext);
			}
		}

		inline uint32_t GetComponentsCount() const { return m_ComponentsCount; }
		inline const TypeID* const ComponentsTypes() const { return m_TypeIDs.data(); }

		inline uint32_t EntitiesCount() const { return m_EntitiesCount; }

		inline bool ContainType(const TypeID& typeID) const
		{
			return m_TypeIDsIndexes.find(typeID) != m_TypeIDsIndexes.end();
		}
	};

	class ArchetypesMap
	{
		friend class Container;

		template<typename ...Types>
		friend class ViewBaseWithEntity;
		template<typename ...Types>
		friend class ViewBase;
		template<typename ...Types>
		friend class View;
		template<typename ...Types>
		friend class ComplexView;

	public:
		ArchetypesMap()
		{

		}

		~ArchetypesMap()
		{
			for (uint64_t i = 0; i < m_Archetypes.size(); i++)
			{
				delete m_Archetypes[i];
			}
		}

		template<typename T>
		Archetype* GetSingleComponentArchetype(ComponentContextBase* componentContext)
		{
			constexpr auto typeID = Type<T>::ID();
			auto& archetype = m_SingleComponentArchetypes[typeID];

			if (archetype == nullptr)
			{
				archetype = new Archetype(typeID, componentContext);
				MakeArchetypeEdges(*archetype);
				AddArchetypeToCorrectContainers(*archetype, false);
			}

			return archetype;
		}


		Archetype* GetArchetypeAfterAddComponent(Archetype& toArchetype, const TypeID& addedComponentTypeID, ComponentContextBase* componentContainer)
		{
			ArchetypeEdge& edge = toArchetype.m_Edges[addedComponentTypeID];

			if (edge.AddEdge != nullptr)
			{
				return edge.AddEdge;
			}

			Archetype* newArchetype = new Archetype();
			for (uint32_t i = 0; i < toArchetype.GetComponentsCount(); i++)
			{
				newArchetype->AddTypeID(toArchetype.m_TypeIDs[i], toArchetype.m_ComponentContexts[i]);
			}
			newArchetype->AddTypeID(addedComponentTypeID, componentContainer);

			MakeArchetypeEdges(*newArchetype);
			AddArchetypeToCorrectContainers(*newArchetype);

			return newArchetype;
		}

		template<typename T>
		inline Archetype* GetArchetypeAfterAddComponent(Archetype& toArchetype, ComponentContextBase* componentContext)
		{
			return GetArchetypeAfterAddComponent(toArchetype, Type<T>::ID(), componentContext);
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

			Archetype* newArchetype = new Archetype();
			for (uint32_t i = 0; i < fromArchetype.GetComponentsCount(); i++)
			{
				TypeID typeID = fromArchetype.ComponentsTypes()[i];
				if (typeID != removedComponentTypeID)
				{
					newArchetype->AddTypeID(typeID, fromArchetype.m_ComponentContexts[i]);
				}
			}

			MakeArchetypeEdges(*newArchetype);
			AddArchetypeToCorrectContainers(*newArchetype);

			return newArchetype;
		}

		template<typename T>
		inline Archetype* GetArchetypeAfterRemoveComponent(Archetype& fromArchetype)
		{
			return GetArchetypeAfterRemoveComponent(fromArchetype, Type<T>::ID());
		}

		template<typename... Types>
		Archetype* CreateArchetype()
		{
			TypeGroup<Types...> typesGroup = {};

			Archetype* archetype = new Archetype();

			for (uint32_t typeIdx = 0; typeIdx < typesGroup.Size(); typeIdx++)
			{
				archetype->AddTypeID(typesGroup.IDs()[typeIdx]);
			}

			MakeArchetypeEdges(*archetype);
			AddArchetypeToCorrectContainers(*archetype, true);
		}


	private:
		std::vector<Archetype*> m_Archetypes;
		ecsMap<TypeID, Archetype*> m_SingleComponentArchetypes;
		std::vector<std::vector<Archetype*>> m_ArchetypesGroupedByComponentsCount;

	private:

		void MakeArchetypeEdges(Archetype& archetype)
		{
			// edges with archetypes with less components:
			{
				const uint32_t componentCountsMinusOne = archetype.GetComponentsCount() - 1;

				if (componentCountsMinusOne > 0)
				{
					const uint32_t archetypeListIndex = componentCountsMinusOne - 1;
					auto& archetypesListToCreateEdges = m_ArchetypesGroupedByComponentsCount[archetypeListIndex];

					uint32_t archCount = archetypesListToCreateEdges.size();
					for (uint32_t archIdx = 0; archIdx < archCount; archIdx++)
					{
						auto& testArchetype = *archetypesListToCreateEdges[archIdx];

						uint32_t incorrectTests = 0;
						TypeID notFindedType;
						bool isArchetypeValid = true;

						for (uint32_t typeIdx = 0; typeIdx < archetype.GetComponentsCount(); typeIdx++)
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
				const uint32_t componentCountsPlusOne = archetype.GetComponentsCount() + 1;

				if (componentCountsPlusOne <= m_ArchetypesGroupedByComponentsCount.size())
				{
					const uint32_t archetypeListIndex = componentCountsPlusOne - 1;
					auto& archetypesListToCreateEdges = m_ArchetypesGroupedByComponentsCount[archetypeListIndex];

					uint32_t archCount = archetypesListToCreateEdges.size();
					for (uint32_t archIdx = 0; archIdx < archCount; archIdx++)
					{
						auto& testArchetype = *archetypesListToCreateEdges[archIdx];

						uint32_t incorrectTests = 0;
						TypeID lastIncorrectType;
						bool isArchetypeValid = true;

						for (uint32_t typeIdx = 0; typeIdx < testArchetype.GetComponentsCount(); typeIdx++)
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
			m_Archetypes.push_back(&archetype);

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
	};

}