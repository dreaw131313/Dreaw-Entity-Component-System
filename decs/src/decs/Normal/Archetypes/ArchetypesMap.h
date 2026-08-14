#pragma once

#include <memory>

#include "decs/Core/Type.h"
#include "decs/Core/ArchetypesCore.h"
#include "decs/Normal/Component/ComponentContextsManager.h"

#include "Archetype.h"

namespace decs
{
	struct ArchetypeGroup
	{
	public:
		ecsVector<Archetype*> Archetypes;

	public:
		ArchetypeGroup() = default;

		inline size_t GetArchetypeCount() const noexcept
		{
			return Archetypes.size();
		}
	};

	class ArchetypesGroupByOneType
	{
	public:
		ArchetypesGroupByOneType(TypeID mainTypeID) :
			m_MainTypeID(mainTypeID)
		{

		}

		inline TypeID GetMainTypeID() const noexcept
		{
			return m_MainTypeID;
		}

		inline Archetype* GetMainTypeArchetype() const
		{
			return m_MainTypeArchetype;
		}

		inline uint64_t GetArchetypesCount() const
		{
			return m_ArchetypesCount;
		}

		inline uint32_t MaxComponentsCount() const
		{
			return (uint32_t)m_Groups.size();
		}

		void AddArchetype(Archetype* archetype);

		std::span<const Archetype* const> GetArchetypesWithTypeCount(uint64_t componentsCount) const
		{
			uint64_t groupIndex = componentsCount - 1;
			if (groupIndex >= m_Groups.size())
			{
				return {};
			}
			auto& group = m_Groups[groupIndex];
			return group.Archetypes;
		}

		const ArchetypeGroup* GetGroupWithTypeCount(uint64_t componentsCount) const
		{
			uint64_t groupIndex = componentsCount - 1;
			if (groupIndex >= m_Groups.size())
			{
				return nullptr;
			}
			return &m_Groups[groupIndex];
		}

		inline Archetype* GetSingleComponentArchetype() const
		{
			if (m_Groups.empty())
			{
				return nullptr;
			}

			auto& group = m_Groups.front();
			return group.Archetypes.empty() ? nullptr : group.Archetypes.front();
		}

		template<typename Callable>
		void IterateOverAllArchetypes(Callable&& func)
		{
			uint64_t archetypesGroupCount = m_Groups.size();
			for (uint64_t groupIdx = 0; groupIdx < archetypesGroupCount; groupIdx++)
			{
				auto& group = m_Groups[groupIdx];
				for (auto& archetype : group.Archetypes)
				{
					func(archetype);
				}
			}
		}

	private:
		TypeID m_MainTypeID = std::numeric_limits<TypeID>::max();
		Archetype* m_MainTypeArchetype = nullptr;
		ecsVector<ArchetypeGroup> m_Groups;
		uint64_t m_ArchetypesCount = 0;
	};

	class ArchetypesMap
	{
		friend class Container;
		friend class ContainerIterator;
		template<typename>
		friend class ContainerSerializer;
		friend class ContainerSerializerComplex;
		template<component_concept...>
		friend class Query;
		template<component_concept...>
		friend class IterationContainerContext;

	public:
		ArchetypesMap()
		{

		}

		ArchetypesMap(uint64_t archetypesVectorChunkSize, uint64_t archetypeGroupsVectorChunkSize);

		~ArchetypesMap();

		inline uint64_t GetArchetypesCount() const noexcept
		{
			return m_Archetypes.Size();
		}

		[[nodiscard]] inline uint64_t EmptyArchetypesCount() const
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

		inline uint64_t GetMaxComponentTagCount() const
		{
			return m_MaxComponentTagCount;
		}

		void ShrinkArchetypesToFit();

		void ShrinkArchetypesToFit(ArchetypesShrinkToFitState& state, const ArchetypesShrinkToFitConfig& config);

