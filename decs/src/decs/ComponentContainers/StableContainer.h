#pragma once
#include "decs/Core.h"
#include "decs/Containers/TChunkedVector.h"
#include "decs/Type.h"

#include "Component/Component.h"

namespace decs
{
	template<typename DataType>
	struct TChunkAllocationResult final
	{
	public:
		DataType* Data = nullptr;
		uint32_t Index = std::numeric_limits<uint32_t>::max();

	public:
		TChunkAllocationResult() {}

		TChunkAllocationResult(
			uint32_t index,
			DataType* data
		) :
			Data(data),
			Index(index)
		{

		}

		inline bool IsValid() const
		{
			return Data != nullptr;
		}
	};

	class ChunkBase
	{
	public:
		virtual uint32_t GetChunkIndex() const = 0;
	};

	template<typename DataType>
	class Chunk final : public ChunkBase
	{
		using AllocationResultType = TChunkAllocationResult<DataType>;
	public:
		uint32_t m_Index = 0;
		uint32_t m_IndexInFreeSpaces = 0;
		bool m_IsInFreeSpaces = 0;
	public:
		Chunk(uint32_t capacity) :
			m_Capacity(capacity)
		{
			m_AllocationFlags = new bool[capacity]();
			m_Data = (DataType*)::operator new(capacity * sizeof(DataType));
		}

		~Chunk()
		{
			if (m_Size > 0)
			{
				for (uint32_t i = 0; i < m_CurrentAllocationOffset; i++)
				{
					if (m_AllocationFlags[i])
					{
						m_Data[i].~DataType();
					}
				}
			}

			::operator delete(m_Data, static_cast<uint64_t>(m_Capacity) * sizeof(DataType));
			delete[] m_AllocationFlags;
		}

		virtual uint32_t GetChunkIndex() const override
		{
			return m_Index;
		}

		bool IsEmpty() const
		{
			return m_Size == 0;
		}

		bool IsFull() const { return m_Capacity == m_Size; }

		DataType& operator[](uint32_t index) const { return m_Data[index]; }

		bool IsAllocatedAt(uint32_t index) const
		{
			return m_AllocationFlags[index];
		}

		template<typename... Args>
		AllocationResultType Emplace(Args&&... args)
		{
			if (IsFull()) return AllocationResultType();

			m_Size += 1;

			if (m_FreeSpaces.size() > 0)
			{
				uint32_t freeSpaceIndex = m_FreeSpaces.back();
				m_FreeSpaces.pop_back();
				DataType* data = new(&m_Data[freeSpaceIndex])DataType(std::forward<Args>(args)...);

				m_AllocationFlags[freeSpaceIndex] = true;
				return AllocationResultType(freeSpaceIndex, data);
			}

			uint32_t allocationIndex = m_CurrentAllocationOffset;
			DataType* data = new(&m_Data[allocationIndex])DataType(std::forward<Args>(args)...);
			m_AllocationFlags[allocationIndex] = true;

			m_CurrentAllocationOffset += 1;

			return AllocationResultType(allocationIndex, data);
		}

		bool RemoveAt(uint32_t index)
		{
			if (index < m_Capacity && m_AllocationFlags[index])
			{
				m_Size -= 1;
				if (index == (m_CurrentAllocationOffset - 1))
				{
					m_CurrentAllocationOffset -= 1;
				}
				else
				{
					m_FreeSpaces.push_back(index);
				}

				if (IsEmpty())
				{
					m_FreeSpaces.clear();
					m_CurrentAllocationOffset = 0;
				}

				m_AllocationFlags[index] = false;
				m_Data[index].~DataType();
				return true;
			}
			return false;
		}

	private:
		std::vector<uint32_t> m_FreeSpaces;

		uint32_t m_Capacity = 0;
		DataType* m_Data = nullptr;
		bool* m_AllocationFlags = nullptr;

		uint32_t m_CurrentAllocationOffset = 0;
		uint32_t m_Size = 0;
	};

	class StableComponentRef
	{
	public:
		ComponentBase* m_ComponentPtr = nullptr;
		ChunkBase* m_Chunk = nullptr;
		uint32_t m_Index = std::numeric_limits<uint32_t>::max();

	public:
		StableComponentRef()
		{

		}

		StableComponentRef(
			ComponentBase* componentPtr,
			ChunkBase* chunk,
			uint32_t index
		) :
			m_ComponentPtr(componentPtr), m_Chunk(chunk), m_Index(index)
		{

		}

	};

	class StableContainerBase
	{
	public:
		virtual ~StableContainerBase()
		{

		}

		inline virtual TypeID GetTypeID()const noexcept = 0;
		virtual StableContainerBase* Clone(uint32_t withChunkSize) = 0;

		virtual bool Remove(const StableComponentRef& compRef) = 0;
		virtual StableComponentRef EmplaceFromBaseComponent(ComponentBase* ptr) = 0;
		virtual uint32_t GetChunkSize() const noexcept = 0;
		virtual void Clear() = 0;
	};

