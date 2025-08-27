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

		Entity(const EntityDataHandle& entityDataHandle):
			m_Handle(entityDataHandle),
			m_Version(entityDataHandle.GetEntityData() != nullptr ? entityDataHandle.GetEntityData()->GetVersion() : std::numeric_limits<EntityVersion>::max())
		{

		}

	public:
		bool operator==(const Entity& rhs)const
		{
			return this->m_Handle == rhs.m_Handle && this->m_Version == rhs.m_Version;
		}

		inline bool IsValid() const
		{
			auto entityData = m_Handle.GetEntityData();
			return entityData != nullptr && m_Version == entityData->GetVersion() && entityData->IsAlive();
		}

		inline bool IsNull() const
		{
			return !IsValid();
		}

		inline bool IsInDestruction() const
		{
			if (IsValid())
			{
				return m_Handle.GetEntityData()->IsInDestructionOrDelayedToDestruction();
			}
			return false;
		}

		inline EntityID GetID() const
		{
			if (IsValid())
			{
				return m_Handle.GetEntityData()->GetID();
			}
			return std::numeric_limits< EntityID>::max();
		}

		inline Container* GetContainer() const
		{
			if (IsValid())
			{
				return m_Handle.GetEntityData()->m_Container;
			}
			return nullptr;
		}

		inline bool IsActive() const
		{
			return IsValid() && m_Handle.GetEntityData()->IsActive();
		}

		inline void SetActive(const bool& isActive) const
		{
			if (IsValid())
			{
				GetContainer_Internal()->SetEntityActive(*this, isActive);
			}
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
				return m_Handle.GetEntityData()->GetComponentOnlyCount();
			}
			return 0;
		}

		inline uint32_t GetComponentAndTagCount() const
		{
			if (IsValid())
			{
				return m_Handle.GetEntityData()->GetComponentAndTagCount();
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
				return GetContainer_Internal()->GetComponentAtIndex(*m_Handle.GetEntityData(), componentIndex);
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
				return GetContainer_Internal()->GetComponent(*m_Handle.GetEntityData(), componentType);
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
				return m_Handle.GetEntityData()->m_Archetype;
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

	#pragma endregion

	#pragma region TAGS:
	public:
		inline bool HasTag(TypeID tagType)const
		{
			if (IsValid())
			{
				return GetContainer()->HasTag(*m_Handle.GetEntityData(), tagType);
			}
			return false;
		}

		template<typename TTag>
		inline bool HasTag()const
		{
			if (IsValid())
			{
				return GetContainer()->HasTag<TTag>(*m_Handle.GetEntityData());
			}
			return false;
		}

		template<typename TTag>
		bool AddTag()const
		{
			if (IsValid())
			{
				return GetContainer()->AddTag<TTag>(*m_Handle.GetEntityData());
			}
			return false;
		}

		bool RemoveTag(TypeID tagType)const
		{
			if (IsValid())
			{
				return GetContainer()->RemoveTag(*m_Handle.GetEntityData(), tagType);
			}
			return false;
		}

		template<typename TTag>
		bool RemoveTag() const
		{
			if (IsValid())
			{
				return GetContainer()->RemoveTag<TTag>(*m_Handle.GetEntityData());
			}
			return false;
		}

	#pragma endregion

	private:
		mutable EntityDataHandle m_Handle{};
		mutable EntityVersion m_Version = std::numeric_limits<EntityVersion>::max();

	private:
		inline void Set_Internal(const EntityDataHandle& data)
		{
			m_Handle = data;
			m_Version = data.GetEntityData() != nullptr ? data.GetEntityData()->GetVersion() : std::numeric_limits<EntityVersion>::max();
		}

		inline void Invalidate() const
		{
			m_Handle = {};
			m_Version = std::numeric_limits<EntityVersion>::max();
		}

		Container* GetContainer_Internal() const
		{
			return m_Handle.GetEntityData()->m_Container;
		}

		EntityData* GetEntityData() const
		{
			return m_Handle.GetEntityData();
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

		inline bool IsValid() const
		{
			return m_Entity.IsValid();
		}

		inline bool IsNull() const
		{
			return m_Entity.IsNull();
		}

		inline bool IsActive() const
		{
			return m_Entity.IsActive();
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
		inline void Set(const EntityDataHandle& data)
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
		uint64_t entityDataHash = std::hash<decs::EntityData*>{}(entity.m_Handle.GetEntityData());
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