#pragma once
#include "Core.h"
#include "Archetype.h"
#include "ComponentContextsManager.h"
#include "Type.h"
#include "PackedComponentContainer.h"

namespace decs
{

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

#pragma region UTILITY
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
							testArchetype.m_AddEdges[notFindedType] = &archetype;
							archetype.m_RemoveEdges[notFindedType] = &testArchetype;
						}
					}

					if (archetype.m_RemoveEdges.size() == 0)
					{
						GetArchetypeAfterRemoveComponent(archetype, archetype.m_AddingOrderTypeIDs[archetype.ComponentsCount() - 1]);
					}
				}
			}

			// edges with archetype with more components:
			{
				const uint64_t componentCountsPlusOne = (uint64_t)archetype.ComponentsCount() + 1;

				if (componentCountsPlusOne <= m_ArchetypesGroupedByComponentsCount.size())
				{
					const uint64_t archetypeListIndex = archetype.ComponentsCount();

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
							testArchetype.m_RemoveEdges[lastIncorrectType] = &archetype;
							archetype.m_AddEdges[lastIncorrectType] = &testArchetype;
						}
					}
				}
			}
		}

		void CreateRemoveEdgesForArchetype(Archetype& archetype)
		{
			Archetype* buffor = &archetype;
			while (buffor->ComponentsCount() > 1 && buffor->m_RemoveEdges.size() == 0)
			{
				buffor = GetArchetypeAfterRemoveComponent(*buffor, buffor->m_AddingOrderTypeIDs[archetype.ComponentsCount() - 1]);
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
			if (typesCount == 0) return nullptr;

			Archetype* archetype = GetSingleComponentArchetype(types[0]);
			uint64_t typeIndex = 1;

			if (archetype == nullptr) return nullptr;

			while (typeIndex < typesCount)
			{
				auto it = archetype->m_AddEdges.find(types[typeIndex]);
				if (it != archetype->m_AddEdges.end() && it->second != nullptr)
				{
					archetype = it->second;
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
			ObserversManager* observerManager
		)
		{
			Archetype* archetype = FindArchetype(fromArchetype.m_AddingOrderTypeIDs.data(), fromArchetype.m_AddingOrderTypeIDs.size());
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


#pragma endregion

#pragma region CREATING ARCHETYPES

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
			AddArchetypeToCorrectContainers(*archetype, false);
			MakeArchetypeEdges(*archetype);
			archetype->AddTypeToAddingComponentOrder<ComponentType>();
			return archetype;
		}

		Archetype* CreateSingleComponentArchetype(Archetype& from)
		{
			uint64_t typeID = from.m_AddingOrderTypeIDs[0];
			auto& archetype = m_SingleComponentArchetypes[typeID];
			if (archetype != nullptr) return archetype;
			archetype = &m_Archetypes.EmplaceBack();
			archetype->AddTypeID(typeID, from.m_PackedContainers[0], from.m_ComponentContexts[0]);
			AddArchetypeToCorrectContainers(*archetype, false);
			MakeArchetypeEdges(*archetype);
			archetype->AddTypeToAddingComponentOrder(from.m_AddingOrderTypeIDs[0]);
			return archetype;
		}

		template<typename T>
		inline Archetype* GetArchetypeAfterAddComponent(Archetype& toArchetype)
		{
			constexpr TypeID addedComponentTypeID = Type<T>::ID();
			auto edge = toArchetype.m_AddEdges.find(addedComponentTypeID);

			if (edge == toArchetype.m_AddEdges.end()) return nullptr;
			return edge->second;
		}

		template<typename T>
		inline Archetype* CreateArchetypeAfterAddComponent(Archetype& toArchetype, ComponentContextBase* componentContext)
		{
			constexpr TypeID addedComponentTypeID = Type<T>::ID();
			auto& edge = toArchetype.m_AddEdges[addedComponentTypeID];
			if (edge != nullptr)
			{
				return edge;
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
				newArchetype.AddTypeToAddingComponentOrder(toArchetype.m_AddingOrderTypeIDs[i]);
			}
			if (!isNewComponentTypeAdded)
			{
				newArchetype.AddTypeID<T>(componentContext);
			}

			newArchetype.AddTypeToAddingComponentOrder<T>();
			AddArchetypeToCorrectContainers(newArchetype);
			MakeArchetypeEdges(newArchetype);

			return &newArchetype;
		}

		inline Archetype* CreateArchetypeAfterAddComponent(
			Archetype& toArchetype,
			Archetype& archetypeToGetContainer
		)
		{
			uint64_t componentIndexToAdd = toArchetype.ComponentsCount();
			TypeID addedComponentTypeID = archetypeToGetContainer.m_TypeIDs[componentIndexToAdd];

			auto& edge = toArchetype.m_AddEdges[addedComponentTypeID];
			if (edge != nullptr)
			{
				return edge;
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
				newArchetype.AddTypeToAddingComponentOrder(toArchetype.m_AddingOrderTypeIDs[i]);
			}
			if (!isNewComponentTypeAdded)
			{
				newArchetype.AddTypeID(
					addedComponentTypeID,
					archetypeToGetContainer.m_PackedContainers[componentIndexToAdd],
					archetypeToGetContainer.m_ComponentContexts[componentIndexToAdd]
				);
			}

			newArchetype.AddTypeToAddingComponentOrder(addedComponentTypeID);
			AddArchetypeToCorrectContainers(newArchetype);
			MakeArchetypeEdges(newArchetype);

			return &newArchetype;
		}

		Archetype* GetArchetypeAfterRemoveComponent(Archetype& fromArchetype, const TypeID& removedComponentTypeID)
		{
			if (fromArchetype.ComponentsCount() == 1 && fromArchetype.m_TypeIDs[0] == removedComponentTypeID)
			{
				return nullptr;
			}

			auto& edge = fromArchetype.m_RemoveEdges[removedComponentTypeID];
			if (edge != nullptr)
			{
				return edge;
			}

			Archetype& newArchetype = m_Archetypes.EmplaceBack();
			for (uint32_t i = 0; i < fromArchetype.ComponentsCount(); i++)
			{

				TypeID typeID = fromArchetype.ComponentsTypes()[i];
				if (typeID != removedComponentTypeID)
				{
					newArchetype.AddTypeID(typeID, fromArchetype.m_PackedContainers[i], fromArchetype.m_ComponentContexts[i]);
				}
				if (fromArchetype.m_AddingOrderTypeIDs[i] != removedComponentTypeID)
				{
					newArchetype.m_AddingOrderTypeIDs.push_back(fromArchetype.m_AddingOrderTypeIDs[i]);
				}
			}

			AddArchetypeToCorrectContainers(newArchetype);
			MakeArchetypeEdges(newArchetype);

			return &newArchetype;
		}

		template<typename T>
		inline Archetype* GetArchetypeAfterRemoveComponent(Archetype& fromArchetype)
		{
			return GetArchetypeAfterRemoveComponent(fromArchetype, Type<T>::ID());
		}
#pragma endregion

	};
}