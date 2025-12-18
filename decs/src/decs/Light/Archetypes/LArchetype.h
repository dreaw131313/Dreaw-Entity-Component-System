#pragma once
#include "decs/Core/Core.h"
#include "decs/Core/Type.h"
#include "decs/Core/trait.h"
#include "decs/Core/Hash.h"
#include "decs/Core/check_cast.h"
#include "decs/Light/Component/PackedLightComponentContainer.h"
#include "decs/Light/LEntityData.h"

#include "decs/Light/LightForward.h"

#include <optional>

namespace decs::light
{
	class Entity;
	class Archetype;

	struct ArchetypeEntityData
	{
	public:
		EntityData* m_EntityData = nullptr;

	public:
		ArchetypeEntityData()
		{

		}

		ArchetypeEntityData(
			EntityData* entityData
		):
			m_EntityData(entityData)
		{

		}

		inline EntityData* GetEntityData()
		{
			return m_EntityData;
		}

		inline void Invalidate()
		{
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
		IPackedLightComponentContainer* m_PackedContainer = nullptr;

	public:
		ArchetypeTypeData()
		{

		}

		ArchetypeTypeData(
			TypeID typeID,
			IPackedLightComponentContainer* packedContainer
		):
			m_TypeID(typeID), m_PackedContainer(packedContainer)
		{

		}

		inline bool IsTag() const
		{
			return m_PackedContainer == nullptr;
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

	private:
		ecsMap<TypeID, uint32_t> m_TypeIDsIndexes;
		ecsMap<TypeID, ArchetypeEdge> m_Edges;

		std::vector<ArchetypeEntityData> m_EntitiesData;
		std::vector<ArchetypeTypeData> m_TypeData;

	public:
		Archetype();

		~Archetype();

		inline uint32_t GetTypeCount() const noexcept
		{
			return static_cast<uint32_t>(m_TypeData.size());
		}

		inline TypeID GetTypeID(uint64_t index) const
		{
			return m_TypeData[index].m_TypeID;
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

		template<TLightComponentConcept... ComponentTypes, TTagConcept... TagTypes>
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

		void RemoveSwapBackEntityData(uint64_t index);

		void RemoveSwapBackEntity(uint64_t index);

		void ReserveSpaceInArchetype(uint64_t desiredCapacity);

		void Reset();

		void InitEmptyFromOther(const Archetype& other);

		void ShrinkToFit();

	#pragma region EDGES
	private:
		void AddEdge(TypeID componentTypeID, Archetype* archetype, EComponentEdgeType edgeType);

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
			if (m_ArchetypeConst == nullptr || m_ArchetypeConst->GetTypeCount() == 0)
			{
				return 0;
			}

			std::size_t finalHash = std::hash<TypeID>{}(m_ArchetypeConst->GetTypeID(0));
			const uint32_t typeCount = m_ArchetypeConst->GetTypeCount();
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
struct std::hash<decs::light::ArchetypeHasher>
{
	std::size_t operator()(const decs::light::ArchetypeHasher& archHasher)const
	{
		return archHasher.CalculateHash();
	}
};