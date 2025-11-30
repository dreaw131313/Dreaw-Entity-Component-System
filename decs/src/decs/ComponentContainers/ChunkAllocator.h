#pragma once
#include <cstdint>
#include <vector>
#include <limits>

#include "decs/Memory.h"

namespace decs
{

	namespace ChunkAllocatorImpl
	{
		class Resource
		{
			template<typename T>
			friend class TAllocator;

		protected:
			~Resource() = default;

		public:
			Resource() = default;

			Resource(const Resource& other)
			{

			}

			Resource(Resource&& move) noexcept
			{

			}

			Resource& operator=(const Resource& other)
			{
				return *this;
			}

			Resource& operator==(Resource&& other) noexcept
			{
				return *this;
			}

		private:
			uint64_t m_IndexInAllocator = std::numeric_limits<uint64_t>::max();
		};

		template<typename T>
		struct TChunkAllocation
		{
		public:
			T* m_Resource = nullptr;
			uint32_t m_Index = std::numeric_limits<uint32_t>::max();

		public:
			TChunkAllocation() = default;

			TChunkAllocation(T* resource, uint32_t index):
				m_Resource(resource), m_Index(index)
			{

			}

			inline bool IsValid() const
			{
				return m_Resource != nullptr;
			}
		};

		template<typename T>
		class TChunk
		{
		public:
			using AllocationResult = TChunkAllocation<T>;

			template<typename T>
			friend class TAllocator;


		private:
			std::vector<uint32_t> m_FreeSpaces;

			uint8_t* m_MemoryBlock = nullptr;
			uint64_t m_MemoryBlockSize = 0;
			T* m_Data = nullptr;
			bool* m_AllocationFlags = nullptr;

			uint32_t m_Capacity = 0;

			uint32_t m_CurrentAllocationOffset = 0;
			uint32_t m_Size = 0;

			uint32_t m_Index = std::numeric_limits<uint32_t>::max();
			uint32_t m_IndexInFreeSpaces = std::numeric_limits<uint32_t>::max();
			bool m_IsInFreeSpaces = false;

		private:
			TChunk(uint32_t capacity):
				m_Capacity(capacity > 0 ? capacity : 100)
			{
				const uint64_t dataSize = m_Capacity * sizeof(T);
				const uint64_t flagsOffset = Memory::Align(dataSize, alignof(bool));
				const uint64_t flagsSize = m_Capacity * sizeof(bool);
				m_MemoryBlockSize = flagsOffset + flagsSize;

				m_MemoryBlock = (uint8_t*)operator new(m_MemoryBlockSize, static_cast<std::align_val_t>(alignof(T)));

				m_Data = reinterpret_cast<T*>(m_MemoryBlock);
				m_AllocationFlags = reinterpret_cast<bool*>(m_MemoryBlock + dataSize);
			}

			~TChunk()
			{
				for (uint32_t i = 0; i < m_CurrentAllocationOffset; i++)
				{
					if (m_AllocationFlags[i])
					{
						m_Data[i].~T();
					}
				}

				operator delete(m_MemoryBlock, m_MemoryBlockSize, static_cast<std::align_val_t>(alignof(T)));
			}

			uint32_t GetChunkIndex() const
			{
				return m_Index;
			}

			bool IsEmpty() const
			{
				return m_Size == 0;
			}

			bool IsFull() const
			{
				return m_Capacity == m_Size;
			}

			T& operator[](uint32_t index) const
			{
				return m_Data[index].Value;
			}

			template<typename... Args>
			AllocationResult Create(Args&&... args)
			{
				if (IsFull()) return {};

				m_Size += 1;

				if (m_FreeSpaces.size() > 0)
				{
					uint32_t freeSpaceIndex = m_FreeSpaces.back();
					m_FreeSpaces.pop_back();

					T& data = m_Data[freeSpaceIndex];
					m_AllocationFlags[freeSpaceIndex] = true;

					T* dataPtr = new(&data)T(std::forward<Args>(args)...);

					return AllocationResult(dataPtr, freeSpaceIndex);
				}

				{
					uint32_t allocationIndex = m_CurrentAllocationOffset;
					T& data = m_Data[m_CurrentAllocationOffset];
					m_AllocationFlags[m_CurrentAllocationOffset] = true;

					T* dataPtr = new(&data)T(std::forward<Args>(args)...);

					m_CurrentAllocationOffset += 1;

					return AllocationResult(dataPtr, allocationIndex);
				}
			}

