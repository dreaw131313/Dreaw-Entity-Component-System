#pragma once
#include "decs/Core.h"
#include "decs/ComponentContext/ComponentContextsManager.h"
#include "decs/Type.h"
#include "decs/ComponentContainers/PackedContainer.h"
#include "decs/ComponentContainers/StableContainer.h"
#include "decs/EntityData.h"


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
	};

	struct ArchetypeTypeData
	{
	public:
		TypeID m_TypeID = std::numeric_limits<TypeID>::max();
		PackedContainerBase* m_PackedContainer = nullptr;
		ComponentContextBase* m_ComponentContext = nullptr;
		StableContainerBase* m_StableContainer = nullptr;

	public:
		ArchetypeTypeData()
		{

		}

		ArchetypeTypeData(
			TypeID typeID,
			PackedContainerBase* packedContainer,
			ComponentContextBase* componentContext,
			StableContainerBase* stableContainer
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
		Remove
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

		template<typename...>
		friend class Query;
		template<typename...>
		friend class MultiQuery;
		template<typename, typename...>
		friend class IterationContainerContext;

		template<typename...>
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

		uint32_t m_ComponentsCount = 0; // number of components for each entity
		uint32_t m_EntitiesCount = 0;

	public:
		Archetype();

		~Archetype();

		inline uint32_t ComponentCount() const
		{
			return m_ComponentsCount;
		}

		inline TypeID GetTypeID(uint64_t index) const
		{
			return m_TypeData[index].m_TypeID;
		}

		inline std::string GetComponentTypeName(uint64_t componentIndex) const
		{
			if (componentIndex < m_TypeData.size())
			{
				auto& typeData = m_TypeData[componentIndex];
				if (typeData.m_ComponentContext != nullptr)
				{
					return typeData.m_ComponentContext->GetComponentName();
				}
			}

			return std::string();
		}

		inline uint32_t EntityCount() const
		{
			return m_EntitiesCount;
		}

		inline float GetLoadFactor()const
		{
			if (m_EntitiesData.capacity() == 0) return 1.f;
			return (float)m_EntitiesCount / (float)m_EntitiesData.capacity();
		}

		inline bool ContainType(TypeID typeID) const
		{
			return m_TypeIDsIndexes.find(typeID) != m_TypeIDsIndexes.end();
		}

		uint32_t FindTypeIndex(TypeID typeID) const;

		template<typename T>
		inline uint32_t FindTypeIndex() const
		{
			TYPE_ID_CONSTEXPR TypeID typeID = Type<T>::ID();
			if (m_ComponentsCount < Limits::MinComponentsInArchetypeToPerformMapLookup)
			{
				for (uint32_t i = 0; i < m_ComponentsCount; i++)
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

		template<typename TTag>
		inline bool HasTag() const
		{
			return HasTag(Type<TTag>::ID());
		}

	private:
		void ClearEntityDataAndComponents();

		// it must be called only from "AddTypeData_WithoutCheck" function
		void InsertComponentContextInCorrectPlace(ComponentContextBase* componentContext, uint32_t typeDataIndex);

		void AddTypeData_WithoutCheck(
			TypeID typeID,
			PackedContainerBase* packedContainer,
			ComponentContextBase* componentContext
		);

		void AddTypeData_WithCheck(
			const TypeID& id,
			PackedContainerBase* packedContainer,
			ComponentContextBase* componentContext
		);

		void UpdateOrderOfComponentContexts();

		inline void SetEntityActiveState(uint32_t index, bool isActive)
		{
			if (index < m_EntitiesCount)
			{
				m_EntitiesData[index].m_bIsActive = isActive;
			}
		}

		void AddEntityData(EntityData* entityData);

		void RemoveSwapBackEntityData(uint64_t index);

		void RemoveSwapBackEntity(uint64_t index);

		/// <summary>
		/// Removes entity data and components on index. Do not destroy stable components and do not change in any way entity data.
		/// </summary>
		/// <param name="index"></param>
		void RemoveSwapBackRecordRaw(uint64_t index);

		void SetRecordAsIntendedToDelayedDestroy(uint64_t index);

		inline PackedContainerBase* GetPackedContainerAt(uint64_t index)
		{
			return m_TypeData[index].m_PackedContainer;
		}

		void ReserveSpaceInArchetype(uint64_t desiredCapacity);

		void Reset();

		void InitEmptyFromOther(Archetype& other, ComponentContextsManager* componentContexts);

		/*
		/// <summary>
		/// Moves entity components from "fromArchetype" to this archetype.
		/// </summary>
		/// <param name="componentTypeID"></param>
		/// <param name="fromArchetype"></param>
		/// <param name="fromIndex"></param>
		void MoveEntityComponentsAfterRemoveComponent(
			TypeID removedComponentTypeID,
			Archetype* fromArchetype,
			uint64_t fromIndex,
			EntityData* entityData
		);

		void MoveEntityComponentsAfterRemoveComponent(
			Archetype* fromArchetype,
			uint64_t fromIndex,
			EntityData* entityData
		);

		*/

		void MoveEntityAfterRemoveComponentWithoutDestroyingFromSource(
			TypeID removedComponentTypeID,
			Archetype* fromArchetype,
			uint64_t fromIndex,
			EntityData* entityData
		);

		void RemoveSwapBackEntityAfterMoveEntityWithoutDestroyingSource(uint64_t entityIndex, TypeID removedComponentTypeID);

		void MoveEntityAfterAddComponentWithoutDestroyingFromSource(
			Archetype* fromArchetype,
			uint64_t fromIndex,
			TypeID newComponentTypeID,
			EntityData* entityData
		);

		/// <summary>
		/// Moves entity components from "fromArchetype" to this archetype.
		/// </summary>
		/// <typeparam name="ComponentType"></typeparam>
		/// <param name="fromArchetype"></param>
		/// <param name="fromIndex"></param>
		template<typename TComponent>
		void MoveEntityComponentsAfterAddComponent(Archetype* fromArchetype, uint64_t fromIndex, EntityData* entityData)
		{
			TYPE_ID_CONSTEXPR TypeID newComponentTypeID = Type<TComponent>::ID();

			this->AddEntityData(entityData);

			uint64_t thisArchetypeIndex = 0;
			uint64_t fromArchetypeIndex = 0;

			for (; thisArchetypeIndex < m_ComponentsCount; thisArchetypeIndex++)
			{
				ArchetypeTypeData& thisTypeData = m_TypeData[thisArchetypeIndex];
				if (thisTypeData.m_TypeID == newComponentTypeID)
				{
					continue;
				}

				ArchetypeTypeData& fromArchetypeData = fromArchetype->m_TypeData[fromArchetypeIndex];

				thisTypeData.m_PackedContainer->PushBack(
					fromArchetypeData.m_PackedContainer->GetComponentBasePtr(fromIndex)
				);

				fromArchetypeData.m_PackedContainer->RemoveSwapBack(fromIndex);

				fromArchetypeIndex++;
			}

			fromArchetype->RemoveSwapBackEntityData(fromIndex);
		}

		void ShrinkToFit();

		// Edges utility functions:
		template<typename TComponent>
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

		template<typename TComponent>
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

		/// <summary>
		/// Used only when calling on destroy and on disable observers when removing components
		/// </summary>
		/// <param name="entityData"></param>
		/// <param name="index"></param>
		/// <returns></returns>
		bool SetPlaceHolderEntityData(EntityData* entityData, uint32_t index)
		{
			if (index < m_EntitiesCount)
			{
				auto& archetypeEntityData = m_EntitiesData[index];
				archetypeEntityData.m_EntityData = entityData;
				archetypeEntityData.m_bIsActive = false;
				return true;
			}
			return false;
		}
	};
}