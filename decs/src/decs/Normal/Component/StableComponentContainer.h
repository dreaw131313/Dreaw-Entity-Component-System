#pragma once
#include "decs/Core/Core.h"
#include "decs/Core/Type.h"
#include "decs/Core/check_cast.h"
#include "decs/Normal/Statistics.h"

#include "Component.h"
#include "ComponentAllocator.h"

namespace decs
{
	class IStableComponentContainer
	{
	public:
		virtual ~IStableComponentContainer()
		{

		}

		inline virtual TypeID GetTypeID()const noexcept = 0;

		virtual bool Destroy(EntityComponent* component) = 0;
		virtual EntityComponent* CreateFromComponentBase(const Entity& e, const EntityComponent* ptr) = 0;
		virtual uint32_t GetChunkSize() const noexcept = 0;
		virtual void Clear() = 0;
	};

	template<typename ComponentType>
	class StableComponentContainer : 
		public IStableComponentContainer,
		private NonCopyableNonMoveable
	{
	public:
		StableComponentContainer()
		{

		}

		StableComponentContainer(uint32_t chunkCapacity):
			m_Allocator(chunkCapacity)
		{
		}

		~StableComponentContainer()
		{
		}

		inline TypeID GetTypeID()const noexcept override
		{
			return Type<ComponentType>::ID();
		}

		inline uint32_t GetChunkSize() const noexcept override
		{
			return m_Allocator.GetChunkSize();
		}

		template<typename... Args>
		inline ComponentType* Create(const Entity& e, Args&&... args)
		{
			ComponentType* t = m_Allocator.Create(std::forward<Args>(args)...);;
			if constexpr (::decs::has_ecs_on_construct<ComponentType>)
			{
				t->ECS_OnConstruct(e);
			}

			return t;
		}

		inline bool Destroy(EntityComponent* componentBase) override
		{
			return m_Allocator.Destroy(::decs::check_cast<ComponentType*>(componentBase));
		}

		inline bool Destroy(ComponentType* component)
		{
			return m_Allocator.Destroy(component);
		}

		inline EntityComponent* CreateFromComponentBase(const Entity& e, const EntityComponent* ptr)override
		{
			return Create(e, *decs::check_cast<const ComponentType*>(ptr));
		}

		inline void Clear() override
		{
			m_Allocator.Clear();
		}

		void FillStatistics(ComponentStatistics& stats) const
		{
			stats.m_CreatedComponentCount = m_Allocator.GetCreatedComponentCount();
			stats.m_ChunkCapacity = m_Allocator.GetChunkSize();
			stats.m_AllocatedChunks = m_Allocator.GetChunkCount();
			stats.m_AllocatorCapacity = m_Allocator.GetCapacity();
		}

	private:
		TComponentAllocator<ComponentType> m_Allocator{};
	};
}