			bool RemoveAt(uint32_t index, const T* value)
			{
				if (index >= m_Capacity)
				{
					return false;
				}

				T& data = m_Data[index];
				bool& allocationFlag = m_AllocationFlags[index];
				if (allocationFlag && (&data) == value)
				{
					m_Size -= 1;

					if (IsEmpty())
					{
						m_FreeSpaces.clear();
						m_CurrentAllocationOffset = 0;
					}
					else
					{
						const uint32_t allocationOffsetMinusOne = m_CurrentAllocationOffset - 1;

						if (index == allocationOffsetMinusOne)
						{
							m_CurrentAllocationOffset = allocationOffsetMinusOne;
						}
						else
						{
							m_FreeSpaces.push_back(index);
						}
					}

					allocationFlag = false;
					data.~T();
					return true;
				}

				return false;
			}
		};

		template<typename T>
		struct TAllocatorResourceRecord
		{
		public:
			T* m_Resource = nullptr;
			TChunk<T>* m_Chunk = nullptr;
			uint32_t m_IndexInChunk = std::numeric_limits<uint32_t>::max();
		};

		template<typename T>
		class TAllocator
		{
			static_assert(std::is_base_of_v<Resource, T>, "T must derive from ::Try::ChunkAllocatorResource");

			using ChunkType = TChunk<T>;

			using ResourceRecord = TAllocatorResourceRecord<T>;
			using ChunkAllocation = ChunkType::AllocationResult;

		public:
			TAllocator()
			{
				m_ResourceRecords.reserve(m_ChunkCapacity);
			}

			TAllocator(uint32_t chunkCapacity):
				m_ChunkCapacity(chunkCapacity == 0 ? 1 : chunkCapacity)
			{
				m_ResourceRecords.reserve(m_ChunkCapacity);
			}

			~TAllocator()
			{
				for (auto& chunk : m_Chunks)
				{
					if (chunk != nullptr)
					{
						delete chunk;
					}
				}
			}

			TAllocator(const TAllocator& other) = delete;
			/*TAllocator(const TAllocator& other):
			m_ChunkCapacity(other.m_ChunkCapacity)
			{
			for (auto otherChunk : other.m_Chunks)
			{
			m_Chunks.push_back(otherChunk->CreateCopy());
			}

			for (auto& otherChunk : other.m_ChunksWithFreeSpace)
			{
			m_ChunksWithFreeSpace.push_back(m_Chunks[otherChunk->m_Index]);
			}

			m_CurrentChunk = m_Chunks[other.m_CurrentChunk->m_Index];
			}*/

			TAllocator(TAllocator&& other) noexcept:
				m_ChunkCapacity(other.m_ChunkCapacity)
			{
				m_Chunks = std::move(other.m_Chunks);
				m_ChunksWithFreeSpace = std::move(other.m_ChunksWithFreeSpace);
				m_ResourceRecords = std::move(other.m_ResourceRecords);
				m_CurrentChunk = other.m_CurrentChunk;

				other.m_Chunks.clear();
				other.m_ChunksWithFreeSpace.clear();
				m_ResourceRecords.clear();
				other.m_CurrentChunk = nullptr;
			}

			TAllocator& operator=(const TAllocator& other) = delete;

			TAllocator& operator=(TAllocator&& other) noexcept
			{
				Clear();

				m_ChunkCapacity = other.m_ChunkCapacity;

				m_Chunks = std::move(other.m_Chunks);
				m_ChunksWithFreeSpace = std::move(other.m_ChunksWithFreeSpace);
				m_ResourceRecords = std::move(other.m_ResourceRecords);
				m_CurrentChunk = other.m_CurrentChunk;

				other.m_Chunks.clear();
				other.m_ChunksWithFreeSpace.clear();
				m_ResourceRecords.clear();
				other.m_CurrentChunk = nullptr;

				return *this;
			}

