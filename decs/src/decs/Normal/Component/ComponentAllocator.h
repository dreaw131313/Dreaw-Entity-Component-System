#pragma once

#include <memory>

#include "decs/Core/Memory.h"
#include "Component.h"

namespace decs
{
	template<typename T>
	struct TComponentChunkAllocation
	{
	public:
		T* m_Resource = nullptr;
		uint32_t m_Index = std::numeric_limits<uint32_t>::max();

	public:
		TComponentChunkAllocation() = default;

		TComponentChunkAllocation(T* resource, uint32_t index) :
			m_Resource(resource), m_Index(index)
		{

		}

		inline bool IsValid() const
		{
			return m_Resource != nullptr;
		}
	};

	template<typename T>
	class TComponentChunk
	{
	public:
		using AllocationResult = TComponentChunkAllocation<T>;

		template<typename T>
		friend class TComponentAllocator;
		template<typename T>
		friend struct TComponentAllocatorResourceRecord;

	private:
		ecsVector<uint32_t> m_FreeSpaces;

		T* m_Components = nullptr;
		uint32_t m_Capacity = 0;

		uint32_t m_CurrentAllocationOffset = 0;
		uint32_t m_Size = 0;

		uint32_t m_Index = std::numeric_limits<uint32_t>::max();
		uint32_t m_IndexInFreeSpaces = std::numeric_limits<uint32_t>::max();
		bool m_IsInFreeSpaces = false;

	private:
		TComponentChunk(uint32_t capacity) :
			m_Capacity(capacity > 0 ? capacity : 100)
		{
			m_Components = (T*) ::operator new(m_Capacity * sizeof(T), static_cast<std::align_val_t>(alignof(T)));
		}

		~TComponentChunk()
		{
			::operator delete(m_Components, m_Capacity * sizeof(T), static_cast<std::align_val_t>(alignof(T)));
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

		void ResetState()
		{
			m_Size = 0;
			m_FreeSpaces.clear();
			m_CurrentAllocationOffset = 0;
		}

		template<typename... Args>
		AllocationResult Create(Args&&... args)
		{
			if (IsFull())
			{
				return {};
			}

			m_Size += 1;

			uint32_t allocationIndex = 0;
			if (m_FreeSpaces.size() > 0)
			{
				allocationIndex = static_cast<uint32_t>(m_FreeSpaces.back());
				m_FreeSpaces.pop_back();
			}
			else
			{
				allocationIndex = m_CurrentAllocationOffset;
				m_CurrentAllocationOffset += 1;
			}

			T* componentPtr = new(&m_Components[allocationIndex])T(std::forward<Args>(args)...);

			return AllocationResult(componentPtr, allocationIndex);

		}

		bool RemoveAt(uint32_t index, const T* value)
		{
			if (index >= m_Capacity)
			{
				return false;
			}

			T* exitingComponentPtr = &m_Components[index];

			if (exitingComponentPtr == value)
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
				static_cast<EntityComponent*>(exitingComponentPtr)->m_InternalData.Reset();
				exitingComponentPtr->~T();

				return true;
			}

			return false;
		}

		/// <summary>
		/// This method is called only in TComponentAllocator destructor, because allocator know which components are allocated
		/// </summary>
		/// <param name="index"></param>
		/// <param name="value"></param>
		void CallDestructor_Unchecked(uint32_t index)
		{
			m_Components[index].~T();
		}
	};

	template<typename T>
	struct TComponentAllocatorResourceRecord
	{
	public:
		TComponentChunk<T>* m_Chunk = nullptr;
		uint32_t m_IndexInChunk = std::numeric_limits<uint32_t>::max();

		inline T* GetPtrFromChunk() const
		{
			return &m_Chunk->m_Components[m_IndexInChunk];
		}
	};

	template<typename T>
	class TComponentAllocator
	{
		static_assert(std::is_base_of_v<EntityComponent, T>, "T must derive from ::Try::ChunkAllocatorResource");

