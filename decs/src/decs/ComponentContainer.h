#pragma once
#include "Core.h"
#include "Containers/ChunkedVector.h"

namespace decs
{
	template<typename ComponentType>
	struct ComponentAllocationData
	{
	public:
		uint64_t ChunkIndex = std::numeric_limits<uint64_t>::max();
		uint64_t ElementIndex = std::numeric_limits<uint64_t>::max();;
		ComponentType* Component = nullptr;

	public:
		ComponentAllocationData() {}

		ComponentAllocationData(uint64_t chunkIndex, uint64_t index, ComponentType* component) :
			ChunkIndex(chunkIndex), ElementIndex(index), Component(component)
		{

		}

		inline bool IsValid()const
		{
			return Component != nullptr;
		}
	};

	using ComponentCopyData = ComponentAllocationData<void>;

	struct ComponentAllocatorSwapData
	{
	public:
		uint64_t ChunkIndex = std::numeric_limits<EntityID>::max();
		uint64_t ElementIndex = std::numeric_limits<EntityID>::max();
		EntityID eID = std::numeric_limits<EntityID>::max();

	public:
		ComponentAllocatorSwapData()
		{
		}

		ComponentAllocatorSwapData(
			uint64_t chunkIndex,
			uint64_t index,
			EntityID entityID
		) :
			ChunkIndex(chunkIndex), ElementIndex(index), eID(entityID)
		{
		}

		inline bool IsValid() const
		{
			return eID != std::numeric_limits<EntityID>::max();
		}
	};

	class BaseComponentAllocator
	{
	public:
		virtual ComponentAllocatorSwapData RemoveSwapBack(const uint64_t& chunkIndex, const uint64_t& elementIndex) = 0;

		virtual void* GetComponentAsVoid(const uint64_t& chunkIndex, const uint64_t& elementIndex) = 0;

		virtual ComponentCopyData CreateCopy(const EntityID& entityID, const uint64_t& chunkIndex, const uint64_t& elementIndex) = 0;

		virtual ComponentCopyData CreateCopy(BaseComponentAllocator* fromContainer, const EntityID& entityID, const uint64_t& chunkIndex, const uint64_t& elementIndex) = 0;

		virtual ComponentCopyData CreateCopy(const EntityID& entityID, void* voidCompPtr) = 0;

		virtual BaseComponentAllocator* CreateEmptyCopyOfYourself() = 0;

		virtual void Clear() = 0;


		virtual uint64_t ChunksCount() = 0;
		virtual uint64_t ChunkSize(const uint64_t& chunkIndex) = 0;
		virtual uint64_t ElementsCount() = 0;
		virtual std::pair<EntityID, void*> GetEntityAndComponentData(const uint64_t& index) = 0;
		virtual std::pair<EntityID, void*> GetEntityAndComponentData(const uint64_t& chunkIndex, const uint64_t& elementIndex) = 0;
	};

	template<typename DataType>
	class StableComponentAllocator : public BaseComponentAllocator
	{
		using AllocationData = ComponentAllocationData<DataType>;
	private:
#pragma region ChunkAllocationResult
		struct ChunkAllocationResult final
		{
		public:
			uint64_t Index = std::numeric_limits<uint64_t>::max();
			DataType* Data = nullptr;

		public:
			ChunkAllocationResult() {}

