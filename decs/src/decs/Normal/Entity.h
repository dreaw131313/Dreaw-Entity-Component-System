#pragma once
#include "decs/Core/Core.h"
#include "decs/Core/Type.h"
#include "decs/Core/Hash.h"
#include "decs/Core/trait.h"

#include "Container.h"

namespace decs
{
	struct Entity final
	{
		template<component_concept ...>
		friend class Query;
		template<component_concept ...>
		friend class MultiQuery;
		friend class Container;
		template<typename>
		friend class ContainerSerializer;
		friend class ContainerSerializerComplex;
		friend class ContainerIterator;
		friend class Iteration;
		friend class EntityComponent;

		friend struct ConstEntity;

		friend struct std::hash<decs::Entity>;

	public:
		Entity()
		{

		}

		Entity(Container& container, EntityData& entityData) :
			m_LifeTimeData(container.GetLifeTimeData()),
			m_EntityID(entityData.GetID()),
			m_Version(entityData.GetVersion())
		{

		}

		inline operator bool() const noexcept
		{
			return IsValid();
		}

		bool operator==(const Entity& rhs)const
		{
			return m_LifeTimeData == rhs.m_LifeTimeData
				&& m_EntityID == rhs.m_EntityID
				&& m_Version == rhs.m_Version;
		}

		[[nodiscard]] inline std::size_t CalculateHash() const noexcept
		{
			if (IsValid())
			{
				const uint64_t entityIDHash = std::hash<decs::EntityID>{}(m_EntityID);
				const uint64_t entityVersionHash = std::hash<decs::EntityVersion>{}(m_Version);

				return decs::hash::Combine(decs::hash::Combine(entityIDHash, entityVersionHash), std::hash<ContainerLifetimeData*>{}(m_LifeTimeData.Get()));
			}
			return 0ull;
		}

		[[nodiscard]] inline bool IsValid() const
		{
			return m_LifeTimeData.IsValid() && m_LifeTimeData->IsAlive()
				&& GetEntityData_Internal()->IsAliveWithVersion(m_Version)
				;
		}

		[[nodiscard]] inline bool IsNull() const
		{
			return !IsValid();
		}

		[[nodiscard]] inline bool IsInDestruction() const
		{
			if (EntityData* entityData = TryGetEntityData())
			{
				return entityData->IsInDestructionOrDelayedToDestruction();
			}
			return false;
		}

		[[nodiscard]] inline EntityID GetID() const
		{
			return m_EntityID;
		}

		/// <summary>
		/// If entity is valid it 32 lower bits stores entity id, and upper 32 bits stores version of this entity
		/// </summary>
		/// <returns></returns>
		[[nodiscard]] inline CombinedEntityID GetCombinedID() const noexcept
		{
			if (EntityData* entityData = TryGetEntityData())
			{
				return static_cast<CombinedEntityID>(m_EntityID & 0xFFFFFFFFull) | (static_cast<CombinedEntityID>(m_Version) << (sizeof(EntityID) * 8));
			}
			return InvalidCombinedID;
		}

		[[nodiscard]] inline Container* GetContainer() const
		{
			if (IsValid())
			{
				return m_LifeTimeData->GetContainer();
			}
			return nullptr;
		}

		/// <summary>
		/// 
		/// </summary>
		/// <returns>Active state with disable overrides taken into account</returns>
		[[nodiscard]] inline bool IsActive() const
		{
			if (EntityData* entityData = TryGetEntityData())
			{
				return entityData->IsActive();
			}
			return false;
		}

		/// <summary>
		/// 
		/// </summary>
		/// <returns>Active state without disable overrides taken into account</returns>
		[[nodiscard]] inline bool IsActiveFlag() const
		{
			if (EntityData* entityData = TryGetEntityData())
			{
				return entityData->IsActiveFlag();
			}
			return false;
		}