	template<typename DataType>
	class StableContainer : public StableContainerBase
	{
		using ChunkType = Chunk<DataType>;
	private:
		NON_COPYABLE(StableContainer);
		NON_MOVEABLE(StableContainer);

	public:
		StableContainer()
		{

		}

		StableContainer(uint32_t chunkCapacity) :
			m_ChunkCapacity(chunkCapacity)
		{
		}

		~StableContainer()
		{
			for (auto& chunk : m_Chunks)
			{
				if (chunk != nullptr)
				{
					delete chunk;
				}
			}
		}

		virtual TypeID GetTypeID()const noexcept override { return Type<DataType>::ID(); }

		virtual StableContainerBase* Clone(uint32_t withChunkSize) override
		{
			return new StableContainer<DataType>(withChunkSize);
		}

		virtual uint32_t GetChunkSize() const noexcept override
		{
			return m_ChunkCapacity;
		}

		template<typename... Args>
		StableComponentRef Emplace(Args&&... args)
		{
			ChunkType* chunk = GetCurrentChunk();
			auto result = chunk->Emplace(std::forward<Args>(args)...);

			if (chunk->IsFull())
			{
				RemoveChunkFromFreeSpaces(chunk);
			}

			return StableComponentRef(result.Data, chunk, result.Index);
		}

		bool Remove(const StableComponentRef& compRef) override
		{
			uint32_t chunkIndex = compRef.m_Chunk->GetChunkIndex();
			if (chunkIndex < m_Chunks.size() && m_Chunks[chunkIndex] == compRef.m_Chunk)
			{
				ChunkType* chunk = m_Chunks[chunkIndex];
				bool wasChunkFull = chunk->IsFull();
				if (chunk->RemoveAt(compRef.m_Index))
				{
					if (chunk->IsEmpty())
					{
						RemoveChunk(chunk);
					}
					else
					{
						AddChunkToFreeSpaces(chunk);
					}
					return true;
				}
			}
			return false;
		}

		virtual StableComponentRef EmplaceFromBaseComponent(ComponentBase* ptr)override
		{
			return Emplace(*static_cast<DataType*>(ptr));
		}

		virtual void Clear() override
		{
			for (auto& chunk : m_Chunks)
			{
				delete chunk;
			}
			m_CurrentChunk = nullptr;
			m_Chunks.clear();
			m_ChunksWithFreeSpace.clear();
		}

	private:
		std::vector<ChunkType*> m_Chunks;
		std::vector<ChunkType*> m_ChunksWithFreeSpace;
		ChunkType* m_CurrentChunk = nullptr;
		uint32_t m_ChunkCapacity = 1000;

	private:
		inline ChunkType* GetCurrentChunk()
		{
			if (m_CurrentChunk == nullptr || m_CurrentChunk->IsFull())
			{
				if (m_ChunksWithFreeSpace.size() != 0)
				{
					m_CurrentChunk = m_ChunksWithFreeSpace[0];
				}
				else
				{
					m_CurrentChunk = new ChunkType(m_ChunkCapacity);
					m_CurrentChunk->m_IsInFreeSpaces = true;
					m_CurrentChunk->m_IndexInFreeSpaces = static_cast<uint32_t>(m_ChunksWithFreeSpace.size());
					m_CurrentChunk->m_Index = static_cast<uint32_t>(m_Chunks.size());

					m_Chunks.push_back(m_CurrentChunk);
					m_ChunksWithFreeSpace.push_back(m_CurrentChunk);
				}
			}

			return m_CurrentChunk;
		}

		void RemoveChunk(ChunkType* chunk)
		{
			RemoveChunkFromFreeSpaces(chunk);

			if (chunk != m_Chunks.back())
			{
				auto lastChunk = m_Chunks.back();
				m_Chunks[chunk->m_Index] = lastChunk;
				lastChunk->m_Index = chunk->m_Index;
			}
			m_Chunks.pop_back();

			if (chunk == m_CurrentChunk)
			{
				m_CurrentChunk = nullptr;
			}

			delete chunk;
		}

		bool RemoveChunkFromFreeSpaces(ChunkType* chunk)
		{
			if (!chunk->m_IsInFreeSpaces) return false;

			if (m_ChunksWithFreeSpace.back() != chunk)
			{
				m_ChunksWithFreeSpace.back()->m_IndexInFreeSpaces = chunk->m_IndexInFreeSpaces;
				m_ChunksWithFreeSpace[chunk->m_IndexInFreeSpaces] = m_ChunksWithFreeSpace.back();
			}
			m_ChunksWithFreeSpace.pop_back();
			chunk->m_IsInFreeSpaces = false;

			return true;
		}

		bool AddChunkToFreeSpaces(ChunkType* chunk)
		{
			if (chunk->m_IsInFreeSpaces || chunk->IsFull()) return false;

			chunk->m_IsInFreeSpaces = true;
			chunk->m_IndexInFreeSpaces = static_cast<uint32_t>(m_ChunksWithFreeSpace.size());
			m_ChunksWithFreeSpace.push_back(chunk);

			return true;
		}
	};
}