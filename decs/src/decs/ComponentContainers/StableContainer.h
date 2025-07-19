#pragma once
#include "decs/Core.h"
#include "decs/Containers/TChunkedVector.h"
#include "decs/Type.h"

#include "decs/Component/Component.h"


namespace decs
{
	class StableContainerBase
	{
	public:
		virtual ~StableContainerBase()
		{

		}

		inline virtual TypeID GetTypeID()const noexcept = 0;

		virtual bool Destroy(ComponentBase* component) = 0;
		virtual ComponentBase* CreateFromComponentBase(ComponentBase* ptr) = 0;
		virtual uint32_t GetChunkSize() const noexcept = 0;
		virtual void Clear() = 0;
	};

	template<typename TComponentType>
	class StableContainer : public StableContainerBase
	{
	private:
		NON_COPYABLE(StableContainer);
		NON_MOVEABLE(StableContainer);

	public:
		StableContainer()
		{

		}

		StableContainer(uint32_t chunkCapacity):
			m_Allocator(chunkCapacity)
		{
		}

		~StableContainer()
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

		inline bool Destroy(ComponentBase* componentBase) override
		{
			return m_Allocator.Destroy(static_cast<TComponentType*>(componentBase));
		}

		inline bool Destroy(TComponentType* component)
		{
			return m_Allocator.Destroy(component);
		}

		inline ComponentBase* CreateFromComponentBase(ComponentBase* ptr)override
		{
			return m_Allocator.Create(*static_cast<TComponentType*>(ptr));
		}

		inline void Clear() override
		{
			m_Allocator.Clear();
		}

	private:
		TChunkAllocator<TComponentType> m_Allocator{};
	};
}