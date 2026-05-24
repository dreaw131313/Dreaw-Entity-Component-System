#pragma once
#include "decs/Core/Core.h"
#include "decs/Core/Type.h"
#include "decs/Core/trait.h"
#include "decs/Core/Hash.h"
#include "decs/Core/check_cast.h"

#include "decs/Normal/Component/ComponentContextsManager.h"
#include "decs/Normal/Component/PackedComponentContainer.h"
#include "decs/Normal/Component/StableComponentContainer.h"
#include "decs/Normal/EntityData.h"

#include <optional>

namespace decs
{
	struct Entity;
	class Archetype;

	struct ArchetypeEntityRecord
	{
	public:
		EntityData* m_EntityData = nullptr;
		bool m_bEnabled = false;

	public:
		inline bool IsValid() const noexcept
		{
			return m_EntityData != nullptr;
		}

		inline bool IsValidAndEnabled() const noexcept
		{
			return m_EntityData != nullptr && m_bEnabled;
		}
	};

	class ArchetypeEntityDataStorage
	{
	public:
		inline size_t GetSize() const noexcept
		{
			return m_EntityData.size();
		}

		inline size_t GetCapacity() const noexcept
		{
			return m_EntityData.capacity();
		}

		void Reserve(size_t size)
		{
			m_EntityData.reserve(size);
			m_Flags.reserve(size);
		}

		void ShrinkToFit()
		{
			m_EntityData.shrink_to_fit();
			m_Flags.shrink_to_fit();
		}

		inline float GetLoadFactor() const noexcept
		{
			if (m_EntityData.capacity() == 0) return 1.f;
			return (float)m_EntityData.size() / (float)m_EntityData.capacity();
		}

		inline std::span<const uint8_t> GetFlags() const noexcept
		{
			return m_Flags;
		}

		inline std::span<EntityData* const> GetEntities() const noexcept
		{
			return m_EntityData;
		}

		inline const ecsVector<uint8_t>& GetFlagsVector() const noexcept
		{
			return m_Flags;
		}

		inline const ecsVector<EntityData*>& GetEntitiesVector() const noexcept
		{
			return m_EntityData;
		}

		inline ArchetypeEntityRecord GetEntityRecord(size_t index) const noexcept
		{
			return { m_EntityData[index], m_Flags[index] != 0 };
		}

		inline std::pair<EntityData*, bool> GetBackRecord() const noexcept
		{
			if (m_EntityData.empty())
			{
				return {};
			}
			return { m_EntityData.back(), m_Flags.back() != 0 };
		}

		inline bool GetEnabled(size_t index) const noexcept
		{
			return m_Flags[index] != 0;
		}

		inline EntityData* GetEntity(size_t index) const noexcept
		{
			return m_EntityData[index];
		}

		inline void SetEnabled(size_t index, bool bEnabled)
		{
			m_Flags[index] = bEnabled && m_EntityData[index] != nullptr;
		}

		inline void SetEntityRecord(size_t index, EntityData* entity, bool bEnabled)
		{
			m_EntityData[index] = entity;
			m_Flags[index] = bEnabled && entity != nullptr;
		}

		/// <summary>
		/// Entity is not checked if its nullptr
		/// </summary>
		/// <param name="index"></param>
		/// <param name="entity"></param>
		/// <param name="bEnabled"></param>
		inline void SetEntityRecord_UpdateEntityIndex(size_t index, EntityData* entity, bool bEnabled)
		{
			entity->m_IndexInArchetype = static_cast<uint32_t>(index);
			m_EntityData[index] = entity;
			m_Flags[index] = bEnabled;
		}

		inline void PushBack(EntityData* entity, bool bEnabled)
		{
			m_EntityData.push_back(entity);
			m_Flags.push_back(bEnabled && entity != nullptr);
		}

		/// <summary>
		/// Entity is not checked if its nullptr
		/// </summary>
		/// <param name="entity"></param>
		/// <param name="bEnabled"></param>
		inline void PushBack_UpdateEntityIndex(EntityData* entity, bool bEnabled)
		{
			entity->m_IndexInArchetype = static_cast<uint32_t>(m_EntityData.size());
			m_EntityData.push_back(entity);
			m_Flags.push_back(bEnabled);
		}

