#pragma once
#include "Core.h"
#include "Container.h"
#include "Type.h"
#include "Archetypes/Archetype.h"

namespace decs
{
	class Container;

	class Entity final
	{
		template<typename ...>
		friend class Query;
		template<typename ...>
		friend class MultiQuery;
		friend class Container;
		template<typename TComponent>
		friend class ComponentRef;
		friend class ComponentBaseRef;
		template<typename>
		friend class ContainerSerializer;
		friend class ContainerSerializerComplex;
		friend class ContainerIterator;

		friend class ConstEntity;

		friend struct std::hash<decs::Entity>;

	public:
		Entity()
		{

		}

		Entity(EntityData& entityData):
			m_EntityData(&entityData),
			m_LifeTimeData(entityData.m_Container->m_EntitiesLifeTimeData),
			m_Version(entityData.GetVersion())
		{

		}

	public:
		bool operator==(const Entity& rhs)const
		{
			return this->m_EntityData == rhs.m_EntityData && this->m_Version == rhs.m_Version;
		}

		inline bool IsValid() const
		{
			return m_LifeTimeData.IsValid() && m_LifeTimeData->IsAlive() && m_EntityData != nullptr && m_EntityData->IsAliveWithVersion(m_Version);
		}

		inline bool IsNull() const
		{
			return !IsValid();
		}

		inline bool IsInDestruction() const
		{
			if (IsValid())
			{
				return m_EntityData->IsInDestructionOrDelayedToDestruction();
			}
			return false;
		}

		inline EntityID GetID() const
		{
			if (IsValid())
			{
				return m_EntityData->GetID();
			}
			return std::numeric_limits< EntityID>::max();
		}

		inline Container* GetContainer() const
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
		inline bool IsActive() const
		{
			return IsValid() && m_EntityData->IsActive();
		}

		/// <summary>
		/// 
		/// </summary>
		/// <returns>Active state without disable overrides taken into account</returns>
		inline bool IsActiveFlag() const
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

		void SetEntityActiveOverride(bool bIsActiveOverride) const
		{
			if (IsValid())
			{
				GetContainer()->SetEntityActiveOverride(*this, bIsActiveOverride);
			}
		}

		void SetEntityDisabledOverrideCount(uint32_t disabledOverrideCount) const
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