		inline void SetActive(const bool& isActive) const
		{
			if (EntityData* entityData = TryGetEntityData())
			{
				GetContainer_Internal()->SetEntityActive(*entityData, *this, isActive);
			}
		}

		void SetActiveOverride(bool bIsActiveOverride) const
		{
			if (EntityData* entityData = TryGetEntityData())
			{
				GetContainer_Internal()->SetEntityActiveOverride(*entityData, *this, bIsActiveOverride);
			}
		}

		void SetDisabledOverrideCount(uint32_t disabledOverrideCount) const
		{
			if (EntityData* entityData = TryGetEntityData())
			{
				GetContainer_Internal()->SetEntityDisabledOverrideCount(*entityData, *this, disabledOverrideCount);
			}
		}

		void ResetDisabledOverrideCount() const
		{
			if (EntityData* entityData = TryGetEntityData())
			{
				GetContainer_Internal()->ResetDisabledOverrideCount(*entityData, *this);
			}
		}

		[[nodiscard]] uint32_t GetDisabledOverrideCount() const
		{
			if (EntityData* entityData = TryGetEntityData())
			{
				return entityData->GetDisabledOverrideCount();
			}

			return 0;
		}

		inline bool Destroy() const
		{
			if (EntityData* entityData = TryGetEntityData())
			{
				GetContainer_Internal()->DestroyEntityInternal(*entityData, *this, true);
				Invalidate_WithoutLifeTimeData();
				return true;
			}
			return false;
		}

		[[nodiscard]] inline uint32_t GetComponentCount() const
		{
			if (EntityData* entityData = TryGetEntityData())
			{
				return entityData->GetComponentOnlyCount();
			}
			return 0;
		}

		[[nodiscard]] inline uint32_t GetComponentAndTagCount() const
		{
			if (EntityData* entityData = TryGetEntityData())
			{
				return entityData->GetComponentAndTagCount();
			}
			return 0;
		}

		/// <summary>
		/// 
		/// </summary>
		/// <param name="componentIndex"></param>
		/// <returns>Components in observers order</returns>
		[[nodiscard]] inline EntityComponent* GetComponentAtIndex_ObserversOrder(uint32_t componentIndex) const
		{
			if (EntityData* entityData = TryGetEntityData())
			{
				return GetContainer_Internal()->GetComponentAtIndex_ObserversOrder(*entityData, componentIndex);
			}
			return nullptr;
		}

		/// <summary>
		/// Gets component by index in order of typeID. Uses ecsVector where component and tags records data are placed. It can return nullptr if componentIndex is greater than component and tag count in archetype or where index points to tag instead of component.
		/// </summary>
		/// <param name="componentIndex"></param>
		/// <returns>Components in type id order</returns>
		[[nodiscard]] inline EntityComponent* GetComponentAtIndex_TypeIDOrder(uint32_t componentIndex) const
		{
			if (EntityData* entityData = TryGetEntityData())
			{
				return GetContainer_Internal()->GetComponentAtIndex_TypeIDOrder(*entityData, componentIndex);
			}
			return nullptr;
		}

		template<component_concept ComponentType>
		[[nodiscard]] inline ComponentType* GetComponent() const
		{
			if (EntityData* entityData = TryGetEntityData())
			{
				return GetContainer_Internal()->GetComponent<drop_const_t<ComponentType>>(*entityData);
			}

			return nullptr;
		}

		template<component_concept... ComponentTypes>
		[[nodiscard]] inline std::tuple<ComponentTypes*...> GetComponents()
		{
			if (EntityData* entityData = TryGetEntityData())
			{
				return GetContainer_Internal()->GetComponents<ComponentTypes...>(*entityData);
			}

			return { static_cast<ComponentTypes*>(nullptr) ... };
		}

		[[nodiscard]] inline EntityComponent* GetComponent(TypeID componentType) const
		{
			if (EntityData* entityData = TryGetEntityData())
			{
				return GetContainer_Internal()->GetComponent(*entityData, componentType);
			}

			return nullptr;
		}

