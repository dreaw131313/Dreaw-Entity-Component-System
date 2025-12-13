#pragma once
#include "decs/Core.h"
#include "decs/Component/ComponentContextsManager.h"
#include "decs/Type.h"
#include "decs/Component/PackedComponentContainer.h"
#include "decs/Component/StableComponentContainer.h"
#include "decs/EntityData.h"

#include "decs/trait.h"

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
		Add,
		Destroy
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

		/// <summary>
		/// Returns number of components and tags
		/// </summary>
		/// <returns></returns>
		inline uint32_t GetComponentAndTagCount() const
		{
			return static_cast<uint32_t>(m_TypeData.size());
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

		inline bool ContainType(TypeID typeID) const
		{
			return m_TypeIDsIndexes.find(typeID) != m_TypeIDsIndexes.end();
		}

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
			TYPE_ID_CONSTEXPR TypeID typeID = Type<T>::ID();
			if (GetComponentAndTagCount() < Limits::MinComponentsInArchetypeToPerformMapLookup)
			{
				for (uint32_t i = 0; i < GetComponentAndTagCount(); i++)
					if (m_TypeData[i].m_TypeID == typeID) return i;

				return std::numeric_limits<uint32_t>::max();
			}

			auto it = m_TypeIDsIndexes.find(typeID);
			if (it == m_TypeIDsIndexes.end())
				return std::numeric_limits<uint32_t>::max();

			return it->second;
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
		template<TComponentConcept TComponent>
		void AddEdge(Archetype* archetype, EComponentEdgeType edgeType)
		{
			auto& edge = m_Edges[Type<TComponent>::ID()];
			if (!edge.IsValid())
			{
				edge.m_Archetype = archetype;
				edge.m_EdgeType = edgeType;
			}
		}

		void AddEdge(TypeID componentTypeID, Archetype* archetype, EComponentEdgeType edgeType)
		{
			auto& edge = m_Edges[componentTypeID];
			if (!edge.IsValid())
			{
				edge.m_Archetype = archetype;
				edge.m_EdgeType = edgeType;
			}
		}

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
}