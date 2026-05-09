#pragma once
#include "decs/Core/Core.h"
#include "decs/Core/Type.h"
#include "decs/Core/trait.h"
#include "decs/Core/Hash.h"
#include "decs/Core/check_cast.h"
#include "decs/Light/Component/PackedLightComponentContainer.h"
#include "decs/Light/Filter/LFilter.h"
#include "decs/Light/LEntityData.h"

#include "decs/Light/LightForward.h"

#include <optional>

namespace decs::light
{
	class Entity;
	class Archetype;
	struct ArchetypeHasher;
	template<typename Components, typename Tags>
	struct EntitySpawner;

	struct ArchetypeEntityList
	{
	public:
		ArchetypeEntityList() = default;

		~ArchetypeEntityList() = default;

		inline size_t Size() const noexcept
		{
			return m_Entities.size();
		}

		inline size_t Capacity() const noexcept
		{
			return m_Entities.capacity();
		}

		inline void ShrinkToFit()
		{
			m_Entities.shrink_to_fit();
		}

		inline std::span<EntityData* const> Data() const noexcept
		{
			return m_Entities;
		}

		inline bool Empty() const noexcept
		{
			return m_Entities.empty();
		}

		inline void Clear()
		{
			m_Entities.clear();
		}

		inline void Reserve(size_t capacity)
		{
			m_Entities.reserve(capacity);
		}

		inline void PushBack(EntityData* entityData)
		{
			DECS_ASSERT(entityData != nullptr, "entityData must not be nullptr!");
			m_Entities.push_back(entityData);
		}

		inline void PushBackUpdateIndex(EntityData* entityData)
		{
			DECS_ASSERT(entityData != nullptr, "entityData must not be nullptr!");
			entityData->m_IndexInArchetype = static_cast<uint32_t>(m_Entities.size());
			m_Entities.push_back(entityData);
		}

		inline void PopBack()
		{
			if (!m_Entities.empty())
			{
				m_Entities.pop_back();
			}
		}

		inline EntityData* Back() const
		{
			return m_Entities.back();
		}

		inline EntityData* Get(size_t index) const
		{
			return m_Entities[index];
		}

		inline bool RemoveSwapBack(size_t index)
		{
			if (index >= m_Entities.size())
			{
				return false;
			}

			if (index < (m_Entities.size() - 1))
			{
				m_Entities[index] = m_Entities.back();
			}
			m_Entities.pop_back();

			return true;
		}

		inline bool RemoveSwapBack_UpdateEntityIndex(size_t index)
		{
			if (index >= m_Entities.size())
			{
				return false;
			}

			if (index < (m_Entities.size() - 1))
			{
				auto backEntity = m_Entities.back();
				backEntity->m_IndexInArchetype = static_cast<uint32_t>(m_Entities.size());
				m_Entities[index] = m_Entities.back();
			}
			m_Entities.pop_back();

			return true;
		}

	private:
		std::vector<EntityData*> m_Entities{};
	};

	struct ArchetypeTypeData
	{
	public:
		IPackedLightComponentContainer* m_PackedContainer = nullptr;
		TypeID m_TypeID = std::numeric_limits<TypeID>::max();

	public:
		ArchetypeTypeData()
		{

		}

		ArchetypeTypeData(
			TypeID typeID,
			IPackedLightComponentContainer* packedContainer
		):
			m_PackedContainer(packedContainer), m_TypeID(typeID)
		{

		}

		inline bool IsTag() const
		{
			return m_PackedContainer == nullptr;
		}
	};

	struct ArchetypeEdge
	{
	public:
		Archetype* m_Archetype = nullptr;
		EArchetypeEdgeType m_EdgeType = EArchetypeEdgeType::Add;

	public:
		ArchetypeEdge()
		{

		}

		ArchetypeEdge(Archetype* archetype, EArchetypeEdgeType edgeType):
			m_Archetype(archetype), m_EdgeType(edgeType)
		{

		}

		inline bool IsValid() const
		{
			return m_Archetype != nullptr;
		}
	};

