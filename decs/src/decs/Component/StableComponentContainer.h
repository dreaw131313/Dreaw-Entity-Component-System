#pragma once
#include "decs/Core.h"
#include "decs/Containers/TChunkedVector.h"
#include "decs/Type.h"

#include "decs/Component/Component.h"


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
		virtual EntityComponent* CreateFromComponentBase(const EntityComponent* ptr) = 0;
		virtual uint32_t GetChunkSize() const noexcept = 0;
		virtual void Clear() = 0;
	};

	template<typename TComponentType>
	class StableComponentContainer : public IStableComponentContainer
	{
	private:
		NON_COPYABLE(StableComponentContainer);
		NON_MOVEABLE(StableComponentContainer);

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
			return Type<TComponentType>::ID();
		}

		inline uint32_t GetChunkSize() const noexcept override
		{
			return m_Allocator.GetChunkSize();
		}

		template<typename... Args>
		inline TComponentType* Create(Args&&... args)
		{
			return m_Allocator.Create(std::forward<Args>(args)...);
		}

		inline bool Destroy(EntityComponent* componentBase) override
		{
			return m_Allocator.Destroy(static_cast<TComponentType*>(componentBase));
		}

		inline bool Destroy(TComponentType* component)
		{
			return m_Allocator.Destroy(component);
		}

		inline EntityComponent* CreateFromComponentBase(const EntityComponent* ptr)override
		{
			return m_Allocator.Create(*static_cast<const TComponentType*>(ptr));
		}

		inline void Clear() override
		{
			m_Allocator.Clear();
		}

	private:
		TChunkAllocator<TComponentType> m_Allocator{};
	};
}