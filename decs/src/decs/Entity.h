#pragma once
#include "Core.h"
#include "Container.h"
#include "Type.h"
#include "Archetypes/Archetype.h"
#include "Hash.h"

namespace decs
{
	class Entity final
	{
		template<TComponentConcept ...>
		friend class Query;
		template<TComponentConcept ...>
		friend class MultiQuery;
		friend class Container;
		template<typename TComponent>
		friend class ComponentRef;
		friend class ComponentBaseRef;
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
			m_Version(entityData.GetVersion())
		{

		}

		Entity(EntityData* entityData):
			m_EntityData(entityData),
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

		[[nodiscard]] inline bool IsValid() const
		{
			return m_EntityData != nullptr && m_EntityData->IsAliveWithVersion(m_Version);
		}

		[[nodiscard]] inline bool IsNull() const
		{
			return !IsValid();
		}

		[[nodiscard]] inline EntityID GetID() const
		{
			if (IsValid())
			{
				return m_EntityData->GetID();
			}
			return std::numeric_limits< EntityID>::max();
		}

		[[nodiscard]] inline Container* GetContainer() const
		{
			if (IsValid())
			{
				return m_EntityData->m_Container;
			}
			return nullptr;
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

		template<TComponentConcept TComponent>
		[[nodiscard]] inline TComponent* GetComponent() const
		{
			if (IsValid())
			{
				return GetContainer_Internal()->GetComponent<drop_const_t<TComponent>>(*GetEntityData());
			}

			return nullptr;
		}

		template<TComponentConcept TComponent>
		[[nodiscard]] inline bool HasComponent() const
		{
			return IsValid() && GetContainer_Internal()->HasComponent<drop_const_t<TComponent>>(*GetEntityData());
		}

		template<TComponentConcept TComponent>
		inline bool TryGetComponent(TComponent*& component) const
		{
			if (IsValid())
				component = GetContainer_Internal()->GetComponent<drop_const_t<TComponent>>(*GetEntityData());
			else
				component = nullptr;

			return component != nullptr;
		}

		template<TComponentConcept TComponent, typename... Args>
		inline typename TComponent* AddComponent(Args&&... args) const
		{
			if (IsValid())
				return GetContainer_Internal()->AddComponent<drop_const_t<TComponent>>(*this, *GetEntityData(), std::forward<Args>(args)...);

			return nullptr;
		}

		template<TComponentConcept TComponent>
		inline bool RemoveComponent() const
		{
			return IsValid() && GetContainer_Internal()->RemoveComponent<drop_const_t<TComponent>>(*this);
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

	#pragma region TAGS:
	public:
		[[nodiscard]] inline bool HasTag(TypeID tagType)const
		{
			if (IsValid())
			{
				return GetContainer()->HasTag(*m_EntityData, tagType);
			}
			return false;
		}

		template<TTagConcept TTag>
		[[nodiscard]] inline bool HasTag()const
		{
			if (IsValid())
			{
				return GetContainer()->HasTag<TTag>(*m_EntityData);
			}
			return false;
		}

		template<TTagConcept TTag>
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

		template<TTagConcept TTag>
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
		mutable EntityVersion m_Version = std::numeric_limits<EntityVersion>::max();

	private:
		inline void Invalidate_WithoutLifeTimeData() const
		{
			m_EntityData = nullptr;
			m_Version = std::numeric_limits<EntityVersion>::max();
		}

		inline void Set_Internal(EntityData& data)
		{
			m_EntityData = &data;
			m_Version = m_EntityData->GetVersion();
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

}

template<>
struct std::hash<decs::Entity>
{
	std::size_t operator()(const decs::Entity& entity) const
	{
		uint64_t entityDataHash = std::hash<decs::EntityData*>{}(entity.m_EntityData);
		uint64_t entityVersionHash = std::hash<decs::EntityVersion>{}(entity.GetVersion());

		return decs::hash::Combine(entityDataHash, entityVersionHash);
	}
};
