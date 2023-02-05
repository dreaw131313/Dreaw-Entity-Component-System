#pragma once
#include "decs\Core.h"
#include "decs\Containers/ChunkedVector.h"
#include "Type.h"

namespace decs
{
	template<typename DataType>
	struct AllocationResult final
	{
	public:
		uint64_t Index = std::numeric_limits<uint64_t>::max();
		DataType* Data = nullptr;

	public:
		AllocationResult() {}

		AllocationResult(
			const uint64_t& index,
			DataType* data
		) :
			Index(index),
			Data(data)
		{

		}

		inline bool IsValid() const
		{
			return Data != nullptr;
		}
	};

	template<typename DataType>
	class Chunk final
	{
		using ChunkAllocationResult = AllocationResult<DataType>;
	public:
		uint64_t m_Index = 0;
		uint64_t m_IndexInFreeSpaces = 0;
		bool m_IsInFreeSpaces = 0;
	public:
		Chunk(const uint64_t& capacity) :
			m_Capacity(capacity),
			m_AllocationFlags(new bool[capacity]()),
			m_Data(static_cast<DataType*> (::operator new(capacity * sizeof(DataType))))
		{
		}

		~Chunk()
		{
			if (m_Size > 0)
			{
				for (uint32_t i = 0; i < m_Capacity; i++)
				{
					if (m_AllocationFlags[i])
					{
						m_Data[i].~DataType();
					}
				}
			}

			::operator delete(m_Data, m_Capacity * sizeof(DataType));
			delete[] m_AllocationFlags;
		}

		inline bool IsEmpty() const
		{
			return m_Size == 0;
		}

		inline bool IsFull() const { return m_Capacity == m_Size; }

		DataType& operator[](const uint64_t& index) const { return m_Data[index]; }

		inline bool IsAllocatedAt(const uint64_t& index) const
		{
			return m_AllocationFlags[index];
		}
		template<typename... Args>
		ChunkAllocationResult Emplace(Args&&... args)
		{
			if (IsFull()) return ChunkAllocationResult();

			m_Size += 1;

			if (m_FreeSpacesCount > 0)
			{
				m_FreeSpacesCount -= 1;
				uint64_t freeSpaceIndex = m_FreeSpaces.back();
				m_FreeSpaces.pop_back();
				DataType* data = new(&m_Data[freeSpaceIndex])DataType(std::forward<Args>(args)...);

				m_AllocationFlags[freeSpaceIndex] = true;
				return ChunkAllocationResult(freeSpaceIndex, data);
			}

			uint64_t allocationIndex = m_CurrentAllocationOffset;
			m_CurrentAllocationOffset += 1;
			DataType* data = new(&m_Data[allocationIndex])DataType(std::forward<Args>(args)...);

			m_AllocationFlags[allocationIndex] = true;

			return ChunkAllocationResult(allocationIndex, data);
		}

		bool RemoveAt(const uint64_t& index)
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
					m_FreeSpacesCount += 1;
					m_FreeSpaces.push_back(index);
				}

				if (IsEmpty())
				{
					m_FreeSpaces.clear();
					m_CurrentAllocationOffset = 0;
					m_FreeSpacesCount = 0;
				}

