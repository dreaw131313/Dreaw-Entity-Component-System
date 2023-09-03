#pragma once
#include "Core.h"
#include "Containers/TChunkedVector.h"
#include "StableContainer.h"

namespace decs
{
	class PackedContainerBase
	{
	public:
		PackedContainerBase()
		{

		}

		virtual ~PackedContainerBase()
		{

		}

		inline virtual PackedContainerBase* CreateOwnEmptyCopy() const noexcept = 0;

		inline virtual void PopBack() = 0;
		inline virtual void Clear() = 0;
		inline virtual void ShrinkToFit() = 0;
		inline virtual uint64_t Capacity() = 0;
		inline virtual uint64_t Size() = 0;
		inline virtual void Reserve(uint64_t newCapacity) = 0;

		inline virtual void* GetComponentPtrAsVoid(uint64_t index) noexcept = 0;

		inline virtual void* GetComponentDataAsVoid(uint64_t index) noexcept = 0;

		inline virtual void RemoveSwapBack(uint64_t index) noexcept = 0;
		inline virtual void EmplaceFromVoid(void* data) noexcept = 0;
		inline virtual void MoveEmplaceBackFromVoid(void* data) noexcept = 0;
	};

	template<typename ComponentType>
	class PackedContainer final : public PackedContainerBase
	{
		friend class Container;
		friend class Archetype;
	private:
		std::vector<ComponentType> m_Data;

	public:
		PackedContainer()
		{

		}

		~PackedContainer()
		{

		}

		virtual PackedContainerBase* CreateOwnEmptyCopy() const noexcept override
		{
			return new PackedContainer<ComponentType>();
		}

		inline ComponentType& GetAsRef(uint64_t index) noexcept
		{
			return m_Data[index];
		}

		inline ComponentType* GetAsPtr(uint64_t index)
		{
			return &m_Data[index];
		}

		inline virtual void PopBack() override
		{
			if (m_Data.size() > 0) { m_Data.pop_back(); }
		}
		inline virtual void Clear() override { m_Data.clear(); }
		inline virtual void ShrinkToFit() override
		{
			m_Data.shrink_to_fit();
		}
		inline virtual uint64_t Capacity() override { return m_Data.capacity(); }
		inline virtual uint64_t Size() override { return m_Data.size(); }
		inline virtual void Reserve(uint64_t newCapacity) override { m_Data.reserve(newCapacity); }

		inline virtual void* GetComponentPtrAsVoid(uint64_t index) noexcept override
		{
			return &m_Data[index];
		}

		inline virtual void* GetComponentDataAsVoid(uint64_t index) noexcept override
		{
			return &m_Data[index];
		}

		inline virtual void EmplaceFromVoid(void* data)  noexcept override
		{
			m_Data.emplace_back(*reinterpret_cast<ComponentType*>(data));
		}

		inline virtual void RemoveSwapBack(uint64_t index)noexcept override
		{
			if (m_Data.size() > 0)
			{
				if (m_Data.size() > 1) m_Data[index] = std::move(m_Data.back());
				m_Data.pop_back();
			}
		}

		inline virtual void MoveEmplaceBackFromVoid(void* data) noexcept override
		{
			m_Data.push_back(std::move(*reinterpret_cast<ComponentType*>(data)));
		}
	};

	template<typename ComponentType>
	class PackedContainer<decs::stable<ComponentType>> final : public PackedContainerBase
	{
		friend class Container;
		friend class Archetype;
	private:
		std::vector<StableComponentRef> m_Data;

	public:
		PackedContainer()
		{

		}

		~PackedContainer()
		{

		}

		virtual PackedContainerBase* CreateOwnEmptyCopy() const noexcept override
		{
			return new PackedContainer<decs::stable<ComponentType>>();
		}

		inline virtual void PopBack() override
		{
			if (m_Data.size() > 0)
			{
				m_Data.pop_back();
			}
		}

		inline virtual void Clear() override
		{
			m_Data.clear();
		}

		inline virtual void ShrinkToFit() override
		{
			m_Data.shrink_to_fit();
		}

		inline virtual uint64_t Capacity() override
		{
			return m_Data.capacity();
		}

		inline virtual uint64_t Size() override
		{
			return m_Data.size();
		}

		inline virtual void Reserve(uint64_t newCapacity) override
		{
			m_Data.reserve(newCapacity);
		}

		inline virtual void* GetComponentPtrAsVoid(uint64_t index) noexcept override
		{
			return m_Data[index].m_ComponentPtr;
		}

		inline virtual void* GetComponentDataAsVoid(uint64_t index) noexcept override
		{
			return &m_Data[index];
		}

		inline virtual void EmplaceFromVoid(void* data) noexcept override
		{
			StableComponentRef* newElement = reinterpret_cast<StableComponentRef*>(data);
			m_Data.emplace_back(newElement->m_ComponentPtr, newElement->m_ChunkIndex, newElement->m_Index);
		}

		inline virtual void MoveEmplaceBackFromVoid(void* data) noexcept override
		{
			StableComponentRef* newElement = reinterpret_cast<StableComponentRef*>(data);
			m_Data.emplace_back(newElement->m_ComponentPtr, newElement->m_ChunkIndex, newElement->m_Index);
		}

		inline virtual void RemoveSwapBack(uint64_t index)noexcept override
		{
			uint64_t dataSize = m_Data.size();
			if (dataSize > 0)
			{
				if (dataSize > 1)
				{
					m_Data[index] = m_Data.back();
				}
				m_Data.pop_back();
			}
		}

		inline ComponentType& GetAsRef(uint64_t index) noexcept
		{
			return *static_cast<ComponentType*>(m_Data[index].m_ComponentPtr);
		}

		inline ComponentType* GetAsPtr(uint64_t index)
		{
			return static_cast<ComponentType*>(m_Data[index].m_ComponentPtr);
		}

		inline StableComponentRef& EmplaceBack(ComponentType* componentPtr, uint64_t chunkIndex, uint64_t elementIndex)
		{
			return m_Data.emplace_back(componentPtr, chunkIndex, elementIndex);
		}
	};
}