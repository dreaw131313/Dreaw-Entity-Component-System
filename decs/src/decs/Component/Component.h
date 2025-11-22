#pragma once
#include "decs/Core.h"

#include "decs/ComponentContainers/ChunkAllocator.h"

namespace decs
{
	class Entity;
	class StableContainerBase;
	template<typename TComponentType>
	class StableContainer;

	class ComponentBase : public ChunkAllocatorResource
	{
		friend class Container;
		template<typename>
		friend class ComponentContext;
		template<typename>
		friend class PackedContainer;
		template<typename>
		friend class StablePackedContainer;

		friend class StableContainerBase;
		template<typename TComponentType>
		friend class StableContainer;

	public:
		ComponentBase() = default;

		ComponentBase(const ComponentBase& other):
			ChunkAllocatorResource(other)
		{

		}

		ComponentBase(ComponentBase&& other) noexcept:
			ChunkAllocatorResource(std::move(other))
		{

		}

		ComponentBase& operator =(const ComponentBase& other)
		{
			ChunkAllocatorResource::operator=(other);
			return *this;
		}

		ComponentBase& operator=(ComponentBase&& other) noexcept
		{
			if (this != &other)
			{
				ChunkAllocatorResource::operator=(std::move(other));
			}

			return *this;
		}

		virtual ~ComponentBase() = default;

		inline bool IsCreatedByECS() const
		{
			return m_bIsCreatedByContainer;
		}

		inline bool IsEnabledByECS() const
		{
			return m_bIsEnabledByECS;
		}

		inline uint16_t GetDependecyCount() const
		{
			return m_DependencyCount;
		}

		inline void AddDependency(uint16_t dependecyCount = 1)
		{
			m_DependencyCount += dependecyCount;
		}

		inline void RemoveDependecy(uint16_t dependecyCount = 1)
		{
			if (dependecyCount > m_DependencyCount)
			{
				m_DependencyCount = 0;
			}
			else
			{
				m_DependencyCount -= dependecyCount;
			}
		}

	protected:
		/// <summary>
		/// This function is called always when adding component to entity (and when entity is spawned). Removing any other component or removing this component is forbidden because it cause undefined behavior.
		/// </summary>
		/// <param name="entity"></param>
		virtual void OnPreCreate(const Entity& entity);

	private:
		uint16_t m_DependencyCount = 0;
		bool m_bIsCreatedByContainer = false;
		bool m_bIsEnabledByECS = false;

	private:
		inline void SetFlags(bool bIsCreated, bool bIsEnabled)
		{
			m_bIsCreatedByContainer = bIsCreated;
			m_bIsEnabledByECS = bIsEnabled;
		}
	};

	template<typename TComponentType>
	concept TComponentConcept = std::derived_from<ComponentBase, TComponentType>;
}