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

		inline virtual void* GetComponentAsVoid(const uint64_t& index) = 0;
		inline virtual void* GetChunkData(const uint64_t& chunkIndex) const noexcept = 0;
		inline virtual uint32_t GetChunkSize(const uint64_t& chunkIndex) const noexcept = 0;
		inline virtual uint32_t GetChunksCount() const noexcept = 0;
		inline virtual PackedContainerBase* CreateOwnEmptyCopy(const uint64_t& chunkSize) const noexcept = 0;
		inline virtual void* EmplaceFromVoid(void* data) noexcept = 0;
		inline virtual void RemoveSwapBack(const uint64_t& index) = 0;
		inline virtual void RemoveSwapBack(const uint64_t& chunkIndex, const uint64_t& elementIndex) = 0;
		inline virtual void PopBack() = 0;
		inline virtual void Clear() = 0;
	};

	template<typename DataType>
	class PackedContainer final : public PackedContainerBase
	{
	public:
		ChunkedVector<DataType> m_Data = { 1000 };

	public:
		PackedContainer()
		{

		}

		PackedContainer(const uint64_t& bucketSize) :
			m_Data(bucketSize)
		{

		}

		~PackedContainer()
		{

		}

		inline virtual void* GetComponentAsVoid(const uint64_t& index)
		{
			return &m_Data[index];
		}

		inline virtual void* GetChunkData(const uint64_t& chunkIndex)const noexcept override
		{
			return m_Data.GetChunk(chunkIndex);
		}

		inline virtual uint32_t GetChunkSize(const uint64_t& chunkIndex)const noexcept override
		{
			return (uint32_t)m_Data.GetChunkSize(chunkIndex);
		}

		inline virtual uint32_t GetChunksCount() const noexcept override
		{
			return m_Data.ChunksCount();
		}

		virtual PackedContainerBase* CreateOwnEmptyCopy(const uint64_t& chunkSize) const noexcept override
		{
			return new PackedContainer<DataType>(chunkSize);
		}

		inline virtual void* EmplaceFromVoid(void* data)  noexcept override
		{
			DataType& newElement = m_Data.EmplaceBack(*reinterpret_cast<DataType*>(data));
			return (void*)(&newElement);
		}

		inline virtual void RemoveSwapBack(const uint64_t& index) override
		{
			m_Data.RemoveSwapBack(index);
		}

		inline virtual void RemoveSwapBack(const uint64_t& chunkIndex, const uint64_t& elementIndex) override
		{
			m_Data.RemoveSwapBack(chunkIndex, elementIndex);
		}

		inline virtual void PopBack() override
		{
			m_Data.PopBack();
		}

		inline virtual void Clear() override
		{
			m_Data.Clear();
		}
	};
}