		template<component_concept ComponentType>
		void UpdateOrderInAllArchetypesWithComponentType()
		{
			auto it = m_ArchetypesGroupedByOneType.find(Type<ComponentType>::ID());
			if (it != m_ArchetypesGroupedByOneType.end())
			{
				it->second->IterateOverAllArchetypes([] (Archetype* arch)
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
				it->second->IterateOverAllArchetypes([] (Archetype* arch)
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

		void ClearEntityDataAndComponents();

		template<typename FuncType>
		void IterateOverArchetypes_Forward(FuncType&& func) const
		{
			for (size_t chunkIdx = 0; chunkIdx < m_Archetypes.ChunkCount(); chunkIdx++)
			{
				const size_t chunkSize = m_Archetypes.GetChunkSize(chunkIdx);
				auto chunk = m_Archetypes.GetChunk(chunkIdx);

				for (size_t i = 0; i < chunkSize; i++)
				{
					const Archetype& archetype = chunk[i];
					func(archetype);
				}
			}
		}

	private:
		ecsHashMap<TypeID, ArchetypesGroupByOneType*> m_ArchetypesGroupedByOneType{};
		ecsHashMap<ArchetypeHasher, Archetype*> m_HashedArchetypes{};

		TChunkedVector<Archetype> m_Archetypes{ 100 };
		TChunkedVector<ArchetypesGroupByOneType> m_ArchetypesGroupsByOneTypeAllocator{ 100 };

		uint32_t m_MaxComponentTagCount = 0;

	private:
		void MakeArchetypeEdges_4(Archetype& archetype);

		void AddArchetypeToCorrectContainers(Archetype& archetype);

		/// <summary>
		/// Can be used to get single tags components
		/// </summary>
		/// <param name="typeID"></param>
		/// <returns></returns>
		inline Archetype* GetSingleComponentArchetype(TypeID typeID) const
		{
			auto it = m_ArchetypesGroupedByOneType.find(typeID);
			return it != m_ArchetypesGroupedByOneType.end() ? it->second->GetMainTypeArchetype() : nullptr;
		}

		template<component_concept ComponentType>
		Archetype* GetSingleComponentArchetype() const
		{
			return GetSingleComponentArchetype(Type<ComponentType>::ID());
		}

		inline ArchetypesGroupByOneType* GetArchetypesGroup(TypeID id)
		{
			ArchetypesGroupByOneType*& group = m_ArchetypesGroupedByOneType[id];
			if (group == nullptr)
			{
				group = &m_ArchetypesGroupsByOneTypeAllocator.EmplaceBack(id);
			}
			return group;
		}

		inline ArchetypesGroupByOneType* GetArchetypesGroupWithoutCreating(TypeID id) const
		{
			auto it = m_ArchetypesGroupedByOneType.find(id);
			if (it == m_ArchetypesGroupedByOneType.end())
			{
				return nullptr;
			}
			return it->second;
		}

		Archetype* FindMatchingArchetype(const Archetype& toArchetype);

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
		Archetype* CreateSingleComponentArchetype(TypeID componentTypeID, IComponentContext* componentContext);

		template<component_concept T>
		inline Archetype* GetArchetypeAfterAddComponent(Archetype& toArchetype)
		{
			TYPE_ID_CONSTEXPR TypeID addedComponentTypeID = Type<T>::ID();
			auto edge = toArchetype.GetEdge(addedComponentTypeID);

			if (edge.IsValid() && edge.m_EdgeType == EArchetypeEdgeType::Add)
			{
				return edge.m_Archetype;
			}
			return nullptr;
		}

		Archetype* GetOrCreateArchetypeAfterAddComponent(const Archetype& toArchetype, TypeID componentTypeID, IComponentContext* componentContext);

		Archetype* CreateArchetypeAfterAddComponent(const Archetype& toArchetype, TypeID componentTypeID, IComponentContext* componentContext);

		Archetype* GetOrCreateArchetypeAfterRemoveComponent(const Archetype& fromArchetype, TypeID removedComponentTypeID);

		Archetype* GetArchetypeAfterAddTag(const Archetype& toArchetype, TypeID tagType);

		Archetype* GetArchetypeAfterRemoveTag(const Archetype& fromArchetype, TypeID tagType);

		Archetype* CreateSingleTagArchetype(TypeID componentTypeID);

		void AddTypeDataAfterRemoveComponent(const Archetype& fromArchetype, Archetype& toArchetype, TypeID compType);

		void AddTypeDataAfterAddComponent(const Archetype& baseArchetype, Archetype& toArchetype, TypeID componentTypeID, IComponentContext* addedComponentContext);

	};
}