			uint32_t GetChunkSize() const noexcept
			{
				return m_ChunkCapacity;
			}

			bool Contain(const T* value) const
			{
				if (value != nullptr)
				{
					const Resource* r = static_cast<const Resource*>(value);
					if (r->m_IndexInAllocator < m_ResourceRecords.size())
					{
						const ResourceRecord& resourceRecord = m_ResourceRecords[r->m_IndexInAllocator];
						return resourceRecord.m_Resource == value;
					}
				}

				return false;
			}

			template<typename... Args>
			T* Create(Args&&... args)
			{
				ChunkType* chunk = GetCurrentChunk();
				TChunkAllocation<T> result = chunk->Create(std::forward<Args>(args)...);

				if (chunk->IsFull())
				{
					RemoveChunkFromFreeSpaces(chunk);
				}

				result.m_Resource->m_IndexInAllocator = m_ResourceRecords.size();
				m_ResourceRecords.push_back({ result.m_Resource, chunk, result.m_Index });

				return result.m_Resource;
			}

			bool Destroy(const T* value)
			{
				if (value == nullptr)
				{
					return false;
				}

				const Resource* resource = static_cast<const Resource*>(value);
				const uint64_t resourceIndexInAllocator = resource->m_IndexInAllocator;
				const uint64_t recordCount = m_ResourceRecords.size();

				if (resourceIndexInAllocator >= recordCount)
				{
					return false;
				}

				ResourceRecord resourceRecord = m_ResourceRecords[resourceIndexInAllocator];
				if (resourceRecord.m_Resource != resource)
				{
					return false;
				}

				if (resourceIndexInAllocator < (recordCount - 1))
				{
					ResourceRecord& lastRecord = m_ResourceRecords.back();
					lastRecord.m_Resource->m_IndexInAllocator = resourceIndexInAllocator;
					m_ResourceRecords[resourceIndexInAllocator] = lastRecord;
				}
				m_ResourceRecords.pop_back();

				ChunkType* chunk = resourceRecord.m_Chunk;
				const uint32_t indexInChunk = resourceRecord.m_IndexInChunk;

				bool wasChunkFull = chunk->IsFull();
				if (chunk->RemoveAt(indexInChunk, value))
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

				return false;
			}

			void Clear()
			{
				for (auto& chunk : m_Chunks)
				{
					delete chunk;
				}
				m_CurrentChunk = nullptr;
				m_Chunks.clear();
				m_ChunksWithFreeSpace.clear();
				m_ResourceRecords.clear();
			}

			template<typename TCallable>
			void IterateOverResources(TCallable&& callable)
			{
				for (ResourceRecord& resourceRecord : m_ResourceRecords)
				{
					callable(*resourceRecord.m_Resource);
				}
			}

		private:
			std::vector<ChunkType*> m_Chunks;
			std::vector<ChunkType*> m_ChunksWithFreeSpace;
			std::vector<ResourceRecord> m_ResourceRecords{};
			ChunkType* m_CurrentChunk = nullptr;
			uint32_t m_ChunkCapacity = 100;

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
						m_CurrentChunk = CreateNewChunk();
					}
				}

				return m_CurrentChunk;
			}

			ChunkType* CreateNewChunk()
			{
				ChunkType* newChunk = new ChunkType(m_ChunkCapacity);
				newChunk->m_IsInFreeSpaces = true;
				newChunk->m_IndexInFreeSpaces = static_cast<uint32_t>(m_ChunksWithFreeSpace.size());
				newChunk->m_Index = static_cast<uint32_t>(m_Chunks.size());

				m_Chunks.push_back(newChunk);
				m_ChunksWithFreeSpace.push_back(newChunk);
				return newChunk;
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

	using ChunkAllocatorResource = ChunkAllocatorImpl::Resource;

	template<typename T>
	concept ChunkAllocatorResourceConcept = std::is_base_of<ChunkAllocatorResource, T>::value;

	template<ChunkAllocatorResourceConcept T>
	using TChunkAllocator = ChunkAllocatorImpl::TAllocator<T>;
}