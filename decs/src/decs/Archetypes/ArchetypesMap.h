#pragma once
#include "decs\Core.h"
#include "decs\Type.h"
#include "decs\ComponentContext\ComponentContextsManager.h"
#include "decs\ComponentContainers\PackedContainer.h"

#include "Archetype.h"

namespace decs
{
	class ArchetypesShrinkToFitState
	{
		friend class ArchetypesMap;
	private:
		enum class State
		{
			Started,
			Ended
		};

	public:
		ArchetypesShrinkToFitState()
		{

		}

		ArchetypesShrinkToFitState(uint64_t archetypesToShrinkInOneCall, float maxArchetypeLoadFactor) :
			m_ArchetypesToShrinkInOneCall(archetypesToShrinkInOneCall),
			m_MaxArchetypeLoadFactor(maxArchetypeLoadFactor)
		{

		}

		void Reset()
		{
			m_State = State::Ended;
			m_ArchetypesCountToShrink = 0;
			m_CurretnArchetypeIndex = 0;
		}

		inline bool IsEnded()
		{
			return m_State == State::Ended;
		}

	private:
		State m_State = State::Ended;
		uint64_t m_ArchetypesCountToShrink = 0;
		uint64_t m_ArchetypesToShrinkInOneCall = 100;
		uint64_t m_CurretnArchetypeIndex = 0;
		float m_MaxArchetypeLoadFactor = 1.f;

	private:
		void Start(const uint64_t& archetypesToShrink)
		{
			m_State = State::Started;
			m_ArchetypesCountToShrink = archetypesToShrink;
			m_CurretnArchetypeIndex = 0;
		}
	};

	class ArchetypesGroupByOneType
	{
	public:
		ArchetypesGroupByOneType(TypeID mainTypeID) :
			m_MainTypeID(mainTypeID)
		{

		}

		inline uint64_t ArchetypesCount() const { return m_ArchetypesCount; }
		inline uint32_t MaxComponentsCount() const { return (uint32_t)m_Archetypes.size(); }

		void AddArchetype(Archetype* archetype)
		{
			m_ArchetypesCount += 1;
			uint64_t archetypesCount = archetype->ComponentCount();
			if (archetypesCount > m_Archetypes.size())
			{
				m_Archetypes.resize(archetypesCount);
			}

			m_Archetypes[archetypesCount - 1].push_back(archetype);
		}

		std::vector<Archetype*>& GetArchetypesWithComponentsCount(const uint64_t& componentsCount)
		{
			return m_Archetypes[componentsCount - 1];
		}

		inline Archetype* GetSingleComponentArchetype() const
		{
			if (m_Archetypes.size() > 0 && m_Archetypes[0].size() > 0)
			{
				return m_Archetypes[0][0];
			}
			return nullptr;
		}

		template<typename Callable>
		void IterateOverAllArchetypes(Callable&& func)
		{
			uint64_t archetypesGroupCount = m_Archetypes.size();
			for (uint64_t groupIdx = 0; groupIdx < archetypesGroupCount; groupIdx++)
			{
				auto& group = m_Archetypes[groupIdx];
				uint64_t archetypeCount = group.size();
				for (uint64_t archIdx = 0; archIdx < archetypeCount; archIdx++)
				{
					auto archetype = group[archIdx];
					func(archetype);
				}
			}
		}

	private:
		TypeID m_MainTypeID = std::numeric_limits<TypeID>::max();
		std::vector<std::vector<Archetype*>> m_Archetypes;
		uint64_t m_ArchetypesCount = 0;
	};

	class ArchetypesMap
	{
		friend class Container;
		template<typename>
		friend class ContainerSerializer;
		template<typename...>
		friend class Query;
		template<typename, typename...>
		friend class IterationContainerContext;

	public:
		ArchetypesMap()
		{

		}

		ArchetypesMap(uint64_t archetypesVectorChunkSize, uint64_t archetypeGroupsVectorChunkSize);

		~ArchetypesMap();

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
				if (m_Archetypes[i].EntityCount() == 0)
				{
					emptyArchetypesCount += 1;
				}
			}