		inline void PopBack()
		{
			m_EntityData.pop_back();
			m_Flags.pop_back();
		}

		void RemoveSwapBack(size_t index, bool bUpdateEntityIndex)
		{
			if (m_EntityData.empty() || index >= m_EntityData.size())
			{
				return;
			}
			size_t lastIndex = m_EntityData.size() - 1;
			if (index < lastIndex)
			{
				EntityData* backEntityData = m_EntityData.back();
				m_EntityData[index] = backEntityData;
				m_Flags[index] = m_Flags.back();

				if (bUpdateEntityIndex && backEntityData != nullptr)
				{
					backEntityData->m_IndexInArchetype = static_cast<uint32_t>(index);
				}
			}
			m_EntityData.pop_back();
			m_Flags.pop_back();
		}

		void Clear()
		{
			m_EntityData.clear();
			m_Flags.clear();
		}

		void InvalidateRecord(size_t index)
		{
			m_EntityData[index] = nullptr;
			m_Flags[index] = false;
		}

		inline bool IsActive(size_t index) const noexcept
		{
			return m_Flags[index];
		}

		inline bool IsValideAndActive(size_t index) const noexcept
		{
			return m_Flags[index] && m_EntityData[index] != nullptr;
		}

	private:
		ecsVector<EntityData*> m_EntityData{};
		ecsVector<uint8_t> m_Flags{};
	};

	struct ArchetypeTypeData
	{
	public:
		TypeID m_TypeID = std::numeric_limits<TypeID>::max();
		IPackedComponentContainer* m_PackedContainer = nullptr;
		IComponentContext* m_ComponentContext = nullptr;
		IStableComponentContainer* m_StableContainer = nullptr;

	public:
		ArchetypeTypeData()
		{

		}

		ArchetypeTypeData(
			TypeID typeID,
			IPackedComponentContainer* packedContainer,
			IComponentContext* componentContext,
			IStableComponentContainer* stableContainer
		):
			m_TypeID(typeID), m_PackedContainer(packedContainer), m_ComponentContext(componentContext), m_StableContainer(stableContainer)
		{

		}

		inline bool IsTag() const
		{
			return m_PackedContainer == nullptr
				|| m_ComponentContext == nullptr
				|| m_StableContainer == nullptr
				;
		}
	};

	template<typename ComponentType>
	struct TArchetypeTypeData
	{
	public:
		using PackedContainerType = PackedStableComponentContainer<ComponentType>;
		using StableContainerType = StableComponentContainer<ComponentType>;
		using ComponentContextType = ComponentContext<ComponentType>;

	public:
		PackedStableComponentContainer<ComponentType>* m_PackedContainer = nullptr;
		StableComponentContainer<ComponentType>* m_StableContainer = nullptr;
		ComponentContext<ComponentType>* m_ComponentContext = nullptr;

	public:
		TArchetypeTypeData() = default;

		TArchetypeTypeData(const ArchetypeTypeData& data):
			m_PackedContainer(::decs::check_cast<PackedContainerType*>(data.m_PackedContainer)),
			m_StableContainer(::decs::check_cast<StableContainerType*>(data.m_StableContainer)),
			m_ComponentContext(::decs::check_cast<ComponentContextType*>(data.m_ComponentContext))
		{

		}