		template<component_concept ComponentType>
		[[nodiscard]] inline bool HasComponent() const
		{
			if (EntityData* entityData = TryGetEntityData())
			{
				return GetContainer_Internal()->HasComponent<pure_type_t<ComponentType>>(*entityData);
			}
			return false;
		}

		template<component_concept ComponentType>
		inline bool TryGetComponent(ComponentType*& component) const
		{
			if (EntityData* entityData = TryGetEntityData())
			{
				component = GetContainer_Internal()->GetComponent<pure_type_t<ComponentType>>(*entityData);
			}
			else
			{
				component = nullptr;
			}

			return component != nullptr;
		}

		template<component_concept ComponentType, typename... Args>
		inline typename ComponentType* AddComponent(Args&&... args) const
		{
			if (EntityData* entityData = TryGetEntityData())
			{
				return GetContainer_Internal()->AddComponent<pure_type_t<ComponentType>>(*this, *entityData, std::forward<Args>(args)...);
			}

			return nullptr;
		}

		template<component_concept ComponentType>
		inline bool RemoveComponent() const
		{
			if (EntityData* entityData = TryGetEntityData())
			{
				return GetContainer_Internal()->RemoveComponent<pure_type_t<ComponentType>>(*entityData, *this);
			}
			return false;
		}

		inline bool RemoveComponent(TypeID componentTypeID) const
		{
			if (EntityData* entityData = TryGetEntityData())
			{
				return GetContainer_Internal()->RemoveComponent(*entityData, *this, componentTypeID);
			}
			return false;
		}

		[[nodiscard]] inline EntityVersion GetVersion() const
		{
			return m_Version;
		}

		[[nodiscard]] inline const Archetype* GetArchetype() const
		{
			if (EntityData* entityData = TryGetEntityData())
			{
				return entityData->m_Archetype;
			}

			return nullptr;
		}

	#pragma region NO CALLBACK METHODS:
	public:
		/// <summary>
		/// Enable and disable observers of entity and components are not invoked.
		/// </summary>
		/// <param name="isActive"></param>
		inline void SetActive_NoObserver(const bool& isActive) const
		{
			if (EntityData* entityData = TryGetEntityData())
			{
				GetContainer_Internal()->SetEntityActive_NoObserver(*entityData, *this, isActive);
			}
		}

		/// <summary>
		/// Destroy observers of entity and components are not invoked.
		/// </summary>
		/// <returns></returns>
		inline bool Destroy_NoObserver() const
		{
			if (EntityData* entityData = TryGetEntityData())
			{
				GetContainer_Internal()->DestroyEntityInternal(*entityData, *this, false);
				Invalidate_WithoutLifeTimeData();
				return true;
			}
			return false;
		}

		/// <summary>
		/// Add component observers are not invoked.
		/// </summary>
		/// <returns></returns>
		template<component_concept ComponentType, typename... Args>
		inline typename ComponentType* AddComponent_NoObserver(Args&&... args) const
		{
			if (EntityData* entityData = TryGetEntityData())
			{
				return GetContainer_Internal()->AddComponent_NoObserver<drop_const_t<ComponentType>>(*this, *entityData, std::forward<Args>(args)...);
			}

			return nullptr;
		}

		/// <summary>
		/// Destroy component observers are not invoked.
		/// </summary>
		/// <returns></returns>
		template<component_concept ComponentType>
		inline bool RemoveComponent_NoObserver() const
		{
			if (EntityData* entityData = TryGetEntityData())
			{
				return GetContainer_Internal()->RemoveComponent_NoObserver<drop_const_t<ComponentType>>(*entityData, *this);
			}
			return false;
		}

