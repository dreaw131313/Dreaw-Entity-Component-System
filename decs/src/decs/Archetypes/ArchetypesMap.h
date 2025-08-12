#pragma once

#include <memory>

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

		ArchetypesShrinkToFitState(uint64_t archetypesToShrinkInOneCall, float maxArchetypeLoadFactor):
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

	struct ArchetypeGroup
	{
	public:
		std::vector<Archetype*> Archetypes;

	public:
		ArchetypeGroup() = default;

	};

	class ArchetypesGroupByOneType
	{
	public:
		ArchetypesGroupByOneType(
			TChunkedVector<ArchetypeGroup>& archetypeGroupAllocator,
			TypeID mainTypeID
		):
			m_ArchetypeGroupAllocator(archetypeGroupAllocator),
			m_MainTypeID(mainTypeID)
		{

		}

		inline uint64_t ArchetypesCount() const { return m_ArchetypesCount; }
		inline uint32_t MaxComponentsCount() const { return (uint32_t)m_Groups.size(); }

		void AddArchetype(Archetype* archetype)
		{
			m_ArchetypesCount += 1;
			uint64_t archetypesCount = archetype->GetComponentAndTagCount();
			if (archetypesCount > m_Groups.size())
			{
				m_Groups.resize(archetypesCount);
			}

			auto& archetypeGroup = m_Groups[archetypesCount - 1];
			if (archetypeGroup == nullptr)
			{
				archetypeGroup = &m_ArchetypeGroupAllocator.EmplaceBack();
			}
			archetypeGroup->Archetypes.push_back(archetype);
		}

		const std::vector<Archetype*>* GetArchetypesWithComponentsCount(uint64_t componentsCount) const
		{
			uint64_t groupIndex = componentsCount - 1;
			if (groupIndex >= m_Groups.size())
			{
				return nullptr;
			}
			auto group = m_Groups[groupIndex];
			if (group == nullptr)
			{
				return nullptr;
			}
			return &group->Archetypes;
		}

		inline Archetype* GetSingleComponentArchetype() const
		{
			if (m_Groups.empty())
			{
				return nullptr;
			}

			auto group = m_Groups.front();
			if (group == nullptr || group->Archetypes.empty())
			{
				return nullptr;
			}
			return group->Archetypes.front();
		}

		template<typename Callable>
		void IterateOverAllArchetypes(Callable&& func)
		{
			uint64_t archetypesGroupCount = m_Groups.size();
			for (uint64_t groupIdx = 0; groupIdx < archetypesGroupCount; groupIdx++)
			{
				auto group = m_Groups[groupIdx];
				if (group != nullptr)
				{
					auto& groupArchetypes = group->Archetypes;
					uint64_t archetypeCount = groupArchetypes.size();
					for (uint64_t archIdx = 0; archIdx < archetypeCount; archIdx++)
					{
						auto archetype = groupArchetypes[archIdx];
						func(archetype);
					}
				}
			}
		}

	private:
		TChunkedVector<ArchetypeGroup>& m_ArchetypeGroupAllocator;

		TypeID m_MainTypeID = std::numeric_limits<TypeID>::max();

		std::vector<ArchetypeGroup*> m_Groups;
		uint64_t m_ArchetypesCount = 0;
	};

	class ArchetypesMap
	{
		friend class Container;
		friend class ContainerIterator;
		template<typename>
		friend class ContainerSerializer;
		friend class ContainerSerializerComplex;
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

		template<typename TComponent>
		void UpdateOrderInAllArchetypesWithComponentType()
		{
			auto it = m_ArchetypesGroupedByOneType.find(Type<TComponent>::ID());
			if (it != m_ArchetypesGroupedByOneType.end())
			{
				it->second->IterateOverAllArchetypes([](Archetype* arch)
				{
					arch->UpdateOrderOfComponentContexts();
				});
			}
		}

		void UpdateOrderInAllArchetypesWithComponentType(TypeID typeID)
		{
			auto it = m_ArchetypesGroupedByOneType.find(typeID);
			if (it != m_ArchetypesGroupedByOneType.end())
			{
				it->second->IterateOverAllArchetypes([](Archetype* arch)
				{
					arch->UpdateOrderOfComponentContexts();
				});
			}
		}

		template<typename Callable>
		void IterateOverArchetypesWithType(TypeID componentType, Callable&& func)
		{
			auto groupedArchetypesIt = m_ArchetypesGroupedByOneType.find(componentType);
			if (groupedArchetypesIt == m_ArchetypesGroupedByOneType.end())
			{
				return;
			}
			groupedArchetypesIt->second->IterateOverAllArchetypes(func);
		}

		template<typename Callable>
		void IterateOverArchetypes(Callable&& func)
		{
			int64_t chunkCount = static_cast<int64_t>(m_Archetypes.ChunkCount());
			for (int64_t chunkIdx = chunkCount - 1; chunkIdx >= 0; chunkIdx--)
			{
				auto chunk = m_Archetypes.GetChunk(chunkIdx);
				int64_t elementCount = m_Archetypes.GetChunkSize(chunkIdx);

				for (int64_t elementIdx = elementCount - 1; elementIdx >= 0; elementIdx--)
				{
					func(&chunk[elementIdx]);
				}
			}
		}

		void FullClear();

		void ClearEntityDataAndComponents();

	private:
		TChunkedVector<Archetype> m_Archetypes = { 100 };
		TChunkedVector<ArchetypeGroup> m_ArchetrypesGroupsAllocator = { 100 };
		TChunkedVector<ArchetypesGroupByOneType> m_ArchetrypesGroupsByOneTypeVector = { 100 };

		ecsMap<TypeID, Archetype*> m_SingleComponentArchetypes = {};
		std::vector<std::vector<Archetype*>> m_ArchetypesGroupedByComponentsCount = {};

		ecsMap<TypeID, ArchetypesGroupByOneType*> m_ArchetypesGroupedByOneType;

		// UTILITY
	private:
		void MakeArchetypeEdges(Archetype& archetype);

		void AddArchetypeToCorrectContainers(Archetype& archetype, bool bTryAddToSingleComponentsMap = true);

		/// <summary>
		/// Can be used to get single tags components
		/// </summary>
		/// <param name="typeID"></param>
		/// <returns></returns>
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
				group = &m_ArchetrypesGroupsByOneTypeVector.EmplaceBack(m_ArchetrypesGroupsAllocator, id);
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
			ComponentContextsManager* componentContextsManager
		);

		void AddArchetypeToGroups(Archetype* arch)
		{
			uint64_t componentsCount = arch->GetComponentAndTagCount();
			for (uint64_t i = 0; i < componentsCount; i++)
			{
				const TypeID& id = arch->GetTypeID(i);
				ArchetypesGroupByOneType* group = GetArchetypesGroup(id);
				group->AddArchetype(arch);
			}
		}

		// CREATING ARCHETYPES
	private:
		template<typename TComponent>
		Archetype* GetSingleComponentArchetype()
		{
			TYPE_ID_CONSTEXPR TypeID typeID = Type<TComponent>::ID();
			auto it = m_SingleComponentArchetypes.find(typeID);

			return it != m_SingleComponentArchetypes.end() ? it->second : nullptr;
		}

		Archetype* CreateSingleComponentArchetype(TypeID componentTypeID, ComponentContextBase* componentContext);

		template<typename T>
		inline Archetype* GetArchetypeAfterAddComponent(Archetype& toArchetype)
		{
			TYPE_ID_CONSTEXPR TypeID addedComponentTypeID = Type<T>::ID();
			auto edge = toArchetype.GetEdge(addedComponentTypeID);

			if (edge.IsValid() && edge.m_EdgeType == EComponentEdgeType::Add)
			{
				return edge.m_Archetype;
			}
			return nullptr;
		}

		Archetype* CreateArchetypeAfterAddComponent(const Archetype& toArchetype, TypeID componentTypeID, ComponentContextBase* componentContext);

		Archetype* GetArchetypeAfterRemoveComponent(const Archetype& fromArchetype, TypeID removedComponentTypeID);

		Archetype* GetArchetypeAfterAddTag(const Archetype& toArchetype, TypeID tagType);

		Archetype* GetArchetypeAfterRemoveTag(const Archetype& fromArchetype, TypeID tagType);

		Archetype* CreateSingleTagArchetype(TypeID componentTypeID);

		void AddTypeDataAfterRemoveComponent(const Archetype& fromArchetype,Archetype& toArchetype, TypeID compType);

		void AddTypeDataAfterAddComponent(const Archetype& baseArchetype, Archetype& toArchetype,TypeID componentTypeID, ComponentContextBase* addedComponentContext);
	};
}