	struct ArchetypeFilterRecord
	{
	public:
		IFilterContainerBase* m_FilterContainer = nullptr;
		TypeID m_FilterTypeID = InvalidTypeID;

	public:
		ArchetypeFilterRecord() = default;

		ArchetypeFilterRecord(IFilterContainerBase* filterContainer, TypeID filterTypeID):
			m_FilterContainer(filterContainer),
			m_FilterTypeID(filterTypeID)
		{
			filterContainer->IncrementUseCount();
		}
	};

	class Archetype final
	{
		friend class light::Container;
		friend class light::ContainerIterator;
		friend class light::EntityData;
		friend class light::EntityManager;
		friend class light::ArchetypesMap;

		template<decs::TLightComponentConcept...>
		friend class light::Query;
		template<TLightComponentConcept...>
		friend class light::MultiQuery;
		template<TLightComponentConcept... ComponentsTypes>
		friend class light::IterationArchetypeContext;
		template<TLightComponentConcept...>
		friend class light::IterationContainerContext;
		template<typename Components, typename Tags>
		friend struct light::EntitySpawner;

	private:
		ecsMap<TypeID, uint32_t> m_TypeIDsIndexes;
		ecsMap<TypeID, ArchetypeEdge> m_Edges;

		ArchetypeEntityList m_Entities{};
		std::vector<ArchetypeTypeData> m_TypeData{};

		std::vector<ArchetypeFilterRecord> m_Filters{};
		std::unordered_map<IFilterContainerBase*, ArchetypeEdge> m_FilterEdges{};

	public:
		Archetype();

		~Archetype();

		inline uint32_t GetTypeCount() const noexcept
		{
			return static_cast<uint32_t>(m_TypeData.size());
		}

		/// <summary>
		/// Return number of components + tags + filters
		/// </summary>
		/// <returns></returns>
		inline uint32_t GetComponentTagFilterCount() const noexcept
		{
			return static_cast<uint32_t>(m_TypeData.size() + m_Filters.size());
		}

		inline TypeID GetTypeID(uint64_t index) const
		{
			return m_TypeData[index].m_TypeID;
		}

		inline uint64_t EntityCount() const noexcept
		{
			return m_Entities.Size();
		}

		inline const ArchetypeEntityList& GetEntities() const noexcept
		{
			return m_Entities;
		}

		inline float GetLoadFactor()const
		{
			if (m_Entities.Capacity() == 0) return 1.f;
			return (float)m_Entities.Size() / (float)m_Entities.Capacity();
		}

		inline std::span<const ArchetypeTypeData> GetComponentAndTagRecords() const noexcept
		{
			return m_TypeData;
		}

		bool ContainType(TypeID typeID) const;

		inline bool HasComponentType(TypeID typeID) const
		{
			auto it = m_TypeIDsIndexes.find(typeID);
			if (it != m_TypeIDsIndexes.end())
			{
				auto& typeData = m_TypeData[it->second];
				return !typeData.IsTag();
			}
			return false;
		}

		uint32_t FindTypeIndex(TypeID typeID) const;

		template<typename T>
		inline uint32_t FindTypeIndex() const
		{
			return FindTypeIndex(Type<T>::ID());
		}

		inline bool HasTag(TypeID tagType) const
		{
			uint32_t index = FindTypeIndex(tagType);
			if (index == std::numeric_limits<uint32_t>::max())
			{
				return false;
			}

			return m_TypeData[index].IsTag();
		}

		template<tag_concept TTag>
		inline bool HasTag() const
		{
			return HasTag(Type<TTag>::ID());
		}

		inline bool IsTypeTag(uint32_t typeIndex) const
		{
			return m_TypeData[typeIndex].IsTag();
		}

		bool HasSameTypesAs(const Archetype& archetype)  const;

		/// <summary>
		/// if types.size() is different thant archetype type count returns false.
		/// </summary>
		/// <param name="types"></param>
		/// <returns></returns>
		bool HasTypes_Exactly(const std::vector<TypeID>& types) const;