			return emptyArchetypesCount;
		}

		inline uint64_t MaxNumberOfTypesInArchetype() const { return m_ArchetypesGroupedByComponentsCount.size(); }

		void ShrinkArchetypesToFit();

		void ShrinkArchetypesToFit(ArchetypesShrinkToFitState& state);

		inline const TChunkedVector<Archetype>& GetArchetypesChunkedVector() const
		{
			return m_Archetypes;
		}

		template<typename ComponentType>
		void UpdateOrderInAllArchetypesWithComponentType()
		{
			auto it = m_ArchetypesGroupedByOneType.find(Type<ComponentType>::ID());
			if (it != m_ArchetypesGroupedByOneType.end())
			{
				it->second->IterateOverAllArchetypes([](Archetype* arch)
				{
					arch->UpdateOrderOfComponentContexts();
				});
			}
		}

	private:
		TChunkedVector<Archetype> m_Archetypes = { 100 };
		ecsMap<TypeID, Archetype*> m_SingleComponentArchetypes;
		std::vector<std::vector<Archetype*>> m_ArchetypesGroupedByComponentsCount;

		TChunkedVector<ArchetypesGroupByOneType> m_ArchetrypesGroupsByOneTypeVector = { 100 };
		ecsMap<TypeID, ArchetypesGroupByOneType*> m_ArchetypesGroupedByOneType;

		// UTILITY
	private:
		void MakeArchetypeEdges(Archetype& archetype);

		void AddArchetypeToCorrectContainers(Archetype& archetype, bool bTryAddToSingleComponentsMap = true);

		inline Archetype* GetSingleComponentArchetype(TypeID typeID)
		{
			auto it = m_SingleComponentArchetypes.find(typeID);
			if (it != m_SingleComponentArchetypes.end())
			{
				return it->second;
			}
			return nullptr;
		}

		inline ArchetypesGroupByOneType* GetArchetypesGroup(TypeID id)
		{
			ArchetypesGroupByOneType*& group = m_ArchetypesGroupedByOneType[id];
			if (group == nullptr)
			{
				group = &m_ArchetrypesGroupsByOneTypeVector.EmplaceBack(id);
			}
			return group;
		}

		/// <summary>
		/// 
		/// </summary>
		/// <param name="archetypeToMatch"></param>
		/// <returns>Archetype and bool that indicates if archetypes is finded with edges.</returns>
		std::pair<Archetype*, bool> FindMatchingArchetype(Archetype* archetypeToMatch);

		Archetype* GetOrCreateMatchedArchetype(
			Archetype& fromArchetype,
			ComponentContextsManager* componentContextsManager,
			StableContainersManager* stableContainersManager,
			ObserversManager* observerManager
		);

		void AddArchetypeToGroups(Archetype* arch)
		{
			uint64_t componentsCount = arch->ComponentCount();
			for (uint64_t i = 0; i < componentsCount; i++)
			{
				const TypeID& id = arch->GetTypeID(i);
				ArchetypesGroupByOneType* group = GetArchetypesGroup(id);
				group->AddArchetype(arch);
			}
		}

		// CREATING ARCHETYPES
	private:
		template<typename ComponentType>
		Archetype* GetSingleComponentArchetype()
		{
			TYPE_ID_CONSTEXPR TypeID typeID = Type<ComponentType>::ID();
			auto it = m_SingleComponentArchetypes.find(typeID);

			return it != m_SingleComponentArchetypes.end() ? it->second : nullptr;
		}

		template<typename ComponentType>
		Archetype* CreateSingleComponentArchetype(ComponentContextBase* componentContext, StableContainerBase* stableContainer)
		{
			TYPE_ID_CONSTEXPR uint64_t typeID = Type<ComponentType>::ID();
			auto& archetype = m_SingleComponentArchetypes[typeID];
			if (archetype != nullptr) return archetype;
			archetype = &m_Archetypes.EmplaceBack();
			archetype->AddTypeID<ComponentType>(componentContext, stableContainer);
			AddArchetypeToCorrectContainers(*archetype, false);
			MakeArchetypeEdges(*archetype);
			return archetype;
		}

		//Archetype* CreateSingleComponentArchetype(Archetype& from)
		//{
		//	uint64_t typeID = from.m_TypeData[0].m_TypeID;
		//	uint64_t typeIndex = from.FindTypeIndex(typeID);
		//	auto& archetype = m_SingleComponentArchetypes[typeID];
		//	if (archetype != nullptr) return archetype;
		//	archetype = &m_Archetypes.EmplaceBack();
		//
		//	ArchetypeTypeData& fromTypeData = from.m_TypeData[typeIndex];
		//	archetype->AddTypeID(typeID, fromTypeData.m_PackedContainer, fromTypeData.m_ComponentContext, fromTypeData.m_StableContainer);
		//	AddArchetypeToCorrectContainers(*archetype, false);
		//	MakeArchetypeEdges(*archetype);
		//	return archetype;
		//}

		template<typename T>
		inline Archetype* GetArchetypeAfterAddComponent(Archetype& toArchetype)
		{
			TYPE_ID_CONSTEXPR TypeID addedComponentTypeID = Type<T>::ID();
			auto edge = toArchetype.m_AddEdges.find(addedComponentTypeID);

			if (edge == toArchetype.m_AddEdges.end()) return nullptr;
			return edge->second;
		}

		template<typename T>
		inline Archetype* CreateArchetypeAfterAddComponent(Archetype& toArchetype, ComponentContextBase* componentContext, StableContainerBase* stableContainer)
		{
			TYPE_ID_CONSTEXPR TypeID addedComponentTypeID = Type<T>::ID();
			auto& edge = toArchetype.m_AddEdges[addedComponentTypeID];
			if (edge != nullptr)
			{
				return edge;
			}

			Archetype& newArchetype = m_Archetypes.EmplaceBack();
			bool isNewComponentTypeAdded = false;

			for (uint32_t i = 0; i < toArchetype.ComponentCount(); i++)
			{
				ArchetypeTypeData& toTypeData = toArchetype.m_TypeData[i];
				TypeID currentTypeID = toTypeData.m_TypeID;
				if (!isNewComponentTypeAdded && currentTypeID > addedComponentTypeID)
				{
					isNewComponentTypeAdded = true;
					newArchetype.AddTypeID<T>(componentContext, stableContainer);
				}

				newArchetype.AddTypeID(currentTypeID, toTypeData.m_PackedContainer, toTypeData.m_ComponentContext, toTypeData.m_StableContainer);
			}

			if (!isNewComponentTypeAdded)
			{
				newArchetype.AddTypeID<T>(componentContext, stableContainer);
			}

			AddArchetypeToCorrectContainers(newArchetype);
			MakeArchetypeEdges(newArchetype);

			return &newArchetype;
		}

		inline Archetype* CreateArchetypeAfterAddComponent(
			Archetype& toArchetype,
			Archetype& archetypeToGetContainer,
			uint64_t addedComponentIndex
		);

		Archetype* GetArchetypeAfterRemoveComponent(Archetype& fromArchetype, TypeID removedComponentTypeID);

		template<typename T>
		inline Archetype* GetArchetypeAfterRemoveComponent(Archetype& fromArchetype)
		{
			return GetArchetypeAfterRemoveComponent(fromArchetype, Type<T>::GetID());
		}
	};
}