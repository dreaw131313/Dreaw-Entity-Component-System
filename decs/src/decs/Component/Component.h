#pragma once
#include "decs/Core.h"

namespace decs
{
	class Entity;
	class ChunkBase;
	class StableContainerBase;
	template<typename TComponentType>
	class StableContainer;
	template<typename TComponentType>
	class TChunk;

	class ComponentBase
	{
		friend class Container;
		template<typename>
		friend class ComponentContext;
		template<typename>
		friend class PackedContainer;
		template<typename>
		friend class StablePackedContainer;

		friend class ChunkBase;
		friend class StableContainerBase;
		template<typename>
		friend class TChunk;
		template<typename TComponentType>
		friend class StableContainer;

	public:
		ComponentBase() = default;

		ComponentBase(const ComponentBase&)
		{

		}

		ComponentBase(ComponentBase&&) noexcept
		{

		}

		ComponentBase& operator =(const ComponentBase&)
		{
			return *this;
		}

		ComponentBase& operator =(ComponentBase&&) noexcept
		{
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
		ChunkBase* m_ParentChunk = nullptr;
		uint32_t m_IndexInChunk = std::numeric_limits<uint32_t>::max();

		uint16_t m_DependencyCount = 0;
		bool m_bIsCreatedByContainer = false;
		bool m_bIsEnabledByECS = false;

	private:

		inline void SetChunkAndIndex(ChunkBase* parentChunk, uint32_t index)
		{
			m_ParentChunk = parentChunk;
			m_IndexInChunk = index;
		}

		inline uint32_t GetIndexInChunk() const
		{
			return m_IndexInChunk;
		}

		inline const ChunkBase* GetParentChunk() const
		{
			return m_ParentChunk;
		}


	private:
		inline void SetFlags(bool bIsCreated, bool bIsEnabled)
		{
			m_bIsCreatedByContainer = bIsCreated;
			m_bIsEnabledByECS = bIsEnabled;
		}
	};
}