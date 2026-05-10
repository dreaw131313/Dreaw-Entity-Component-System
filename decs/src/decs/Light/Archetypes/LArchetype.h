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

		ArchetypeFilterRecord(IFilterContainerBase* filterContainer):
			m_FilterContainer(filterContainer),
			m_FilterTypeID(filterContainer != nullptr ? filterContainer->GetDataTypeID() : InvalidTypeID)
		{

		}
	};

	/// <summary>
	/// Represents class which can be used to as key in map for component type/tag or filter data
	/// </summary>
	class ArchetypeDataKey
	{
	public:
		TypeID m_TypeID = InvalidTypeID;
		IFilterContainerBase* m_FilterContainer = nullptr;

	public:
		ArchetypeDataKey() = default;

		inline bool IsFilterEdge() const noexcept
		{
			return m_FilterContainer != nullptr;
		}

		ArchetypeDataKey(TypeID typeID):
			m_TypeID(typeID)
		{

		}

		ArchetypeDataKey(IFilterContainerBase* container):
			m_TypeID(container != nullptr ? container->GetDataTypeID() : 0),
			m_FilterContainer(container)
		{

		}

		inline bool operator==(const ArchetypeDataKey& other) const noexcept
		{
			return m_TypeID == other.m_TypeID && m_FilterContainer == other.m_FilterContainer;
		}

		inline bool operator!=(const ArchetypeDataKey& other) const noexcept
		{
			return m_TypeID != other.m_TypeID || m_FilterContainer != other.m_FilterContainer;
		}
	};
}

template<>
struct std::hash<decs::light::ArchetypeDataKey>
{
public:
	std::size_t operator()(const decs::light::ArchetypeDataKey& v) const noexcept
	{
		if (v.m_FilterContainer != nullptr)
		{
			return std::hash<decs::light::IFilterContainerBase*>{}(v.m_FilterContainer);
		}
		else
		{
			return std::hash<decs::TypeID>{}(v.m_TypeID);
		}
	}
};

namespace decs::light
{

	class Archetype final
	{
		friend class light::Container;
		friend class light::ContainerIterator;
		friend class light::EntityData;
		friend class light::EntityManager;
		friend class light::ArchetypesMap;

		template<decs::light_component_or_filter_concept...>
		friend class light::Query;
		template<light_component_or_filter_concept...>
		friend class light::MultiQuery;
		template<light_component_or_filter_concept... ComponentsTypes>
		friend class light::IterationArchetypeContext;
		template<light_component_or_filter_concept...>
		friend class light::IterationContainerContext;
		template<typename Components, typename Tags>
		friend struct light::EntitySpawner;

	private:
		ecsMap<TypeID, uint32_t> m_TypeIDsIndexes{};
		ecsMap<ArchetypeDataKey, ArchetypeEdge> m_Edges{};

		ArchetypeEntityList m_Entities{};
		std::vector<ArchetypeTypeData> m_TypeData{};
		std::vector<ArchetypeFilterRecord> m_Filters{};

	public:
		Archetype();

		~Archetype();

		/// <summary>
		/// </summary>
		/// <returns>number of components + tags</returns>
		inline uint32_t GetComponentTagCount() const noexcept
		{
			return static_cast<uint32_t>(m_TypeData.size());
		}

		/// <summary>
		/// </summary>
		/// <returns>number of components + tags + filters</returns>
		inline size_t GetComponentTagFilterCount() const noexcept
		{
			return m_TypeData.size() + m_Filters.size();
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

		bool ContainComponentOrTagType(TypeID typeID) const;

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

		bool HasSameComponentsTagsAs(const Archetype& other) const;

		bool HasSameComponentsTagsFiltersAs(const Archetype& other) const;

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
			const uint32_t componentAndTagCount = GetComponentTagCount();

			if (static_cast<uint32_t>(group.Size()) != componentAndTagCount)
			{
				return false;
			}

			for (uint32_t i = 0; i < componentAndTagCount; i++)
			{
				if (!ContainComponentOrTagType(group[i]))
				{
					return false;
				}
			}

			return true;
		}

