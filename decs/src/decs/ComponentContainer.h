#pragma once
#include "decspch.h"
#include "Core.h"

#include "Containers/BucketAllocator.h"
#include "Containers/ChunkedVector.h"

namespace decs
{

	template<typename ComponentType>
	struct ComponentAllocationData
	{
	public:
		uint64_t BucketIndex = std::numeric_limits<uint64_t>::max();
		uint64_t ElementIndex = std::numeric_limits<uint64_t>::max();;
		ComponentType* Component = nullptr;

	public:
		ComponentAllocationData() {}

		ComponentAllocationData(uint64_t bucketIndex, uint64_t index, ComponentType* component) :
			BucketIndex(bucketIndex), ElementIndex(index), Component(component)
		{

		}

		inline bool IsValid()const 
		{
			return Component != nullptr;
		}
	};

	struct ComponentCopyData
	{
	public:
		uint64_t BucketIndex = std::numeric_limits<uint64_t>::max();
		uint64_t ElementIndex = std::numeric_limits<uint64_t>::max();;
		void* Component = nullptr;

	public:
		ComponentCopyData() {}

		ComponentCopyData(uint64_t bucketIndex, uint64_t index, void* component) :
			BucketIndex(bucketIndex), ElementIndex(index), Component(component)
		{

		}
	};

	template<typename ComponentType>
	struct BucketRemoveSwapBackResult
	{
	public:
		uint64_t Index = std::numeric_limits<uint64_t>::max();
		EntityID eID = std::numeric_limits<EntityID>::max();
		ComponentType* Component = nullptr;

	public:
		BucketRemoveSwapBackResult()
		{

		}

		BucketRemoveSwapBackResult(
			uint64_t index,
			EntityID entityID,
			ComponentType* component
		) :
			Index(index),
			eID(entityID),
			Component(component)
		{

		}

		inline bool IsValid() const
		{
			return eID != std::numeric_limits<EntityID>::max();
		}
	};


	template<typename ComponentType>
	struct ComponentNodeInfo
	{
	public:
		BucketNode<ComponentType>* Node = nullptr;
		EntityID eID = std::numeric_limits<EntityID>::max();

	public:
		ComponentNodeInfo(
			EntityID entityID,
			BucketNode<ComponentType>* node
		) :
			Node(node),
			eID(entityID)
		{

		}

		ComponentNodeInfo()
		{

		}
	};

	struct ComponentAllocatorSwapData
	{
	public:
		uint64_t BucketIndex = std::numeric_limits<EntityID>::max();
		uint64_t ElementIndex = std::numeric_limits<EntityID>::max();
		EntityID eID = std::numeric_limits<EntityID>::max();

	public:
		ComponentAllocatorSwapData()
		{
		}

		ComponentAllocatorSwapData(
			uint64_t bucketIndex,
			uint64_t index,
			EntityID entityID
		) :
			BucketIndex(bucketIndex), ElementIndex(index), eID(entityID)
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
		inline bool IsEmpty()const { return m_CreatedElements == 0; }

		inline uint64_t Size() const { return m_CreatedElements; }

		virtual ComponentAllocatorSwapData RemoveSwapBack(const uint64_t& bucketIndex, const uint64_t& elementIndex) = 0;

		virtual void* GetComponentAsVoid(const uint64_t& bucketIndex, const uint64_t& elementIndex) = 0;

		virtual ComponentCopyData CreateCopy(const EntityID& entityID, const uint64_t& bucketIndex, const uint64_t& elementIndex) = 0;

		virtual ComponentCopyData CreateCopy(BaseComponentAllocator* fromContainer, const EntityID& entityID, const uint64_t& bucketIndex, const uint64_t& elementIndex) = 0;

	protected:
		uint64_t m_CreatedElements = 0;
	};

	template<typename ComponentType>
	class ComponentAllocator : public BaseComponentAllocator
	{
	public:
		ComponentAllocator()
		{

		}

		~ComponentAllocator()
		{

		}

		virtual inline ComponentType* GetComponent(const uint64_t& bucketIndex, const uint64_t& elementIndex) = 0;
	};

	template<typename ComponentType>
	class StableComponentAllocator_Old : public ComponentAllocator<ComponentType>
	{
		using AllocationData = ComponentAllocationData<ComponentType>;
		using NodeInfo = ComponentNodeInfo<ComponentType>;
	public:
		StableComponentAllocator_Old()
		{

		}

		StableComponentAllocator_Old(const uint64_t& bucketCapacity) :
			m_Components(bucketCapacity),
			m_Nodes(bucketCapacity)
		{

		}

		~StableComponentAllocator_Old()
		{

		}

		template<typename... Args>
		AllocationData EmplaceBack(const EntityID& entityID, Args&&... args)
		{
			auto node = m_Components.AddNode(std::forward<Args>(args)...);
			auto result = m_Nodes.EmplaceBack_CR(entityID, node);

			return AllocationData(result.BucketIndex, result.ElementIndex, &node->Data());
		}

		/// <summary>
		/// 
		/// </summary>
		/// <param name="allocationData"></param>
		/// <returns></returns>
		virtual ComponentAllocatorSwapData RemoveSwapBack(const uint64_t& bucketIndex, const uint64_t& elementIndex) override
		{
			auto& nodeInfo = m_Nodes(bucketIndex, elementIndex);
			m_Components.RemoveNode(nodeInfo.Node);

			m_Nodes.RemoveSwapBack(bucketIndex, elementIndex);

			if (m_Nodes.IsPositionValid(bucketIndex, elementIndex))
			{
				auto& swappedNodeInfo = m_Nodes(bucketIndex, elementIndex);
				return ComponentAllocatorSwapData(bucketIndex, elementIndex, swappedNodeInfo.eID);
			}

			return ComponentAllocatorSwapData();
		}