		/// <summary>
		/// if group.Size() is different thant archetype type count returns false.
		/// </summary>
		/// <typeparam name="...Types"></typeparam>
		/// <param name="types"></param>
		/// <returns></returns>
		template<typename... Types>
		bool HasTypes_Exactly(const TypeGroup<Types...>& group) const
		{
			const uint32_t componentAndTagCount = GetTypeCount();

			if (static_cast<uint32_t>(group.Size()) != componentAndTagCount)
			{
				return false;
			}

			for (uint32_t i = 0; i < componentAndTagCount; i++)
			{
				if (!ContainType(group[i]))
				{
					return false;
				}
			}

			return true;
		}

		template<TLightComponentConcept... ComponentTypes, tag_concept... TagTypes>
		bool IsArchetypeWithComponentsAndTags_Exactly(
			const LightComponentTypeGroup<ComponentTypes...> components,
			const TagTypeGroup<TagTypes...> tags
		) const noexcept
		{
			if ((sizeof...(ComponentTypes) + sizeof...(TagTypes)) != GetTypeCount())
			{
				return false;
			}

			for (uint32_t i = 0; i < components.Size(); i++)
			{
				if (!ContainType(components[i]))
				{
					return false;
				}
			}

			for (uint32_t i = 0; i < tags.Size(); i++)
			{
				if (!ContainType(tags[i]))
				{
					return false;
				}
			}

			return true;
		}

		/// <summary>
		/// 
		/// </summary>
		/// <param name="neighbour">Archetype with smaller number of componetnts than this archetype</param>
		/// <returns></returns>
		std::optional<TypeID> IsRemoveComponentNeighbour(const Archetype& neighbour) const;

		/// <summary>
		/// 
		/// </summary>
		/// <param name="neighbour"></param>
		/// <returns>Archetype with larger number of componetns than this archetype</returns>
		std::optional<TypeID> IsAddComponentNeighbour(const Archetype& neighbour) const;

		inline bool HasAnyEdge(TypeID toTypeID) const noexcept
		{
			return m_Edges.contains(toTypeID);
		}

	#pragma region FILTERS
	public:
		size_t GetFilterCount() const noexcept
		{
			return m_Filters.size();
		}

		std::span<const ArchetypeFilterRecord> GetFilters() const noexcept
		{
			return m_Filters;
		}

		bool HasSameFiltersAs(const Archetype& other) const;

		inline bool ContainFilter(const IFilterContainerBase* filter) const
		{
			return  std::find_if(
				m_Filters.begin(),
				m_Filters.end(),
				[&](const ArchetypeFilterRecord& record)
			{
				return filter == record.m_FilterContainer;
			}
			) != m_Filters.end();
		}

		/// <summary>
		/// 
		/// </summary>
		/// <param name="neighbour">Archetype with smaller number of filters than this archetype</param>
		/// <returns></returns>
		std::optional<IFilterContainerBase*> IsRemoveFilterNeighbour(const Archetype& neighbour) const;

		/// <summary>
		/// 
		/// </summary>
		/// <param name="neighbour">Archetype with larger number of filters than this archetype</param>
		/// <returns></returns>
		std::optional<IFilterContainerBase*> IsAddFilterNeighbour(const Archetype& neighbour) const;

	private:
		void AddFilterEdge(IFilterContainerBase* filter, Archetype* archetype, EArchetypeEdgeType edgeType);

	#pragma endregion

	private:

		template<typename TComponentType>
		PackedLightComponentContainer<TComponentType>* GetTypePackedContainer() const
		{
			uint32_t compIdx = FindTypeIndex<TComponentType>();
			if (compIdx == std::numeric_limits<uint32_t>::max())
			{
				return nullptr;
			}

			auto& typeData = m_TypeData[compIdx];

			return ::decs::check_cast<PackedLightComponentContainer<TComponentType>*>(typeData.m_PackedContainer);
		}

		void ClearEntityDataAndComponents();