		template<light_component_or_filter_concept... ComponentTypes, tag_concept... TagTypes>
		bool IsArchetypeWithComponentsAndTags_Exactly(
			const LightComponentTypeGroup<ComponentTypes...> components,
			const TagTypeGroup<TagTypes...> tags
		) const noexcept
		{
			if ((sizeof...(ComponentTypes) + sizeof...(TagTypes)) != GetComponentTagCount())
			{
				return false;
			}

			for (uint32_t i = 0; i < components.Size(); i++)
			{
				if (!ContainComponentOrTagType(components[i]))
				{
					return false;
				}
			}

			for (uint32_t i = 0; i < tags.Size(); i++)
			{
				if (!ContainComponentOrTagType(tags[i]))
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
		std::optional<ArchetypeDataKey> IsRemoveAnyDataNeighbour(const Archetype& neighbour) const;

		/// <summary>
		/// 
		/// </summary>
		/// <param name="neighbour"></param>
		/// <returns>Archetype with larger number of componetns than this archetype</returns>
		std::optional<ArchetypeDataKey> IsAddAnyDataNeighbour(const Archetype& neighbour) const;

		inline bool HasAnyEdge(ArchetypeDataKey edgeKey) const noexcept
		{
			return m_Edges.contains(edgeKey);
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

		inline bool HasFilterWithType(TypeID filterTypeID) const
		{
			return  std::find_if(
				m_Filters.begin(),
				m_Filters.end(),
				[&](const ArchetypeFilterRecord& record)
			{
				return filterTypeID == record.m_FilterTypeID;
			}
			) != m_Filters.end();
		}

		template<filter_concept FilterType>
		size_t GetFilterIndex()
		{
			TYPE_ID_CONSTEXPR TypeID id = Type<FilterType>::ID();

			for (size_t idx = 0; idx < m_Filters.size(); id++)
			{
				if (m_Filters[idx].m_FilterTypeID == id)
				{
					return idx;
				}
			}
			return std::numeric_limits<size_t>::max();
		}

		IFilterContainerBase* GetFilterContainer(TypeID filterID) const
		{
			for (auto& filterRecord : m_Filters)
			{
				if (filterRecord.m_FilterTypeID == filterID)
				{
					return filterRecord.m_FilterContainer;
				}
			}
			return nullptr;
		}

		template<filter_concept FilterType>
		FilterContainer<FilterType>* GetFilterContainer() const
		{
			return check_cast<FilterContainer<FilterType>*>(GetFilterContainer(Type<FilterType>::ID()));
		}

	private:
		void AddFilter_WithoutCheckout(IFilterContainerBase* filter);

		void AddFilterInCorrectPlace(IFilterContainerBase& filter);

	#pragma endregion

	public:
		bool ContainComponentOrTagOrFilterType(TypeID typeID) const noexcept;

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

		void InitEmptyFromOther(const Archetype& other, FilterManager& filterManager);

		void ShrinkToFit();

	#pragma region EDGES
	private:
		void AddEdge(ArchetypeDataKey key, Archetype* archetype, EArchetypeEdgeType edgeType);

		template<light_component_or_filter_concept TComponent>
		ArchetypeEdge GetEdge() const
		{
			auto it = m_Edges.find(Type<TComponent>::ID());
			if (it != m_Edges.end())
			{
				return it->second;
			}
			return {};
		}

		ArchetypeEdge GetEdge(ArchetypeDataKey componentTypeID) const
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

		static bool MoveEntiyAfterFilterChange(
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
				|| m_ArchetypeConst->GetComponentTagCount() != rhs.m_ArchetypeConst->GetComponentTagCount()
				)
			{
				return false;
			}

			return m_ArchetypeConst->HasSameComponentsTagsAs(*rhs.m_ArchetypeConst)
				&& m_ArchetypeConst->HasSameFiltersAs(*rhs.m_ArchetypeConst)
				;
		}

		inline const Archetype* GetConstArchetype() const
		{
			return m_ArchetypeConst;
		}

		std::size_t CalculateHash() const noexcept
		{
			if (m_ArchetypeConst == nullptr || m_ArchetypeConst->GetComponentTagCount() == 0)
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


	class IFilterDataTuple : public RefCountedObject
	{
	public:
		virtual bool TestArchetype(const Archetype& archetpye)  const = 0;
	};

	using IFilterDataTupleHandle = TRefCountHandle<IFilterDataTuple>;

	template<typename... FilterTypes>
	class TFilterDataTuple final : public IFilterDataTuple
	{
	public:
		std::tuple<pure_type_t<FilterTypes>...> m_FiltersData{};

	public:
		TFilterDataTuple(FilterTypes&&... filterData):
			m_FiltersData(std::forward_as_tuple(std::forward<FilterTypes>(filterData)...))
		{

		}

		bool TestArchetype(const Archetype& archetype) const override
		{
			return (CompareFilterTypeData<FilterTypes>(archetype) && ...);
		}

	private:
		template<typename T>
		bool CompareFilterTypeData(const Archetype& archetype) const
		{
			FilterContainer<T>* filterContainer = archetype.GetFilterContainer<T>();
			if (filterContainer  == nullptr)
			{
				return false;
			}

			const T& data = std::get<T>(m_FiltersData);

			return filterContainer->m_Data == data;
		}
	};

	template<typename... FilterTypes>
	using TFilterDataTupleHandle = TRefCountHandle<TFilterDataTuple<FilterTypes...>>;
}

template<>
struct std::hash<decs::light::ArchetypeHasher>
{
	std::size_t operator()(const decs::light::ArchetypeHasher& archHasher)const
	{
		return archHasher.CalculateHash();
	}
};