		uint32_t GetEntityActiveOverrideCount() const
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
				Invalidate();
				return true;
			}
			return false;
		}

		inline uint32_t GetComponentCount() const
		{
			if (IsValid())
			{
				return m_EntityData->GetComponentOnlyCount();
			}
			return 0;
		}

		inline uint32_t GetComponentAndTagCount() const
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
		inline ComponentBase* GetComponentAtIndex(uint32_t componentIndex) const
		{
			if (IsValid())
			{
				return GetContainer_Internal()->GetComponentAtIndex(*m_EntityData, componentIndex);
			}
			return nullptr;
		}

		template<typename TComponent>
		inline TComponent* GetComponent() const
		{
			if (IsValid())
			{
				return GetContainer_Internal()->GetComponent<TComponent>(*GetEntityData());
			}

			return nullptr;
		}

		inline ComponentBase* GetComponent(TypeID componentType) const
		{
			if (IsValid())
			{
				return GetContainer_Internal()->GetComponent(*m_EntityData, componentType);
			}

			return nullptr;
		}

		/// <summary>
		/// Iterates over all components on entity and use dynamic cast. If casted component is not nullptr returns it. If none of componets can be casted to TComponent returns nullptr.
		/// </summary>
		/// <typeparam name="TComponent"></typeparam>
		/// <returns></returns>
		template<typename TComponent>
		inline TComponent* GetComponentDynamic() const
		{
			if (IsValid())
				return GetContainer_Internal()->GetComponentDynamic<TComponent>(*GetEntityData());

			return nullptr;
		}

		/// <summary>
		/// Entity can not have multiple components of same type, bu can have components which inherits from same type. This method retrive all components which are or inherits from TComponent. In is not efficient method, it uses dynamic cast to check if component is valid
		/// </summary>
		/// <typeparam name="TComponent"></typeparam>
		/// <param name="components"></param>
		template<typename TComponent>
		inline void GetComponentsDynamic(std::vector<TComponent*>& components) const
		{
			if (IsValid())
			{
				GetContainer_Internal()->GetComponentsDynamic<TComponent>(*GetEntityData(), components);
			}
		}

		template<typename TComponent>
		inline bool HasComponent() const
		{
			return IsValid() && GetContainer_Internal()->HasComponent<TComponent>(*GetEntityData());
		}

		template<typename TComponent>
		inline bool TryGetComponent(TComponent*& component) const
		{
			if (IsValid())
				component = GetContainer_Internal()->GetComponent<TComponent>(*GetEntityData());
			else
				component = nullptr;

			return component != nullptr;
		}

		template<typename TComponent, typename... Args>
		inline typename TComponent* AddComponent(Args&&... args) const
		{
			if (IsValid())
				return GetContainer_Internal()->AddComponent<TComponent>(*this, *GetEntityData(), std::forward<Args>(args)...);

			return nullptr;
		}

		template<typename TComponent>
		inline bool RemoveComponent() const
		{
			return IsValid() && GetContainer_Internal()->RemoveComponent<TComponent>(*this);
		}

		inline bool RemoveComponent(TypeID componentTypeID) const
		{
			return IsValid() && GetContainer_Internal()->RemoveComponent(*this, componentTypeID);
		}

		template<typename TComponent, typename TCallable>
		inline bool RemoveComponent_If(TCallable&& canRemoveFunc) const
		{
			return IsValid() && GetContainer_Internal()->RemoveComponent_If<TComponent>(*GetEntityData(), canRemoveFunc);
		}

		inline EntityVersion GetVersion() const
		{
			return m_Version;
		}

		inline const Archetype* GetArchetype() const
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
		inline void SetActive_NoCallback(const bool& isActive) const
		{
			if (IsValid())
			{
				GetContainer_Internal()->SetEntityActive_NoCallback(*this, isActive);
			}
		}

		/// <summary>
		/// Destroy observers of entity and components are not invoked.
		/// </summary>
		/// <returns></returns>
		inline bool Destroy_NoCallback() const
		{
			if (IsValid())
			{
				GetContainer_Internal()->DestroyEntityInternal(*this, false);
				Invalidate();
				return true;
			}
			return false;
		}

		/// <summary>
		/// Add component observers are not invoked.
		/// </summary>
		/// <returns></returns>
		template<typename TComponent, typename... Args>
		inline typename TComponent* AddComponent_NoCallback(Args&&... args) const
		{
			if (IsValid())
				return GetContainer_Internal()->AddComponent_NoCallback<TComponent>(*this, *GetEntityData(), std::forward<Args>(args)...);

			return nullptr;
		}

		/// <summary>
		/// Destroy component observers are not invoked.
		/// </summary>
		/// <returns></returns>
		template<typename TComponent>
		inline bool RemoveComponent_NoCallback() const
		{
			return IsValid() && GetContainer_Internal()->RemoveComponent_NoCallback<TComponent>(*this);
		}

		/// <summary>
		/// Destroy component observers are not invoked.
		/// </summary>
		/// <returns></returns>
		inline bool RemoveComponent_NoCallback(TypeID componentTypeID) const
		{
			return IsValid() && GetContainer_Internal()->RemoveComponent_NoCallback(*this, componentTypeID);
		}

		void SetEntityActiveOverride_NoCallback(bool bIsActiveOverride) const
		{
			if (IsValid())
			{
				GetContainer()->SetEntityActiveOverride_NoCallback(*this, bIsActiveOverride);
			}
		}

		void SetEntityDisabledOverrideCount_NoCallback(uint32_t disabledOverrideCount) const
		{
			if (IsValid())
			{
				GetContainer()->SetEntityDisabledOverrideCount_NoCallback(*this, disabledOverrideCount);
			}
		}

		void ResetDisabledOverrideCount_NoCallback() const
		{
			if (IsValid())
			{
				GetContainer()->ResetDisabledOverrideCount_NoCallback(*this);
			}
		}

	#pragma endregion

	#pragma region TAGS:
	public:
		inline bool HasTag(TypeID tagType)const
		{
			if (IsValid())
			{
				return GetContainer()->HasTag(*m_EntityData, tagType);
			}
			return false;
		}

		template<typename TTag>
		inline bool HasTag()const
		{
			if (IsValid())
			{
				return GetContainer()->HasTag<TTag>(*m_EntityData);
			}
			return false;
		}

		template<typename TTag>
		bool AddTag()const
		{
			if (IsValid())
			{
				return GetContainer()->AddTag<TTag>(*m_EntityData);
			}
			return false;
		}

		bool RemoveTag(TypeID tagType)const
		{
			if (IsValid())
			{
				return GetContainer()->RemoveTag(*m_EntityData, tagType);
			}
			return false;
		}

		template<typename TTag>
		bool RemoveTag() const
		{
			if (IsValid())
			{
				return GetContainer()->RemoveTag<TTag>(*m_EntityData);
			}
			return false;
		}

	#pragma endregion

	private:
		mutable EntityData* m_EntityData = nullptr;
		mutable TRefCounterHandle<EnityLifeTimeData> m_LifeTimeData{};
		mutable EntityVersion m_Version = std::numeric_limits<EntityVersion>::max();

	private:
		inline void Set_Internal(EntityData& data)
		{
			m_EntityData = &data;
			m_Version = m_EntityData->GetVersion();
			m_LifeTimeData = m_EntityData->m_Container->m_EntitiesLifeTimeData;
		}

		inline void Invalidate() const
		{
			m_EntityData = nullptr;
			m_LifeTimeData.Reset();
			m_Version = std::numeric_limits<EntityVersion>::max();
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
		template<typename ...>
		friend class Query;
		template<typename ...>
		friend class MultiQuery;
		friend class Container;
		template<typename TComponent>
		friend class ComponentRef;
		friend class ComponentBaseRef;
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

		bool operator==(const ConstEntity& rhs)const
		{
			return rhs.m_Entity == m_Entity;
		}

		bool operator==(const Entity& rhs)const
		{
			return rhs == m_Entity;
		}

		inline bool IsValid() const noexcept
		{
			return m_Entity.IsValid();
		}

		inline bool IsNull() const noexcept
		{
			return m_Entity.IsNull();
		}

		inline bool IsActive() const noexcept
		{
			return m_Entity.IsActive();
		}

		inline bool IsActiveFlag() const noexcept
		{
			return m_Entity.IsActiveFlag();
		}

		inline EntityID GetID() const
		{
			return m_Entity.GetID();
		}

		inline Container* GetContainer() const
		{
			return m_Entity.GetContainer();
		}

		template<typename TComponent>
		inline bool HasComponent() const
		{
			return m_Entity.HasComponent<TComponent>();
		}

		template<typename TComponent>
		inline TComponent* GetComponent() const
		{
			return m_Entity.GetComponent<TComponent>();
		}

		template<typename TComponent>
		inline TComponent* GetComponentDynamic() const
		{
			return m_Entity.GetComponentDynamic<TComponent>();
		}

		/// <summary>
		/// Entity can not have multiple components of same type, bu can have components which inherits from same type. This method retrive all components which are or inherits from TComponent. In is not efficient method, it uses dynamic cast to check if component is valid
		/// </summary>
		/// <typeparam name="TComponent"></typeparam>
		/// <param name="components"></param>
		template<typename TComponent>
		inline void GetComponentsDynamic(std::vector<TComponent*>& components) const
		{
			m_Entity.GetComponentDynamic<TComponent>(components);
		}
		template<typename TComponent>
		inline bool TryGetComponent(typename TComponent*& component) const
		{
			return m_Entity.TryGetComponent(component);
		}

		inline EntityVersion GetVersion() const
		{
			return m_Entity.GetVersion();
		}

		inline uint32_t GetComponentCount() const
		{
			return m_Entity.GetComponentCount();
		}

		inline const Archetype* GetArchetype() const
		{
			return m_Entity.GetArchetype();
		}

		inline bool Destroy() const
		{
			return m_Entity.Destroy();
		}

		inline bool HasTag(TypeID tagType)const
		{
			return m_Entity.HasTag(tagType);
		}

		template<typename TTag>
		inline bool HasTag()const
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


		inline void Invalidate() const
		{
			m_Entity.Invalidate();
		}

	};
}

template<>
struct std::hash<decs::Entity>
{
	std::size_t operator()(const decs::Entity& entity) const
	{
		uint64_t entityDataHash = std::hash<decs::EntityData*>{}(entity.m_EntityData);
		uint64_t entityVersionHash = std::hash<decs::EntityVersion>{}(entity.GetVersion());

		return entityDataHash ^ (entityVersionHash + 0x9e3779b9 + (entityDataHash << 6) + (entityDataHash >> 2));
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