		void AddTypeData_WithoutCheck(
			TypeID typeID,
			IPackedLightComponentContainer* packedContainer
		);

		void AddEntityData(EntityData* entityData);

		bool AddEntityDataAndDefaultComponents(EntityData* entityData);

		void RemoveSwapBackEntityData(size_t index);

		void RemoveSwapBackEntity(size_t index);

		void ReserveSpaceInArchetype(size_t desiredCapacity);

		void Reset();

		void InitEmptyFromOther(const Archetype& other);

		void ShrinkToFit();

	#pragma region EDGES
	private:
		void AddEdge(TypeID componentTypeID, Archetype* archetype, EArchetypeEdgeType edgeType);

		template<TLightComponentConcept TComponent>
		ArchetypeEdge GetEdge() const
		{
			auto it = m_Edges.find(Type<TComponent>::ID());
			if (it != m_Edges.end())
			{
				return it->second;
			}
			return {};
		}

		ArchetypeEdge GetEdge(TypeID componentTypeID) const
		{
			auto it = m_Edges.find(componentTypeID);
			if (it != m_Edges.end())
			{
				return it->second;
			}
			return {};
		}

	#pragma endregion

	#pragma region STATICS
	private:

		static bool MoveEntityAfterAddType(
			Archetype& fromArchetype,
			Archetype& toArchetype,
			uint64_t entityIndex,
			TypeID addedComponentTypeID
		);

		static bool MoveEntityAfterRemoveType(
			Archetype& fromArchetype,
			Archetype& toArchetype,
			uint64_t entityIndex,
			TypeID removedComponentTypeID
		);

		static bool MoveEnttiyAfterFilterChange(
			Archetype& fromArchetype,
			Archetype& toArchetype,
			uint64_t entityIndex
		);


	#pragma endregion

	};

	struct ArchetypeHasher final
	{
	public:
		ArchetypeHasher() = default;

		ArchetypeHasher(const Archetype* archetype):
			m_ArchetypeConst(archetype)
		{

		}

		bool operator==(const ArchetypeHasher& rhs)const
		{
			if (m_ArchetypeConst == rhs.m_ArchetypeConst)
			{
				return true;
			}
			if (m_ArchetypeConst == nullptr || rhs.m_ArchetypeConst == nullptr
				|| m_ArchetypeConst->GetTypeCount() != rhs.m_ArchetypeConst->GetTypeCount()
				)
			{
				return false;
			}

			return m_ArchetypeConst->HasSameTypesAs(*rhs.m_ArchetypeConst)
				&& m_ArchetypeConst->HasSameFiltersAs(*rhs.m_ArchetypeConst)
				;
		}

		inline const Archetype* GetConstArchetype() const
		{
			return m_ArchetypeConst;
		}

		std::size_t CalculateHash() const noexcept
		{
			if (m_ArchetypeConst == nullptr || m_ArchetypeConst->GetTypeCount() == 0)
			{
				return 0;
			}

			auto componentTagTypeRecords = m_ArchetypeConst->GetComponentAndTagRecords();
			std::size_t finalHash = std::hash<TypeID>{}(componentTagTypeRecords[0].m_TypeID);
			for (auto& typeRecord : componentTagTypeRecords)
			{
				finalHash = hash::Combine(finalHash, std::hash<TypeID>{}(typeRecord.m_TypeID));
			}

			auto filters = m_ArchetypeConst->GetFilters();
			for (auto& filterRecord : filters)
			{
				finalHash = hash::Combine(finalHash, std::hash<TypeID>{}(filterRecord.m_FilterTypeID));
				finalHash = hash::Combine(finalHash, std::hash<TypeID>{}(filterRecord.m_FilterContainer->GetDataHash()));
			}

			return finalHash;
		}

	private:
		const Archetype* m_ArchetypeConst = nullptr;
	};
}

template<>
struct std::hash<decs::light::ArchetypeHasher>
{
	std::size_t operator()(const decs::light::ArchetypeHasher& archHasher)const
	{
		return archHasher.CalculateHash();
	}
};