		/// <summary>
		/// Destroy component observers are not invoked.
		/// </summary>
		/// <returns></returns>
		inline bool RemoveComponent_NoObserver(TypeID componentTypeID) const
		{
			if (EntityData* entityData = TryGetEntityData())
			{
				return GetContainer_Internal()->RemoveComponent_NoObserver(*entityData, *this, componentTypeID);
			}
			return false;
		}

		void SetActiveOverride_NoObserver(bool bIsActiveOverride) const
		{
			if (EntityData* entityData = TryGetEntityData())
			{
				GetContainer_Internal()->SetEntityActiveOverride_NoObserver(*entityData, *this, bIsActiveOverride);
			}
		}

		void SetDisabledOverrideCount_NoObserver(uint32_t disabledOverrideCount) const
		{
			if (EntityData* entityData = TryGetEntityData())
			{
				GetContainer_Internal()->SetEntityDisabledOverrideCount_NoObserver(*entityData, *this, disabledOverrideCount);
			}
		}

		void ResetDisabledOverrideCount_NoObserver() const
		{
			if (EntityData* entityData = TryGetEntityData())
			{
				GetContainer_Internal()->ResetDisabledOverrideCount_NoObserver(*entityData, *this);
			}
		}

	#pragma endregion

	#pragma region TAGS:
	public:
		/// <summary>
		/// 
		/// </summary>
		/// <param name="tagType">it must be type id acquired by Type<decs::tag<TagType>::ID()></param>
		/// <returns></returns>
		[[nodiscard]] inline bool HasTag(TypeID tagType)const
		{
			if (EntityData* entityData = TryGetEntityData())
			{
				return GetContainer_Internal()->HasTag(*entityData, tagType);
			}
			return false;
		}

		template<typename TagType>
		[[nodiscard]] inline bool HasTag()const
		{
			if (EntityData* entityData = TryGetEntityData())
			{
				return GetContainer_Internal()->HasTag<tag_type_t<TagType>>(*entityData);
			}
			return false;
		}

		template<typename... TagType>
		[[nodiscard]] inline bool HasTags()const
		{
			if (EntityData* entityData = TryGetEntityData())
			{
				return GetContainer_Internal()->HasTags<tag_type_t<TagType>...>(*entityData);
			}
			return false;
		}

		template<typename TagType>
		bool AddTag() const
		{
			if (EntityData* entityData = TryGetEntityData())
			{
				return GetContainer_Internal()->AddTag<tag_type_t<TagType>>(*entityData);
			}
			return false;
		}

		/// <summary>
		/// 
		/// </summary>
		/// <param name="tagType">it must be type id acquired by Type<decs::tag<TagType>::ID()</param>
		/// <returns></returns>
		bool RemoveTag(TypeID tagType)const
		{
			if (EntityData* entityData = TryGetEntityData())
			{
				return GetContainer_Internal()->RemoveTag(*entityData, tagType);
			}
			return false;
		}

		template<typename TagType>
		bool RemoveTag() const
		{
			if (EntityData* entityData = TryGetEntityData())
			{
				return GetContainer_Internal()->RemoveTag<tag_type_t<TagType>>(*entityData);
			}
			return false;
		}

	#pragma endregion

	private:
		ContainerLifetimeDataHandle m_LifeTimeData{};
		mutable EntityID m_EntityID = InvalidEntityID;
		mutable EntityVersion m_Version = InvalidEntityVersion;

	private:
		inline void Set_Internal(const Container& container, EntityData& data)
		{
			m_LifeTimeData = container.GetLifeTimeData();
			m_EntityID = data.GetID();
			m_Version = data.GetVersion();
		}

		inline void Invalidate_WithoutLifeTimeData() const
		{
			m_EntityID = InvalidEntityID;
			m_Version = InvalidEntityVersion;
		}

		inline void SetWithoutLifeTimeDataInvalidation_Internal(EntityData& data)
		{
			m_EntityID = data.GetID();
			m_Version = data.GetVersion();
		}