		using ChunkType = TComponentChunk<T>;

		using ResourceRecord = TComponentAllocatorResourceRecord<T>;
		using ChunkAllocation = ChunkType::AllocationResult;

	public:
		TComponentAllocator()
		{
			m_ResourceRecords.reserve(m_ChunkCapacity);
		}

		TComponentAllocator(uint32_t chunkCapacity) :
			m_ChunkCapacity(chunkCapacity == 0 ? 1 : chunkCapacity)
		{
			m_ResourceRecords.reserve(m_ChunkCapacity);
		}

		~TComponentAllocator()
		{
			for (ResourceRecord& record: m_ResourceRecords)
			{
				record.m_Chunk->CallDestructor_Unchecked(record.m_IndexInChunk);
			}

			for (auto& chunk : m_Chunks)
			{
				if (chunk != nullptr)
				{
					delete chunk;
				}
			}
		}

		TComponentAllocator(const TComponentAllocator&) = delete;
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

		TComponentAllocator(TComponentAllocator&& other) noexcept :
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

		TComponentAllocator& operator=(const TComponentAllocator&) = delete;

		TComponentAllocator& operator=(TComponentAllocator&& other) noexcept
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
				const EntityComponent* r = static_cast<const EntityComponent*>(value);
				const uint64_t indexInAllocator = r->GetIndexInAllocator();
				if (indexInAllocator < m_ResourceRecords.size())
				{
					const ResourceRecord& record = m_ResourceRecords[indexInAllocator];
					return record.GetPtrFromChunk() == value;
				}
			}

			return false;
		}

		template<typename... Args>
		T* Create(Args&&... args)
		{
			ChunkType* chunk = GetCurrentChunk();
			TComponentChunkAllocation<T> result = chunk->Create(std::forward<Args>(args)...);

			if (chunk->IsFull())
			{
				RemoveChunkFromFreeSpaces(chunk);
			}

			EntityComponent* baseComponentPtr = result.m_Resource;
			baseComponentPtr->SetIndexInAllocator(static_cast<uint32_t>(m_ResourceRecords.size()));

			m_ResourceRecords.push_back({ chunk, result.m_Index });

			return result.m_Resource;
		}

		bool Destroy(const T* value)
		{
			if (value == nullptr)
			{
				return false;
			}

			const EntityComponent* baseComponentPtr = static_cast<const EntityComponent*>(value);
			const uint32_t resourceIndexInAllocator = baseComponentPtr->GetIndexInAllocator();
			const uint32_t recordCount = static_cast<uint32_t>(m_ResourceRecords.size());

			if (resourceIndexInAllocator >= recordCount)
			{
				return false;
			}

			ResourceRecord resourceRecord = m_ResourceRecords[resourceIndexInAllocator];
			if (resourceRecord.GetPtrFromChunk() != value)
			{
				return false;
			}

			if (resourceIndexInAllocator < (recordCount - 1))
			{
				ResourceRecord& lastRecord = m_ResourceRecords.back();
				static_cast<EntityComponent*>(lastRecord.GetPtrFromChunk())->SetIndexInAllocator(resourceIndexInAllocator);
				m_ResourceRecords[resourceIndexInAllocator] = lastRecord;
			}
			m_ResourceRecords.pop_back();

			ChunkType* chunk = resourceRecord.m_Chunk;
			const uint32_t indexInChunk = resourceRecord.m_IndexInChunk;

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
			for (ResourceRecord& record : m_ResourceRecords)
			{
				record.m_Chunk->CallDestructor_Unchecked(record.m_IndexInChunk);
			}
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
				callable(*resourceRecord.GetPtrFromChunk());
			}
		}

	private:
		ecsVector<ChunkType*> m_Chunks;
		ecsVector<ChunkType*> m_ChunksWithFreeSpace;
		ecsVector<ResourceRecord> m_ResourceRecords{};
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

