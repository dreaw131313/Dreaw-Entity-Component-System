#pragma once
#include "LContainer.h"

namespace decs::light
{
	class Entity final
	{
		friend class light::Container;
		friend class light::ContainerIterator;
		friend class light::Iteration;

		friend struct std::hash<decs::light::Entity>;

		template<typename Components, typename Tags>
		friend struct light::EntitySpawner;

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
			return std::numeric_limits<EntityID>::max();
		}

		[[nodiscard]] inline Container* GetContainer() const
		{
			if (IsValid())
			{
				return m_EntityData->m_Container;
			}
			return nullptr;
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

		[[nodiscard]] inline uint32_t GetComponentTagCount() const
		{
			if (IsValid())
			{
				return m_EntityData->GetComponentTagCount();
			}
			return 0;
		}

		template<TLightComponentConcept TComponent>
		[[nodiscard]] inline TComponent* GetComponent() const
		{
			if (IsValid())
			{
				return GetContainer_Internal()->GetComponent<drop_const_t<TComponent>>(*GetEntityData());
			}

			return nullptr;
		}

		template<TLightComponentConcept TComponent>
		[[nodiscard]] inline bool HasComponent() const
		{
			return IsValid() && GetContainer_Internal()->HasComponent<drop_const_t<TComponent>>(*GetEntityData());
		}

		template<TLightComponentConcept TComponent>
		inline bool TryGetComponent(TComponent*& component) const
		{
			if (IsValid())
				component = GetContainer_Internal()->GetComponent<drop_const_t<TComponent>>(*GetEntityData());
			else
				component = nullptr;

			return component != nullptr;
		}

		template<TLightComponentConcept TComponent, typename... Args>
		inline typename TComponent* AddComponent(Args&&... args) const
		{
			if (IsValid())
				return GetContainer_Internal()->AddComponent<drop_const_t<TComponent>>(*this, *GetEntityData(), std::forward<Args>(args)...);

			return nullptr;
		}

		template<TLightComponentConcept TComponent>
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

		template<tag_concept TTag>
		[[nodiscard]] inline bool HasTag()const
		{
			if (IsValid())
			{
				return GetContainer()->HasTag<TTag>(*m_EntityData);
			}
			return false;
		}

		template<tag_concept TTag>
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

		template<tag_concept TTag>
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
struct std::hash<decs::light::Entity>
{
	std::size_t operator()(const decs::light::Entity& entity) const
	{
		uint64_t entityDataHash = std::hash<decs::light::EntityData*>{}(entity.m_EntityData);
		uint64_t entityVersionHash = std::hash<decs::EntityVersion>{}(entity.GetVersion());

		return decs::hash::Combine(entityDataHash, entityVersionHash);
	}
};
