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

		friend class ConstEntity;

		friend struct std::hash<decs::Entity>;

	public:
		Entity()
		{

		}

		Entity(EntityData& entityData):
			m_EntityData(&entityData),
			m_LifeTimeData(entityData.m_Container->m_LifeTimeData),
			m_Version(entityData.GetVersion())
		{

		}

		Entity(EntityData* entityData):
			m_EntityData(entityData),
			m_LifeTimeData(entityData != nullptr ? entityData->m_Container->GetLifeTimeData() : nullptr),
			m_Version(entityData != nullptr ? entityData->GetVersion() : 0)
		{

		}

		inline operator bool() const noexcept
		{
			return IsValid();
		}

		bool operator==(const Entity& rhs)const
		{
			return this->m_EntityData == rhs.m_EntityData && this->m_Version == rhs.m_Version;
		}

		[[nodiscard]] inline std::size_t CalculateHash() const noexcept
		{
			if (IsValid())
			{
				uint64_t entityDataHash = std::hash<decs::EntityData*>{}(m_EntityData);
				uint64_t entityVersionHash = std::hash<decs::EntityVersion>{}(m_Version);

				return decs::hash::Combine(entityDataHash, entityVersionHash);
			}
			return 0ull;
		}

		[[nodiscard]] inline bool IsValid() const
		{
			return m_LifeTimeData.IsValid() && m_LifeTimeData->IsAlive() && m_EntityData != nullptr && m_EntityData->IsAliveWithVersion(m_Version);
		}

		[[nodiscard]] inline bool IsNull() const
		{
			return !IsValid();
		}

		[[nodiscard]] inline bool IsInDestruction() const
		{
			if (IsValid())
			{
				return m_EntityData->IsInDestructionOrDelayedToDestruction();
			}
			return false;
		}

		[[nodiscard]] inline EntityID GetID() const
		{
			if (IsValid())
			{
				return m_EntityData->GetID();
			}
			return std::numeric_limits< EntityID>::max();
		}

		/// <summary>
		/// If entity is valid it 32 lower bits stores entity id, and upper 32 bits stores version of this entity
		/// </summary>
		/// <returns></returns>
		[[nodiscard]] inline CombinedEntityID GetCombinedID() const noexcept
		{
			if (IsValid())
			{
				return static_cast<CombinedEntityID>(m_EntityData->GetID() & 0xFFFFFFFFull) | (static_cast<CombinedEntityID>(m_Version) << (sizeof(EntityID) * 8));
			}
			return 0;
		}

		[[nodiscard]] inline Container* GetContainer() const
		{
			if (IsValid())
			{
				return m_EntityData->m_Container;
			}
			return nullptr;
		}

		/// <summary>
		/// 
		/// </summary>
		/// <returns>Active state with disable overrides taken into account</returns>
		[[nodiscard]] inline bool IsActive() const
		{
			return IsValid() && m_EntityData->IsActive();
		}

		/// <summary>
		/// 
		/// </summary>
		/// <returns>Active state without disable overrides taken into account</returns>
		[[nodiscard]] inline bool IsActiveFlag() const
		{
			return IsValid() && m_EntityData->IsActiveFlag();
		}

		inline void SetActive(const bool& isActive) const
		{
			if (IsValid())
			{
				GetContainer_Internal()->SetEntityActive(*this, isActive);
			}
		}

		void SetActiveOverride(bool bIsActiveOverride) const
		{
			if (IsValid())
			{
				GetContainer()->SetEntityActiveOverride(*this, bIsActiveOverride);
			}
		}

		void SetDisabledOverrideCount(uint32_t disabledOverrideCount) const
		{
			if (IsValid())
			{
				GetContainer()->SetEntityDisabledOverrideCount(*this, disabledOverrideCount);
			}
		}

		void ResetDisabledOverrideCount() const
		{
			if (IsValid())
			{
				GetContainer()->ResetDisabledOverrideCount(*this);
			}
		}

		[[nodiscard]] uint32_t GetDisabledOverrideCount() const
		{
			if (IsValid())
			{
				return m_EntityData->GetDisabledOverrideCount();
			}

			return 0;
		}

		inline bool Destroy() const
		{
			if (IsValid())
			{
				GetContainer_Internal()->DestroyEntityInternal(*this, true);
				Invalidate_WithoutLifeTimeData();
				return true;
			}
			return false;
		}

		[[nodiscard]] inline uint32_t GetComponentCount() const
		{
			if (IsValid())
			{
				return m_EntityData->GetComponentOnlyCount();
			}
			return 0;
		}

		[[nodiscard]] inline uint32_t GetComponentAndTagCount() const
		{
			if (IsValid())
			{
				return m_EntityData->GetComponentAndTagCount();
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
			if (IsValid())
			{
				return GetContainer_Internal()->GetComponentAtIndex_ObserversOrder(*m_EntityData, componentIndex);
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
			if (IsValid())
			{
				return GetContainer_Internal()->GetComponentAtIndex_TypeIDOrder(*m_EntityData, componentIndex);
			}
			return nullptr;
		}

		template<component_concept ComponentType>
		[[nodiscard]] inline ComponentType* GetComponent() const
		{
			if (IsValid())
			{
				return GetContainer_Internal()->GetComponent<drop_const_t<ComponentType>>(*GetEntityData());
			}

			return nullptr;
		}

		template<component_concept... ComponentTypes>
		[[nodiscard]] inline std::tuple<ComponentTypes*...> GetComponents()
		{
			if (IsValid())
			{
				return GetContainer()->GetComponents<ComponentTypes...>(*m_EntityData);
			}

			return { static_cast<ComponentTypes*>(nullptr) ... };
		}

		[[nodiscard]] inline EntityComponent* GetComponent(TypeID componentType) const
		{
			if (IsValid())
			{
				return GetContainer_Internal()->GetComponent(*m_EntityData, componentType);
			}

			return nullptr;
		}

		/// <summary>
		/// Iterates over all components on entity and use dynamic cast. If casted component is not nullptr returns it. If none of componets can be casted to ComponentType returns nullptr.
		/// </summary>
		/// <typeparam name="ComponentType"></typeparam>
		/// <returns></returns>
		template<component_concept ComponentType>
		[[nodiscard]] inline ComponentType* GetComponentDynamic() const
		{
			if (IsValid())
			{
				return GetContainer_Internal()->GetComponentDynamic<drop_const_t<ComponentType>>(*GetEntityData());
			}

			return nullptr;
		}

		/// <summary>
		/// Entity can not have multiple components of same type, bu can have components which inherits from same type. This method retrive all components which are or inherits from ComponentType. In is not efficient method, it uses dynamic cast to check if component is valid
		/// </summary>
		/// <typeparam name="ComponentType"></typeparam>
		/// <param name="components"></param>
		template<component_concept ComponentType>
		inline void GetComponentsDynamic(ecsVector<ComponentType*>& components) const
		{
			if (IsValid())
			{
				GetContainer_Internal()->GetComponentsDynamic<pure_type_t<ComponentType>>(*GetEntityData(), components);
			}
		}

		template<component_concept ComponentType>
		[[nodiscard]] inline bool HasComponent() const
		{
			return IsValid() && GetContainer_Internal()->HasComponent<pure_type_t<ComponentType>>(*GetEntityData());
		}

		template<component_concept ComponentType>
		inline bool TryGetComponent(ComponentType*& component) const
		{
			if (IsValid())
			{
				component = GetContainer_Internal()->GetComponent<pure_type_t<ComponentType>>(*GetEntityData());
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
			if (IsValid())
			{
				return GetContainer_Internal()->AddComponent<pure_type_t<ComponentType>>(*this, *GetEntityData(), std::forward<Args>(args)...);
			}

			return nullptr;
		}

		template<component_concept ComponentType>
		inline bool RemoveComponent() const
		{
			return IsValid() && GetContainer_Internal()->RemoveComponent<pure_type_t<ComponentType>>(*this);
		}

		inline bool RemoveComponent(TypeID componentTypeID) const
		{
			return IsValid() && GetContainer_Internal()->RemoveComponent(*this, componentTypeID);
		}

		[[nodiscard]] inline EntityVersion GetVersion() const
		{
			return m_Version;
		}

		[[nodiscard]] inline const Archetype* GetArchetype() const
		{
			if (IsValid())
			{
				return m_EntityData->m_Archetype;
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
			if (IsValid())
			{
				GetContainer_Internal()->SetEntityActive_NoObserver(*this, isActive);
			}
		}

		/// <summary>
		/// Destroy observers of entity and components are not invoked.
		/// </summary>
		/// <returns></returns>
		inline bool Destroy_NoObserver() const
		{
			if (IsValid())
			{
				GetContainer_Internal()->DestroyEntityInternal(*this, false);
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
			if (IsValid())
			{
				return GetContainer_Internal()->AddComponent_NoObserver<drop_const_t<ComponentType>>(*this, *GetEntityData(), std::forward<Args>(args)...);
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
			return IsValid() && GetContainer_Internal()->RemoveComponent_NoObserver<drop_const_t<ComponentType>>(*this);
		}

		/// <summary>
		/// Destroy component observers are not invoked.
		/// </summary>
		/// <returns></returns>
		inline bool RemoveComponent_NoObserver(TypeID componentTypeID) const
		{
			return IsValid() && GetContainer_Internal()->RemoveComponent_NoObserver(*this, componentTypeID);
		}

		void SetActiveOverride_NoObserver(bool bIsActiveOverride) const
		{
			if (IsValid())
			{
				GetContainer_Internal()->SetEntityActiveOverride_NoObserver(*this, bIsActiveOverride);
			}
		}

		void SetDisabledOverrideCount_NoObserver(uint32_t disabledOverrideCount) const
		{
			if (IsValid())
			{
				GetContainer_Internal()->SetEntityDisabledOverrideCount_NoObserver(*this, disabledOverrideCount);
			}
		}

		void ResetDisabledOverrideCount_NoObserver() const
		{
			if (IsValid())
			{
				GetContainer_Internal()->ResetDisabledOverrideCount_NoObserver(*this);
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
			if (IsValid())
			{
				return GetContainer_Internal()->HasTag(*m_EntityData, tagType);
			}
			return false;
		}

		template<typename TagType>
		[[nodiscard]] inline bool HasTag()const
		{
			if (IsValid())
			{
				return GetContainer_Internal()->HasTag<tag_type_t<TagType>>(*m_EntityData);
			}
			return false;
		}

		template<typename... TagType>
		[[nodiscard]] inline bool HasTags()const
		{
			if (IsValid())
			{
				return GetContainer_Internal()->HasTags<tag_type_t<TagType>...>(*m_EntityData);
			}
			return false;
		}

		template<typename TagType>
		bool AddTag() const
		{
			if (IsValid())
			{
				return GetContainer_Internal()->AddTag<tag_type_t<TagType>>(*m_EntityData);
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
			if (IsValid())
			{
				return GetContainer_Internal()->RemoveTag(*m_EntityData, tagType);
			}
			return false;
		}

		template<typename TagType>
		bool RemoveTag() const
		{
			if (IsValid())
			{
				return GetContainer_Internal()->RemoveTag<tag_type_t<TagType>>(*m_EntityData);
			}
			return false;
		}

	#pragma endregion

	private:
		ContainerLifetimeDataHandle m_LifeTimeData{};
		mutable EntityData* m_EntityData = nullptr;
		mutable EntityVersion m_Version = std::numeric_limits<EntityVersion>::max();

	private:
		inline void Set_Internal(EntityData& data)
		{
			m_EntityData = &data;
			m_Version = m_EntityData->GetVersion();
			m_LifeTimeData = m_EntityData->m_Container->m_LifeTimeData;
		}

		inline void Invalidate_WithoutLifeTimeData() const
		{
			m_EntityData = nullptr;
			m_Version = std::numeric_limits<EntityVersion>::max();
		}

		inline void SetWithoutLifeTimeDataInvalidation_Internal(EntityData& data)
		{
			m_EntityData = &data;
			m_Version = m_EntityData->GetVersion();
		}

		inline void SetLifeTimeData_Internal(const ContainerLifetimeDataHandle& lifeTimeData)
		{
			m_LifeTimeData = lifeTimeData;
		}

		Container* GetContainer_Internal() const
		{
			return m_EntityData->m_Container;
		}

		EntityData* GetEntityData() const
		{
			return m_EntityData;
		}

	};

	class ConstEntity final
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

		ConstEntity(const Entity& entity):
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
		[[nodiscard]] inline ComponentType* GetComponentDynamic() const
		{
			return m_Entity.GetComponentDynamic<ComponentType>();
		}

		/// <summary>
		/// Entity can not have multiple components of same type, bu can have components which inherits from same type. This method retrive all components which are or inherits from ComponentType. In is not efficient method, it uses dynamic cast to check if component is valid
		/// </summary>
		/// <typeparam name="ComponentType"></typeparam>
		/// <param name="components"></param>
		template<component_concept ComponentType>
		inline void GetComponentsDynamic(ecsVector<ComponentType*>& components) const
		{
			m_Entity.GetComponentDynamic<ComponentType>(components);
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

	private:
		inline void Set(EntityData& data)
		{
			m_Entity.Set_Internal(data);
		}

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