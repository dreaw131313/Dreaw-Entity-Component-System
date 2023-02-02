#pragma once
#include "Core.h"
#include "Containers\ChunkedVector.h"

#include "StableContainer.h"

namespace decs
{
	class PackedContainerBase
	{
	public:
		PackedContainerBase()
		{

		}

		inline virtual PackedContainerBase* CreateOwnEmptyCopy() const noexcept = 0;

		inline virtual StableContainerBase* GetLinkedStableContainer() const
		{
			return nullptr;
		}

		inline virtual bool IsForStableComponents() const { return false; }

		inline virtual void PopBack() = 0;
		inline virtual void Clear() = 0;
		inline virtual void ShrinkToFit() = 0;
		inline virtual uint64_t Capacity() = 0;
		inline virtual uint64_t Size() = 0;
		inline virtual void Reserve(const uint64_t& newCapacity) = 0;

		inline virtual void* GetComponentPtrAsVoid(const uint64_t& index) noexcept = 0;

		inline virtual void* GetComponentDataAsVoid(const uint64_t& index) noexcept = 0;

		inline virtual void RemoveSwapBack(const uint64_t& index) = 0;
		inline virtual void* EmplaceFromVoid(void* data) noexcept = 0;

	};

	template<typename ComponentType>
	class PackedContainer final : public PackedContainerBase
	{
	public:
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
		inline virtual void Reserve(const uint64_t& newCapacity) override { m_Data.reserve(newCapacity); }

		inline virtual void* GetComponentPtrAsVoid(const uint64_t& index) noexcept override
		{
			return &m_Data[index];
		}

		inline virtual void* GetComponentDataAsVoid(const uint64_t& index) noexcept override
		{
			return &m_Data[index];
		}

		inline virtual void* EmplaceFromVoid(void* data)  noexcept override
		{
			ComponentType& newElement = m_Data.emplace_back(*reinterpret_cast<ComponentType*>(data));
			return (void*)(&newElement);
		}

		inline virtual void RemoveSwapBack(const uint64_t& index) override
		{
			if (m_Data.size() > 0)
			{
				if (m_Data.size() > 1) m_Data[index] = m_Data.back();
				m_Data.pop_back();
			}
		}

		inline ComponentType& GetAsRef(const uint64_t& index) noexcept
		{
			return m_Data[index];
		}

		inline ComponentType* GetAsPtr(const uint64_t& index) const noexcept
		{
			return &m_Data[index];
		}
	};

	struct StableComponentRef
	{
	public:
		uint32_t m_ChunkIndex = std::numeric_limits<uint32_t>::max();
		uint32_t m_ElementIndex = std::numeric_limits<uint32_t>::max();
		void* m_ComponentPtr = nullptr;

		StableComponentRef()
		{

		}

		StableComponentRef(const uint32_t& chunkIndex, const uint32_t& elementIndex, void* componentPtr) :
			m_ChunkIndex(chunkIndex), m_ElementIndex(elementIndex), m_ComponentPtr(componentPtr)
		{

		}
	};

	template<typename ComponentType>
	class PackedContainer<decs::Stable<ComponentType>> : public PackedContainerBase
	{
	public:
		std::vector<StableComponentRef> m_Data;
		StableContainerBase* m_LinkedStableContainer = nullptr;

	public:
		PackedContainer()
		{

		}

		~PackedContainer()
		{

		}

		virtual PackedContainerBase* CreateOwnEmptyCopy() const noexcept override
		{
			return new PackedContainer<decs::Stable<ComponentType>>();
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
		inline virtual void Reserve(const uint64_t& newCapacity) override { m_Data.reserve(newCapacity); }

		inline virtual void* GetComponentPtrAsVoid(const uint64_t& index) noexcept override
		{
			return m_Data[index].m_ComponentPtr;
		}

		inline virtual void* GetComponentDataAsVoid(const uint64_t& index) noexcept override
		{
			return &m_Data[index];
		}

		inline virtual void* EmplaceFromVoid(void* data) noexcept override
		{
			StableComponentRef& newElement = m_Data.emplace_back(*reinterpret_cast<StableComponentRef*>(data));
			return newElement.m_ComponentPtr;
		}

		inline virtual void RemoveSwapBack(const uint64_t& index) override
		{
			if (m_Data.size() > 0)
			{
				if (m_Data.size() > 1) m_Data[index] = m_Data.back();
				m_Data.pop_back();
			}
		}

		inline ComponentType& GetAsRef(const uint64_t& index) noexcept
		{
			return *static_cast<ComponentType*>(m_Data[index].m_ComponentPtr);
		}

		inline ComponentType* GetAsPtr(const uint64_t& index)const noexcept
		{
			return static_cast<ComponentType*>(m_Data[index].m_ComponentPtr);
		}
	};
}