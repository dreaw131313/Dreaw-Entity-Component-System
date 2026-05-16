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

		template<light_component_concept TComponent>
		[[nodiscard]] inline TComponent* GetComponent() const
		{
			if (IsValid())
			{
				return GetContainer_Internal()->GetComponent<drop_const_t<TComponent>>(*GetEntityData());
			}

			return nullptr;
		}

		template<light_component_concept... ComponentTypes>
		[[nodiscard]] inline std::tuple<ComponentTypes*...> GetComponents()
		{
			if (IsValid())
			{
				return GetContainer()->GetComponents<ComponentTypes...>(*m_EntityData);
			}
			return { static_cast<ComponentTypes*>(nullptr) ... };
		}

		template<light_component_concept TComponent>
		[[nodiscard]] inline bool HasComponent() const
		{
			return IsValid() && GetContainer_Internal()->HasComponent<drop_const_t<TComponent>>(*GetEntityData());
		}

		template<light_component_concept TComponent>
		inline bool TryGetComponent(TComponent*& component) const
		{
			if (IsValid())
				component = GetContainer_Internal()->GetComponent<drop_const_t<TComponent>>(*GetEntityData());
			else
				component = nullptr;

			return component != nullptr;
		}

		template<light_component_concept TComponent, typename... Args>
		inline typename TComponent* AddComponent(Args&&... args) const
		{
			if (IsValid())
				return GetContainer_Internal()->AddComponent<drop_const_t<TComponent>>(*this, *GetEntityData(), std::forward<Args>(args)...);

			return nullptr;
		}

		template<light_component_concept TComponent>
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
		/// <summary>
		/// 
		/// </summary>
		/// <param name="tagType">it must be type id acquired by Type<decs::tag<TagType>::ID()></param>
		/// <returns></returns>
		[[nodiscard]] inline bool HasTag(TypeID tagType)const
		{
			if (IsValid())
			{
				return GetContainer()->HasTag(*m_EntityData, tagType);
			}
			return false;
		}

		template<typename TagType>
		[[nodiscard]] inline bool HasTag()const
		{
			if (IsValid())
			{
				if constexpr (tag_concept<TagType>)
				{
					return GetContainer()->HasTag<TagType>(*m_EntityData);
				}
				else
				{
					return GetContainer()->HasTag<tag<TagType>>(*m_EntityData);
				}

			}
			return false;
		}

		template<typename... TagType>
		[[nodiscard]] inline bool HasTags()const
		{
			if (IsValid())
			{
				return GetContainer()->HasTags<tag_type_t<TagType>...>(*m_EntityData);
			}
			return false;
		}

		template<typename TagType>
		bool AddTag() const
		{
			if (IsValid())
			{
				if constexpr (tag_concept<TagType>)
				{
					return GetContainer()->AddTag<TagType>(*m_EntityData);
				}
				else
				{
					return GetContainer()->AddTag<tag<TagType>>(*m_EntityData);
				}
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
				return GetContainer()->RemoveTag(*m_EntityData, tagType);
			}
			return false;
		}

		template<typename TagType>
		bool RemoveTag() const
		{
			if (IsValid())
			{
				if constexpr (tag_concept<TagType>)
				{
					return GetContainer()->RemoveTag<TagType>(*m_EntityData);
				}
				else
				{
					return GetContainer()->RemoveTag<tag<TagType>>(*m_EntityData);
				}
			}
			return false;
		}

	#pragma endregion

	#pragma region FILTERS:
	public:
		template<typename FilterType>
		bool SetFilter(const filter_data_t<FilterType>& filterData) const
		{
			if (IsValid())
			{
				return GetContainer()->SetFilter<filter_type_t<FilterType>>(*m_EntityData, filterData);
			}
			return false;
		}

		/// <summary>
		/// 
		/// </summary>
		/// <param name="filterTypeID">must be type id obtained by Type<filter<T>::ID()</param>
		/// <returns></returns>
		bool RemoveFilter(TypeID filterTypeID) const
		{
			if (IsValid())
			{
				return GetContainer()->RemoveFilter(*m_EntityData, filterTypeID);
			}
			return false;
		}

		template<typename FilterType>
		bool RemoveFilter() const
		{
			if (IsValid())
			{
				return GetContainer()->RemoveFilter<filter_type_t<FilterType>>(*m_EntityData);
			}
			return false;
		}

		template<typename FilterType>
		[[nodiscard]] const FilterType* GetFilter() const
		{
			if (IsValid())
			{
				return GetContainer()->GetFilter<filter_type_t<FilterType>>(*m_EntityData);
			}
			return nullptr;
		}
		/// <summary>
		/// 
		/// </summary>
		/// <param name="filterID">must be type id obtained by Type<filter<T>::ID()</param>
		/// <returns></returns>
		[[nodiscard]] bool HasFilter(TypeID filterID)const
		{
			if (IsValid())
			{
				return GetContainer()->HasFilter(*m_EntityData, filterID);
			}
			return false;
		}

		template<typename FilterType>
		[[nodiscard]] bool HasFilter()const
		{
			if (IsValid())
			{
				return GetContainer()->HasFilter<filter_type_t<FilterType>>(*m_EntityData);
			}
			return false;
		}

		template<typename FilterType>
		[[nodiscard]] bool HasFilter(const filter_data_t<FilterType>& filterData)const
		{
			if (IsValid())
			{
				return GetContainer()->HasFilter<filter_type_t<FilterType>>(*m_EntityData, filterData);
			}
			return false;
		}

		template<typename... FilterType>
		[[nodiscard]] bool HasFilters()const
		{
			if (IsValid())
			{
				auto container = GetContainer();
				return ((container->HasFilter<filter_type_t<FilterType>>(*m_EntityData)) && ...);
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