		inline void SetLifeTimeData_Internal(const ContainerLifetimeDataHandle& lifeTimeData)
		{
			m_LifeTimeData = lifeTimeData;
		}

		inline Container* GetContainer_Internal() const noexcept
		{
			return m_LifeTimeData->GetContainer();
		}

		inline EntityData* GetEntityData_Internal() const noexcept
		{
			return GetContainer_Internal()->GetEntityDataByID(m_EntityID);
		}

		inline EntityData* TryGetEntityData() const noexcept
		{
			if (m_LifeTimeData.IsValid() && m_LifeTimeData->IsAlive())
			{
				EntityData* entityData = m_LifeTimeData->GetContainer()->GetEntityDataByID(m_EntityID);;
				if (entityData->IsAliveWithVersion(m_Version))
				{
					return entityData;
				}
			}

			return nullptr;
		}
	};

	struct ConstEntity final
	{
		template<component_concept...>
		friend class Query;
		template<component_concept...>
		friend class MultiQuery;
		friend class Container;
		template<typename>
		friend class ContainerSerializer;
		friend class ContainerSerializerComplex;
		friend class ContainerIterator;

		friend struct std::hash<decs::ConstEntity>;

	public:
		ConstEntity()
		{

		}

		ConstEntity(const Entity& entity) :
			m_Entity(entity)
		{

		}

		inline operator bool() const noexcept
		{
			return IsValid();
		}

		bool operator==(const ConstEntity& rhs)const
		{
			return rhs.m_Entity == m_Entity;
		}

		bool operator==(const Entity& rhs)const
		{
			return rhs == m_Entity;
		}

		[[nodiscard]] inline bool IsValid() const noexcept
		{
			return m_Entity.IsValid();
		}

		[[nodiscard]] inline bool IsNull() const noexcept
		{
			return m_Entity.IsNull();
		}

		[[nodiscard]] inline bool IsActive() const noexcept
		{
			return m_Entity.IsActive();
		}

		[[nodiscard]] inline bool IsActiveFlag() const noexcept
		{
			return m_Entity.IsActiveFlag();
		}

		[[nodiscard]] inline EntityID GetID() const
		{
			return m_Entity.GetID();
		}

		[[nodiscard]] inline Container* GetContainer() const
		{
			return m_Entity.GetContainer();
		}

		template<component_concept ComponentType>
		[[nodiscard]] inline bool HasComponent() const
		{
			return m_Entity.HasComponent<ComponentType>();
		}

		template<component_concept ComponentType>
		[[nodiscard]] inline ComponentType* GetComponent() const
		{
			return m_Entity.GetComponent<ComponentType>();
		}

		template<component_concept ComponentType>
		inline bool TryGetComponent(typename ComponentType*& component) const
		{
			return m_Entity.TryGetComponent(component);
		}

		[[nodiscard]] inline EntityVersion GetVersion() const
		{
			return m_Entity.GetVersion();
		}

		[[nodiscard]] inline uint32_t GetComponentCount() const
		{
			return m_Entity.GetComponentCount();
		}

		[[nodiscard]] inline const Archetype* GetArchetype() const
		{
			return m_Entity.GetArchetype();
		}

		inline bool Destroy() const
		{
			return m_Entity.Destroy();
		}

		[[nodiscard]] inline bool HasTag(TypeID tagType)const
		{
			return m_Entity.HasTag(tagType);
		}

		template<tag_concept TTag>
		[[nodiscard]] inline bool HasTag()const
		{
			return m_Entity.HasTag<TTag>();
		}

	private:
		Entity m_Entity = {};

	};
}

template<>
struct std::hash<decs::Entity>
{
	std::size_t operator()(const decs::Entity& entity) const
	{
		return entity.CalculateHash();
	}
};

template<>
struct std::hash<decs::ConstEntity>
{
	std::size_t operator()(const decs::ConstEntity& entity) const
	{
		return std::hash<decs::Entity>{}(entity.m_Entity);
	}
};