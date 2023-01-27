#pragma once
#include "Core.h"
#include "Containers\ChunkedVector.h"

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

		inline virtual void PopBack() = 0;
		inline virtual void Clear() = 0;
		inline virtual void ShrinkToFit() = 0;
		inline virtual uint64_t Capacity() = 0;
		inline virtual uint64_t Size() = 0;
		inline virtual void Reserve(const uint64_t& newCapacity) = 0;


		inline virtual PackedContainerBase* CreateOwnEmptyCopy() const noexcept = 0;
		inline virtual void* GetComponentAsVoid(const uint64_t& index) = 0;

		inline virtual void RemoveSwapBack(const uint64_t& index) = 0;
		inline virtual void* EmplaceFromVoid(void* data) noexcept = 0;

	};

	template<typename DataType>
	class PackedContainer final : public PackedContainerBase
	{
	public:
		std::vector<DataType> m_Data;

	public:
		PackedContainer()
		{

		}

		PackedContainer(const uint64_t& chunkSize)
		{

		}

		~PackedContainer()
		{

		}

		virtual PackedContainerBase* CreateOwnEmptyCopy() const noexcept override
		{
			return new PackedContainer<DataType>();
		}

		inline virtual void PopBack() override
		{
			if (m_Data.size() > 0)
			{
				m_Data.pop_back();
			}
		}
		inline virtual void Clear() override { m_Data.clear(); }
		inline virtual void ShrinkToFit() override
		{
			m_Data.shrink_to_fit();
		}
		inline virtual uint64_t Capacity() override { return m_Data.capacity(); }
		inline virtual uint64_t Size() override { return m_Data.size(); }
		inline virtual void Reserve(const uint64_t& newCapacity) override { m_Data.reserve(newCapacity); }

		inline virtual void* GetComponentAsVoid(const uint64_t& index)
		{
			return &m_Data[index];
		}

		inline virtual void* EmplaceFromVoid(void* data)  noexcept override
		{
			DataType& newElement = m_Data.emplace_back(*reinterpret_cast<DataType*>(data));
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

	};
}