			ChunkAllocationResult(
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
#pragma endregion

#pragma region Chunk
		class Chunk final
		{
		public:
			DataType* m_Data = nullptr;
			uint64_t m_CreatedElements = 0;
			uint64_t m_CurrentAllocationOffset = 0;
			uint64_t m_Index = 0;
			uint64_t m_Capacity = 0;

			bool bIsInChunksWithFreeSpace = true;
			uint64_t m_IndexInWithFreeSpaces = 0;

			uint64_t m_FreeSpacesCount = 0;
			std::vector<uint64_t> m_FreeSpaces;

		public:
			Chunk()
			{

			}

			~Chunk()
			{

			}

			inline bool IsEmpty() const { return m_CreatedElements == 0; }
			inline bool IsFull() const
			{
				return m_Capacity == m_CreatedElements;
			}

			template<typename... Args>
			ChunkAllocationResult Add(Args&&... args)
			{
				if (IsFull()) return ChunkAllocationResult();

				m_CreatedElements += 1;

				if (m_FreeSpacesCount > 0)
				{
					m_FreeSpacesCount -= 1;
					uint64_t freeSpaceIndex = m_FreeSpaces.back();
					m_FreeSpaces.pop_back();
					DataType* data = new(&m_Data[freeSpaceIndex])DataType(std::forward<Args>(args)...);

					return ChunkAllocationResult(freeSpaceIndex, data);
				}

				uint64_t allocationIndex = m_CurrentAllocationOffset;
				m_CurrentAllocationOffset += 1;
				DataType* data = new(&m_Data[allocationIndex])DataType(std::forward<Args>(args)...);

				return ChunkAllocationResult(allocationIndex, data);
			}

			bool RemoveAt(const uint64_t& index)
			{
				if (index >= m_Capacity) return false;

				m_CreatedElements -= 1;
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

				m_Data[index].~DataType();
				return true;
			}

		};
#pragma endregion

#pragma region Entity node info
		struct NodeInfo
		{
		public:
			EntityID eID = std::numeric_limits<EntityID>::max();
			Chunk* ChunkPtr = nullptr;
			DataType* Data = nullptr;
			uint64_t Index = std::numeric_limits<uint64_t>::max();// index in chunk

		public:
			NodeInfo(
				const EntityID& entityID,
				Chunk* chunk,
				DataType* data,
				const uint64_t& index
			) :
				eID(entityID),
				ChunkPtr(chunk),
				Data(data),
				Index(index)
			{

			}

			~NodeInfo()
			{

			}
		};
#pragma endregion

	public:
		StableComponentAllocator()
		{

		}

		StableComponentAllocator(const uint64_t& chunkCapacity) :
			m_ChunkCapacity(chunkCapacity),
			m_Nodes(chunkCapacity)
		{
			//CreateChunk();
		}

		~StableComponentAllocator()
		{
			DestroyChunks();
		}

		ChunkedVector<NodeInfo>& Nodes() { return m_Nodes; }

		virtual void Clear() override
		{
			DestroyChunks();
			m_Nodes.Clear();
			m_Chunks.clear();
			m_ChunksWithFreeSpaces.clear();
		}

		inline uint64_t GetChunkCapacity() const { return m_ChunkCapacity; }

		template<typename... Args>
		AllocationData EmplaceBack(const EntityID& entityID, Args&&... args)
		{
			Chunk* chunk = nullptr;
			if (m_ChunksWithFreeSpaces.size() > 0)
			{
				chunk = m_ChunksWithFreeSpaces[0];
			}
			else
			{
				chunk = CreateChunk();
			}

			auto chunkAllocationData = chunk->Add(std::forward<Args>(args)...);
			auto nodeAllocationData = m_Nodes.EmplaceBack_CR(entityID, chunk, chunkAllocationData.Data, chunkAllocationData.Index);

			RemoveChunkFromChunksWithFreeSpaces(chunk);

			return AllocationData(nodeAllocationData.ChunkIndex, nodeAllocationData.ElementIndex, chunkAllocationData.Data);
		}

		virtual ComponentAllocatorSwapData RemoveSwapBack(const uint64_t& chunkIndex, const uint64_t& elementIndex) override
		{
			NodeInfo& node = m_Nodes(chunkIndex, elementIndex);
			node.ChunkPtr->RemoveAt(node.Index);

			if (!RemoveEmptyChunk(node.ChunkPtr))
			{
				AddChunkToChunksWithFreeSpaces(node.ChunkPtr);
			}

			m_Nodes.RemoveSwapBack(chunkIndex, elementIndex);
			if (m_Nodes.IsPositionValid(chunkIndex, elementIndex))
			{
				auto& swappedNodeInfo = m_Nodes(chunkIndex, elementIndex);
				return ComponentAllocatorSwapData(chunkIndex, elementIndex, swappedNodeInfo.eID);
			}

			return ComponentAllocatorSwapData();
		}

		virtual void* GetComponentAsVoid(const uint64_t& chunkIndex, const uint64_t& elementIndex) override
		{
			return m_Nodes(chunkIndex, elementIndex).Data;
		}

		virtual ComponentCopyData CreateCopy(const EntityID& entityID, const uint64_t& chunkIndex, const uint64_t& elementIndex) override
		{
			if (!m_Nodes.IsPositionValid(chunkIndex, elementIndex))
			{
				return ComponentCopyData();
			}

			NodeInfo& nodeToCopy = m_Nodes(chunkIndex, elementIndex);
			AllocationData allocationData = EmplaceBack(entityID, *nodeToCopy.Data);

			return ComponentCopyData(allocationData.ChunkIndex, allocationData.ElementIndex, allocationData.Component);
		}

		virtual ComponentCopyData CreateCopy(BaseComponentAllocator* fromContainer, const EntityID& entityID, const uint64_t& chunkIndex, const uint64_t& elementIndex) override
		{
			using ContainerType = StableComponentAllocator<DataType>;
			ContainerType* from = dynamic_cast<ContainerType*>(fromContainer);
			if (from == nullptr)
			{
				return ComponentCopyData();
			}
			if (!from->m_Nodes.IsPositionValid(chunkIndex, elementIndex))
			{
				return ComponentCopyData();
			}

			NodeInfo& nodeToCopy = from->m_Nodes(chunkIndex, elementIndex);
			AllocationData allocationData = EmplaceBack(entityID, *nodeToCopy.Data);

			return ComponentCopyData(allocationData.ChunkIndex, allocationData.ElementIndex, allocationData.Component);
		}

		virtual ComponentCopyData CreateCopy(const EntityID& entityID, void* voidCompPtr) override
		{
			DataType* component = reinterpret_cast<DataType*>(voidCompPtr);

			Chunk* chunk = nullptr;
			if (m_ChunksWithFreeSpaces.size() > 0)
			{
				chunk = m_ChunksWithFreeSpaces[0];
			}
			else
			{
				chunk = CreateChunk();
			}

			auto chunkAllocationData = chunk->Add(*component);
			auto nodeAllocationData = m_Nodes.EmplaceBack_CR(entityID, chunk, chunkAllocationData.Data, chunkAllocationData.Index);

			RemoveChunkFromChunksWithFreeSpaces(chunk);

			return ComponentCopyData(nodeAllocationData.ChunkIndex, nodeAllocationData.ElementIndex, chunkAllocationData.Data);
		}

		virtual BaseComponentAllocator* CreateEmptyCopyOfYourself() override
		{
			return new StableComponentAllocator<DataType>(m_ChunkCapacity);
		}


		virtual uint64_t ChunksCount() override
		{
			return m_Nodes.ChunksCount();
		}

		virtual uint64_t ChunkSize(const uint64_t& chunkIndex) override
		{
			return m_Nodes.GetChunkSize(chunkIndex);
		}

		virtual uint64_t ElementsCount() override
		{
			return m_Nodes.Size();
		}

		virtual std::pair<EntityID, void*> GetEntityAndComponentData(const uint64_t& index) override
		{
			NodeInfo& node = m_Nodes[index];
			return { node.eID, node.Data };
		}

		virtual std::pair<EntityID, void*> GetEntityAndComponentData(const uint64_t& chunkIndex, const uint64_t& elementIndex) override
		{
			NodeInfo& node = m_Nodes(chunkIndex, elementIndex);
			return { node.eID, node.Data };
		}


	private:
		uint64_t m_ChunkCapacity = 128;
		ChunkedVector<NodeInfo> m_Nodes;

		std::vector<Chunk*> m_Chunks;
		std::vector<Chunk*> m_ChunksWithFreeSpaces;

	private:
		Chunk* CreateChunk()
		{
			Chunk* chunk = new Chunk();
			chunk->m_Capacity = m_ChunkCapacity;
			chunk->m_Data = static_cast<DataType*> (::operator new(m_ChunkCapacity * sizeof(DataType)));
			chunk->m_Index = m_Chunks.size();
			chunk->m_IndexInWithFreeSpaces = m_ChunksWithFreeSpaces.size();

			m_Chunks.push_back(chunk);
			m_ChunksWithFreeSpaces.push_back(chunk);
			return chunk;
		}

		void DestroyChunk(Chunk* chunk)
		{
			::operator delete(chunk->m_Data, chunk->m_Capacity * sizeof(DataType));
			delete chunk;
		}

		bool RemoveEmptyChunk(Chunk* chunk)
		{
			if (m_Chunks.size() > 1)
			{
				if (!chunk->IsEmpty()) return false;

				{
					Chunk* lastChunk = m_Chunks.back();
					if (chunk != lastChunk)
					{
						lastChunk->m_Index = chunk->m_Index;
						m_Chunks[lastChunk->m_Index] = lastChunk;
					}
					m_Chunks.pop_back();
				}

				{
					if (chunk->bIsInChunksWithFreeSpace)
					{
						Chunk* lastChunk = m_ChunksWithFreeSpaces.back();
						if (chunk != lastChunk)
						{
							lastChunk->m_IndexInWithFreeSpaces = chunk->m_IndexInWithFreeSpaces;
							m_ChunksWithFreeSpaces[lastChunk->m_Index] = lastChunk;
						}
						m_ChunksWithFreeSpaces.pop_back();
					}
				}

				DestroyChunk(chunk);
				return true;
			}

			return false;
		}

		void DestroyChunks()
		{
			uint64_t m_NodesChunkCount = m_Nodes.ChunksCount();
			for (uint64_t chunkIdx = 0; chunkIdx < m_NodesChunkCount; chunkIdx++)
			{
				NodeInfo* chunk = m_Nodes.GetChunk(chunkIdx);
				uint64_t elementsInChunk = m_Nodes.GetChunkSize(chunkIdx);
				for (uint64_t elementIdx = 0; elementIdx < elementsInChunk; elementIdx++)
				{
					chunk[elementIdx].Data->~DataType();
				}
			}

			uint64_t chunksCount = m_Chunks.size();
			for (uint64_t chunkIdx = 0; chunkIdx < chunksCount; chunkIdx++)
			{
				DestroyChunk(m_Chunks[chunkIdx]);
			}
		}

		void RemoveChunkFromChunksWithFreeSpaces(Chunk* chunk)
		{
			if (chunk->bIsInChunksWithFreeSpace && chunk->IsFull())
			{
				Chunk* lastChunk = m_ChunksWithFreeSpaces.back();
				chunk->bIsInChunksWithFreeSpace = false;
				if (chunk != lastChunk)
				{
					lastChunk->m_IndexInWithFreeSpaces = chunk->m_IndexInWithFreeSpaces;
					m_ChunksWithFreeSpaces[lastChunk->m_IndexInWithFreeSpaces] = lastChunk;
				}
				m_ChunksWithFreeSpaces.pop_back();
			}
		}

		void AddChunkToChunksWithFreeSpaces(Chunk* chunk)
		{
			if (chunk->bIsInChunksWithFreeSpace || chunk->IsFull()) return;

			chunk->bIsInChunksWithFreeSpace = true;
			chunk->m_IndexInWithFreeSpaces = m_ChunksWithFreeSpaces.size();
			m_ChunksWithFreeSpaces.push_back(chunk);
		}

	};
}