		virtual void* GetComponentAsVoid(const uint64_t& bucketIndex, const uint64_t& elementIndex) override
		{
			return &m_Nodes(bucketIndex, elementIndex).Node->Data();
		}

		virtual inline ComponentType* GetComponent(const uint64_t& bucketIndex, const uint64_t& elementIndex) override
		{
			return &m_Nodes(bucketIndex, elementIndex).Node->Data();
		}

		virtual ComponentCopyData CreateCopy(const EntityID& entityID, const uint64_t& bucketIndex, const uint64_t& elementIndex) override
		{
			if (!m_Nodes.IsPositionValid(bucketIndex, elementIndex))
			{
				return ComponentCopyData();
			}

			NodeInfo& nodeToCopy = m_Nodes(bucketIndex, elementIndex);

			auto newNode = m_Components.AddNode(nodeToCopy.Node->Data());
			auto result = m_Nodes.EmplaceBack_CR(entityID, newNode);

			return ComponentCopyData(result.BucketIndex, result.ElementIndex, &newNode->Data());
		}

		virtual ComponentCopyData CreateCopy(BaseComponentAllocator* fromContainer, const EntityID& entityID, const uint64_t& bucketIndex, const uint64_t& elementIndex) override
		{
			using ContainerType = StableComponentAllocator_Old<ComponentType>;
			ContainerType* from = dynamic_cast<ContainerType*>(fromContainer);
			if (from == nullptr)
			{
				return ComponentCopyData();
			}
			if (!from->m_Nodes.IsPositionValid(bucketIndex, elementIndex))
			{
				return ComponentCopyData();
			}

			NodeInfo& nodeToCopy = from->m_Nodes(bucketIndex, elementIndex);

			auto newNode = m_Components.AddNode(nodeToCopy.Node->Data());
			auto result = m_Nodes.EmplaceBack_CR(entityID, newNode);

			return ComponentCopyData(result.BucketIndex, result.ElementIndex, &newNode->Data());
		}
	private:
		BucketAllocator<ComponentType> m_Components;
		ChunkedVector<NodeInfo> m_Nodes;
	};


	template<typename DataType>
	class StableComponentAllocator : public ComponentAllocator<DataType>
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
			CreateChunk();
		}

		~StableComponentAllocator()
		{
			DestroyChunks();
		}

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

			return AllocationData(nodeAllocationData.BucketIndex, nodeAllocationData.ElementIndex, chunkAllocationData.Data);
		}

		virtual ComponentAllocatorSwapData RemoveSwapBack(const uint64_t& bucketIndex, const uint64_t& elementIndex) override
		{
			NodeInfo& node = m_Nodes(bucketIndex, elementIndex);
			node.ChunkPtr->RemoveAt(node.Index);

			if (!RemoveEmptyChunk(node.ChunkPtr))
			{
				AddChunkToChunksWithFreeSpaces(node.ChunkPtr);
			}

			m_Nodes.RemoveSwapBack(bucketIndex, elementIndex);
			if (m_Nodes.IsPositionValid(bucketIndex, elementIndex))
			{
				auto& swappedNodeInfo = m_Nodes(bucketIndex, elementIndex);
				return ComponentAllocatorSwapData(bucketIndex, elementIndex, swappedNodeInfo.eID);
			}

			return ComponentAllocatorSwapData();
		}

		virtual void* GetComponentAsVoid(const uint64_t& bucketIndex, const uint64_t& elementIndex) override
		{
			return m_Nodes(bucketIndex, elementIndex).Data;
		}

		virtual ComponentCopyData CreateCopy(const EntityID& entityID, const uint64_t& bucketIndex, const uint64_t& elementIndex) override
		{
			if (!m_Nodes.IsPositionValid(bucketIndex, elementIndex))
			{
				return ComponentCopyData();
			}

			NodeInfo& nodeToCopy = m_Nodes(bucketIndex, elementIndex);
			AllocationData allocationData = EmplaceBack(entityID, *nodeToCopy.Data);

			return ComponentCopyData(allocationData.BucketIndex, allocationData.ElementIndex, allocationData.Component);
		}

		virtual ComponentCopyData CreateCopy(BaseComponentAllocator* fromContainer, const EntityID& entityID, const uint64_t& bucketIndex, const uint64_t& elementIndex) override
		{
			using ContainerType = StableComponentAllocator<DataType>;
			ContainerType* from = dynamic_cast<ContainerType*>(fromContainer);
			if (from == nullptr)
			{
				return ComponentCopyData();
			}
			if (!from->m_Nodes.IsPositionValid(bucketIndex, elementIndex))
			{
				return ComponentCopyData();
			}

			NodeInfo& nodeToCopy = from->m_Nodes(bucketIndex, elementIndex);
			AllocationData allocationData = EmplaceBack(entityID, *nodeToCopy.Data);

			return ComponentCopyData(allocationData.BucketIndex, allocationData.ElementIndex, allocationData.Component);
		}

		virtual inline DataType* GetComponent(const uint64_t& bucketIndex, const uint64_t& elementIndex) override
		{
			return m_Nodes(bucketIndex, elementIndex).Data;
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