		inline bool IsTag() const
		{
			return m_PackedContainer != nullptr;
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

	class Archetype final
	{
		friend class Container;
		friend class ContainerIterator;
		friend struct EntityData;
		friend struct EntityManager;
		friend class ArchetypesMap;
		template<typename>
		friend class ContainerSerializer;
		friend class ContainerSerializerComplex;

		template<ComponentConcept...>
		friend class Query;
		template<ComponentConcept...>
		friend class MultiQuery;
		template<ComponentConcept... ComponentsTypes>
		friend class IterationArchetypeContext;
		template<ComponentConcept...>
		friend class IterationContainerContext;

		template<ComponentConcept...>
		friend class BatchIterator;

	private:
		ecsMap<TypeID, uint32_t> m_TypeIDsIndexes{};
		ecsMap<TypeID, ArchetypeEdge> m_Edges{};

		ArchetypeEntityDataStorage m_EntityStorage{};
		ecsVector<ArchetypeTypeData> m_TypeData{};

		struct OrderData
		{
		public:
			IComponentContext* m_ComponentContext = nullptr;
			uint32_t m_ComponentIndex = std::numeric_limits<uint32_t>::max();
		};

		ecsVector<OrderData> m_ComponentContextsInOrder{};

	public:
		Archetype();

		~Archetype();

		inline const ArchetypeEntityDataStorage& GetEntityStorage() const noexcept
		{
			return m_EntityStorage;
		}

		inline uint32_t GetComponentTagCount() const noexcept
		{
			return static_cast<uint32_t>(m_TypeData.size());
		}

		/// <summary>
		/// Returns number of components and tags
		/// </summary>
		/// <returns></returns>
		inline uint32_t GetComponentAndTagCount() const noexcept
		{
			return GetComponentTagCount();
		}

		inline uint32_t GetComponentOnlyCount() const
		{
			return static_cast<uint32_t>(m_ComponentContextsInOrder.size());
		}

		inline TypeID GetTypeID(uint64_t index) const
		{
			return m_TypeData[index].m_TypeID;
		}

		inline TypeID GetTypeIDFromOrderData(uint64_t index) const
		{
			return m_TypeData[m_ComponentContextsInOrder[index].m_ComponentIndex].m_TypeID;
		}

		inline uint64_t EntityCount() const noexcept
		{
			return m_EntityStorage.GetSize();
		}

		inline float GetLoadFactor()const
		{
			return m_EntityStorage.GetLoadFactor();
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

		bool HasSameComponentsTagsAs(const Archetype& archetype)  const;

		/// <summary>
		/// if types.size() is different thant archetype type count returns false.
		/// </summary>
		/// <param name="types"></param>
		/// <returns></returns>
		bool HasTypes_Exactly(const ecsVector<TypeID>& types) const;

		/// <summary>
		/// if group.Size() is different thant archetype type count returns false.
		/// </summary>
		/// <typeparam name="...Types"></typeparam>
		/// <param name="types"></param>
		/// <returns></returns>
		template<typename... Types>
		bool HasTypes_Exactly(const TypeGroup<Types...>& group) const
		{
			const uint32_t componentAndTagCount = GetComponentAndTagCount();

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

		template<ComponentConcept... ComponentTypes, tag_concept... TagTypes>
		bool IsArchetypeWithComponentsAndTags_Exactly(
			const ComponentTypeGroup<ComponentTypes...> components,
			const TagTypeGroup<TagTypes...> tags
		) const noexcept
		{
			if ((sizeof...(ComponentTypes) + sizeof...(TagTypes)) != GetComponentTagCount())
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
		std::optional<TypeID> IsRemoveAnyDataNeighbour(const Archetype& neighbour) const;

		/// <summary>
		/// 
		/// </summary>
		/// <param name="neighbour"></param>
		/// <returns>Archetype with larger number of componetns than this archetype</returns>
		std::optional<TypeID> IsAddAnyDataNeighbour(const Archetype& neighbour) const;

		inline bool HasAnyEdge(TypeID toTypeID) const noexcept
		{
			return m_Edges.contains(toTypeID);
		}

	private:

		template<typename TComponentType>
		PackedStableComponentContainer<TComponentType>* GetTypePackedContainer() const
		{
			uint32_t compIdx = FindTypeIndex<TComponentType>();
			if (compIdx == std::numeric_limits<uint32_t>::max())
			{
				return nullptr;
			}
			auto& typeData = m_TypeData[compIdx];
			if (typeData.IsTag())
			{
				return nullptr;
			}

			return ::decs::check_cast<PackedStableComponentContainer<TComponentType>*>(typeData.m_PackedContainer);
		}

		IPackedComponentContainer* GetTypePackedContainer(TypeID componentTypeID) const
		{
			uint32_t compIdx = FindTypeIndex(componentTypeID);
			if (compIdx == std::numeric_limits<uint32_t>::max())
			{
				return nullptr;
			}
			auto& typeData = m_TypeData[compIdx];
			if (typeData.IsTag())
			{
				return nullptr;
			}

			return typeData.m_PackedContainer;
		}

		template<ComponentConcept ComponentType>
		TArchetypeTypeData<pure_type_t<ComponentType>> GetTypeData() const
		{
			uint32_t compIdx = FindTypeIndex<pure_type_t<ComponentType>>();
			if (compIdx == std::numeric_limits<uint32_t>::max())
			{
				return {};
			}

			return TArchetypeTypeData<pure_type_t<ComponentType>>(m_TypeData[compIdx]);
		}

		void ClearEntityDataAndComponents();

		// it must be called only from "AddTypeData_WithoutCheck" function
		void InsertComponentContextInCorrectPlace(IComponentContext* componentContext, uint32_t typeDataIndex);

		void AddTypeData_WithoutCheck(
			TypeID typeID,
			IComponentContext* componentContext
		);

		void UpdateOrderOfComponentContexts();

		inline void SetEntityActiveState(uint32_t index, bool bIsActive)
		{
			if (index < m_EntityStorage.GetSize())
			{
				m_EntityStorage.SetEnabled(index, bIsActive);
			}
		}

		void AddEntityData(EntityData* entityData);

		void RemoveSwapBackEntityData(uint64_t index);

		void RemoveSwapBackEntity(uint64_t index);

		void RemoveSwapBackEntityAfterRemoveComponent(uint64_t index);

		/// <summary>
		/// Removes entity data and components on index. Do not destroy stable components and do not change in any way entity data.
		/// </summary>
		/// <param name="index"></param>
		void RemoveSwapBackRecordRaw(uint64_t index);

		void SetRecordAsIntendedToDelayedDestroy(uint64_t index);

		inline IPackedComponentContainer* GetPackedContainerAt(uint64_t index)
		{
			return m_TypeData[index].m_PackedContainer;
		}

		void ReserveSpaceInArchetype(uint64_t desiredCapacity);

		void Reset();

		void InitEmptyFromOther(const Archetype& other, ComponentContextsManager* componentContexts);

		void RemoveSwapBackEntityAfterMoveEntityWithoutDestroyingSource(uint64_t entityIndex, TypeID removedComponentTypeID);

		void ShrinkToFit();

	#pragma region EDGES
	private:
		void AddEdge(TypeID componentTypeID, Archetype* archetype, EArchetypeEdgeType edgeType);

		template<ComponentConcept TComponent>
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

		static bool MoveEntityComponentsAfterAddComponent(
			Archetype& fromArchetype,
			Archetype& toArchetype,
			uint64_t entityIndex,
			TypeID addedComponentTypeID
		);

		static bool MoveEntityAfterAddComponentWithoutDestroyingFromSource(
			Archetype& fromArchetype,
			Archetype& toArchetype,
			uint64_t entityIndex,
			TypeID addedComponentTypeID
		);

		static bool MoveEntityAfterRemoveComponentWithoutDestroyingFromSource(
			Archetype& fromArchetype,
			Archetype& toArchetype,
			uint64_t entityIndex,
			TypeID removedComponentTypeID
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

			return m_ArchetypeConst->HasSameComponentsTagsAs(*rhs.m_ArchetypeConst);
		}

		inline const Archetype* GetConstArchetype() const
		{
			return m_ArchetypeConst;
		}

		std::size_t CalculateHash() const noexcept
		{
			if (m_ArchetypeConst == nullptr || m_ArchetypeConst->GetComponentAndTagCount() == 0)
			{
				return 0;
			}

			std::size_t finalHash = std::hash<TypeID>{}(m_ArchetypeConst->GetTypeID(0));
			const uint32_t typeCount = m_ArchetypeConst->GetComponentAndTagCount();
			for (uint32_t typeIdx = 1; typeIdx < typeCount; typeIdx++)
			{
				finalHash = hash::Combine(finalHash, std::hash<TypeID>{}(m_ArchetypeConst->GetTypeID(typeIdx)));
			}

			return finalHash;
		}

	private:
		const Archetype* m_ArchetypeConst = nullptr;
	};
}

template<>
struct std::hash<decs::ArchetypeHasher>
{
	std::size_t operator()(const decs::ArchetypeHasher& archHasher)const
	{
		return archHasher.CalculateHash();
	}
};