				m_AllocationFlags[index] = false;
				m_Data[index].~DataType();
				return true;
			}
			return false;
		}

	private:
		uint64_t m_Capacity = 0;
		DataType* m_Data = nullptr;
		bool* m_AllocationFlags = nullptr;

		uint64_t m_CurrentAllocationOffset = 0;
		uint64_t m_Size = 0;

		uint64_t m_FreeSpacesCount = 0;
		std::vector<uint64_t> m_FreeSpaces;
	};


	class StableComponentRef
	{
	public:
		uint64_t m_ChunkIndex = std::numeric_limits<uint64_t>::max();
		uint64_t m_Index = std::numeric_limits<uint64_t>::max();
		void* m_ComponentPtr = nullptr;

	public:
		StableComponentRef()
		{

		}

		StableComponentRef(
			void* componentPtr,
			const uint64_t& chunkIndex,
			const uint64_t& index
		) :
			m_ComponentPtr(componentPtr), m_ChunkIndex(chunkIndex), m_Index(index)
		{

		}

	};

	class StableContainerBase
	{
	public:
		inline virtual TypeID GetTypeID()const noexcept = 0;
		virtual StableContainerBase* CreateOwnEmptyCopy(const uint64_t& withChunkSize) = 0;

		virtual bool Remove(const uint64_t& chunkIndex, const uint64_t& elementIndex) = 0;
		virtual StableComponentRef EmplaceFromVoid(void* ptr) = 0;
		virtual uint64_t GetChunkSize() const noexcept = 0;
		virtual void Clear() = 0;
	};

	template<typename DataType>
	class StableContainer : public StableContainerBase
	{
		using ChunkType = Chunk<DataType>;

	public:
		StableContainer()
		{

		}

		StableContainer(const uint64_t& chunkCapacity) :
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

		inline virtual TypeID GetTypeID()const noexcept override { return Type<Stable<DataType>>::ID(); }

		virtual StableContainerBase* CreateOwnEmptyCopy(const uint64_t& withChunkSize) override
		{
			return new StableContainer<DataType>(withChunkSize);
		}

		virtual uint64_t GetChunkSize() const noexcept override
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

			return StableComponentRef(result.Data, chunk->m_Index, result.Index);
		}

		bool Remove(const uint64_t& chunkIndex, const uint64_t& elementIndex) override
		{
			if (chunkIndex < m_Chunks.size() && m_Chunks[chunkIndex] != nullptr)
			{
				ChunkType* chunk = m_Chunks[chunkIndex];
				bool wasChunkFull = chunk->IsFull();
				if (chunk->RemoveAt(elementIndex))
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

		virtual StableComponentRef EmplaceFromVoid(void* ptr)override
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
		uint64_t m_ChunkCapacity = 1000;
		ChunkType* m_CurrentChunk = nullptr;
		std::vector<ChunkType*> m_Chunks;
		std::vector<ChunkType*> m_ChunksWithFreeSpace;

	private:
		ChunkType* GetCurrentChunk()
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
					m_CurrentChunk->m_IndexInFreeSpaces = m_ChunksWithFreeSpace.size();
					m_ChunksWithFreeSpace.push_back(m_CurrentChunk);

					bool isChunkPlacedInChunks = false;
					for (uint32_t i = 0; i < m_Chunks.size(); i++)
					{
						if (m_Chunks[i] == nullptr)
						{
							m_Chunks[i] = m_CurrentChunk;
							m_CurrentChunk->m_Index = i;
							isChunkPlacedInChunks = true;
							break;
						}
					}

					if (!isChunkPlacedInChunks)
					{
						m_CurrentChunk->m_Index = m_Chunks.size();
						m_Chunks.push_back(m_CurrentChunk);
					}
				}
			}

			return m_CurrentChunk;
		}

		void RemoveChunk(ChunkType* chunk)
		{
			RemoveChunkFromFreeSpaces(chunk);

			if (chunk == m_Chunks.back())
			{
				m_Chunks.pop_back();

				for (int i = (int)m_Chunks.size() - 1; i > -1; i--)
				{
					if (m_Chunks[i] == nullptr)
					{
						m_Chunks.pop_back();
					}
					else
					{
						break;
					}
				}
			}
			else
			{
				m_Chunks[chunk->m_Index] = nullptr;
			}

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
			chunk->m_IndexInFreeSpaces = m_ChunksWithFreeSpace.size();
			m_ChunksWithFreeSpace.push_back(chunk);

			return true;
		}
	};

	class StableContainersManager
	{
	public:
		StableContainersManager()
		{

		}

		StableContainersManager(const uint64_t& defaultChunkSize) :
			m_DefaultChunkSize(defaultChunkSize)
		{

		}

		template<typename T>
		bool SetStableComponentChunkSize(const uint64_t& chunkSize)
		{
			constexpr TypeID typeID = Type<Stable<T>>::ID();
			auto& container = m_Containers[typeID];

			if (container == nullptr)
			{
				container = new StableContainer<T>(chunkSize);
				return true;
			}
			return false;
		}

		template<typename T>
		uint64_t GetStableComponentChunkSize()
		{
			constexpr TypeID typeID = Type<Stable<T>>::ID();
			auto it = m_Containers.find(typeID);

			if (it != m_Containers.end())
			{
				return it->second->GetChunkSize();
			}

			return std::numeric_limits<uint64_t>::max();
		}

		inline StableContainerBase* GetStableContainer(const TypeID& typeID)
		{
			auto it = m_Containers.find(typeID);
			return it != m_Containers.end() ? it->second : nullptr;
		}

		inline StableContainerBase* CreateStableContainerFromOther(StableContainerBase* other)
		{
			const TypeID id = other->GetTypeID();
			auto& container = m_Containers[id];
			if (container == nullptr)
			{
				container = other->CreateOwnEmptyCopy(m_DefaultChunkSize);
			}

			return container;
		}

		template<typename T>
		StableContainer<T>* GetOrCreateStableContainer()
		{
			constexpr TypeID typeID = Type<Stable<T>>::ID();
			auto& container = m_Containers[typeID];

			if (container == nullptr)
			{
				container = new StableContainer<T>(m_DefaultChunkSize);
			}

			return static_cast<StableContainer<T>*>(container);
		}

		inline void DestroyContainers()
		{
			for (auto& [key, value] : m_Containers)
			{
				delete value;
			}
		}

		inline void ClearContainers()
		{
			for (auto& [key, value] : m_Containers)
			{
				value->Clear();
			}
		}

	private:
		ecsMap<TypeID, StableContainerBase*> m_Containers;
		uint64_t m_DefaultChunkSize = 100;

	};
}