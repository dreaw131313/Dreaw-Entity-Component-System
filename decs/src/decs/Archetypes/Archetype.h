#pragma once
#include "decs/Core.h"
#include "decs/Component/ComponentContextsManager.h"
#include "decs/Type.h"
#include "decs/Component/PackedComponentContainer.h"
#include "decs/Component/StableComponentContainer.h"
#include "decs/EntityData.h"
#include "decs/trait.h"
#include "decs/Hash.h"

#include <optional>

namespace decs
{
	class Entity;
	class Archetype;

	struct ArchetypeEntityData
	{
	public:
		EntityData* m_EntityData = nullptr;
		bool m_bIsActive = false;

	public:
		ArchetypeEntityData()
		{

		}

		ArchetypeEntityData(
			EntityData* entityData
		):
			m_EntityData(entityData),
			m_bIsActive(entityData->IsActive())
		{

		}

		inline EntityData* GetEntityData()
		{
			return m_EntityData;
		}

		inline bool IsActive() const noexcept
		{
			return m_bIsActive;
		}

		inline void Invalidate()
		{
			m_bIsActive = false;
			m_EntityData = nullptr;
		}

		inline bool IsValid() const
		{
			return m_EntityData != nullptr;
		}

		inline bool IsValidAndActive() const noexcept
		{
			return m_EntityData != nullptr && m_bIsActive;
		}
	};

	struct ArchetypeTypeData
	{
	public:
		TypeID m_TypeID = std::numeric_limits<TypeID>::max();
		IPackedComponentContainer* m_PackedContainer = nullptr;
		ComponentContextBase* m_ComponentContext = nullptr;
		IStableComponentContainer* m_StableContainer = nullptr;

	public:
		ArchetypeTypeData()
		{

		}

		ArchetypeTypeData(
			TypeID typeID,
			IPackedComponentContainer* packedContainer,
			ComponentContextBase* componentContext,
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

	enum class EComponentEdgeType
	{
		Add = 0,
		Remove = 1
	};

	struct ArchetypeEdge
	{
	public:
		Archetype* m_Archetype = nullptr;
		EComponentEdgeType m_EdgeType = EComponentEdgeType::Add;

	public:
		ArchetypeEdge()
		{

		}

		ArchetypeEdge(Archetype* archetype, EComponentEdgeType edgeType):
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
		friend class EntityData;
		friend class EntityManager;
		friend class ArchetypesMap;
		template<typename>
		friend class ContainerSerializer;
		friend class ContainerSerializerComplex;

		template<TComponentConcept...>
		friend class Query;
		template<TComponentConcept...>
		friend class MultiQuery;
		template<TComponentConcept... ComponentsTypes>
		friend class IterationArchetypeContext;
		template<TComponentConcept...>
		friend class IterationContainerContext;

		template<TComponentConcept...>
		friend class BatchIterator;


	private:
		ecsMap<TypeID, uint32_t> m_TypeIDsIndexes;
		ecsMap<TypeID, ArchetypeEdge> m_Edges;

		std::vector<ArchetypeEntityData> m_EntitiesData;
		std::vector<ArchetypeTypeData> m_TypeData;

		struct OrderData
		{
		public:
			ComponentContextBase* m_ComponentContext = nullptr;
			uint32_t m_ComponentIndex = std::numeric_limits<uint32_t>::max();
		};

		std::vector<OrderData> m_ComponentContextsInOrder = {};


	public:
		Archetype();

		~Archetype();

		inline uint32_t GetTypeCount() const noexcept
		{
			return static_cast<uint32_t>(m_TypeData.size());
		}

		/// <summary>
		/// Returns number of components and tags
		/// </summary>
		/// <returns></returns>
		inline uint32_t GetComponentAndTagCount() const noexcept
		{
			return GetTypeCount();
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
			return m_EntitiesData.size();
		}

		inline float GetLoadFactor()const
		{
			if (m_EntitiesData.capacity() == 0) return 1.f;
			return (float)EntityCount() / (float)m_EntitiesData.capacity();
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

		template<TTagConcept TTag>
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

		template<TComponentConcept... ComponentTypes, TTagConcept... TagTypes>
		bool IsArchetypeWithComponentsAndTags_Exactly(
			const ComponentTypeGroup<ComponentTypes...> components,
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

			return static_cast<PackedStableComponentContainer<TComponentType>*>(typeData.m_PackedContainer);
		}

		void ClearEntityDataAndComponents();

		// it must be called only from "AddTypeData_WithoutCheck" function
		void InsertComponentContextInCorrectPlace(ComponentContextBase* componentContext, uint32_t typeDataIndex);

		void AddTypeData_WithoutCheck(
			TypeID typeID,
			ComponentContextBase* componentContext
		);

		void UpdateOrderOfComponentContexts();

		inline void SetEntityActiveState(uint32_t index, bool isActive)
		{
			if (index < EntityCount())
			{
				m_EntitiesData[index].m_bIsActive = isActive;
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
		void AddEdge(TypeID componentTypeID, Archetype* archetype, EComponentEdgeType edgeType);

		template<TComponentConcept TComponent>
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
				|| m_ArchetypeConst->GetTypeCount() != rhs.m_ArchetypeConst->GetTypeCount()
				)
			{
				return false;
			}

			return m_ArchetypeConst->HasSameTypesAs(*rhs.m_ArchetypeConst);
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