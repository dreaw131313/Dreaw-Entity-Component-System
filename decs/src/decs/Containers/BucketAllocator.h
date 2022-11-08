#pragma once
#include "decspch.h"

#include "../Core.h"

namespace decs
{
	template<typename T>
	class Chunk;

	template<typename DataType>
	struct BucketNode
	{
		template<typename T>
		friend class Chunk;
		template<typename T>
		friend class BucketAllocator;

		typedef Chunk<DataType> BucketType;
	public:
		inline BucketNode<DataType>* Next() { return m_Next; }
		inline BucketNode<DataType>* Previous() { return m_Previous; }

	private:
		BucketNode()
		{

		}

	public:
		template<typename... Args>
		BucketNode(
			BucketType* bucket,
			BucketNode<DataType>* previous,
			BucketNode<DataType>* next,
			Args&&... args
		) :
			m_Bucket(bucket),
			m_Previous(previous),
			m_Next(next),
			m_Data(std::forward<Args>(args)...)
		{

		}

		~BucketNode()
		{

		}

	public:
		inline DataType& Data() { return m_Data; }

	private:
		DataType m_Data;
		BucketNode<DataType>* m_Previous = nullptr;
		BucketNode<DataType>* m_Next = nullptr;
		BucketType* m_Bucket = nullptr;
	};

	template<typename Data>
	class Chunk
	{
		template<typename T>
		friend class BucketAllocator;

		typedef BucketNode<Data> NodeType;

		struct FreeNode
		{
		public:
			NodeType* Node;
			NodeType* Previous;
			NodeType* Next;

		public:
			FreeNode(
				NodeType* node,
				NodeType* previous,
				NodeType* next
			) :
				Node(node),
				Previous(previous),
				Next(next)
			{

			}
		};

	public:
		Chunk(uint64_t elementsCount)
		{
			if (elementsCount == 0)
			{
				m_MaxSize = 1;
			}
			else
			{
				m_MaxSize = elementsCount;
			}
			m_FreeNodes.reserve(m_MaxSize);
			m_NodesArray = (NodeType*) ::operator new(m_MaxSize * sizeof(NodeType));
		}

		~Chunk()
		{
			NodeType* node = m_FirstNode;
			while (node != nullptr)
			{
				NodeType* next = node->Next();

				node->~NodeType();
				node = next;
			}

			::operator delete(m_NodesArray, m_MaxSize * sizeof(NodeType));
		}

		inline bool IsEmpty() const { return m_ValidElements == 0; }
		inline bool HasFreeSpace()const { return m_ValidElements < m_MaxSize; }
		inline uint64_t MaxAllocatedIndex() const { return m_CurrentAllocationIndex; }
		inline uint64_t Size()const { return m_CurrentAllocationIndex; }
		inline NodeType* Data() { return m_NodesArray; }
		inline uint64_t ValidElements() const { return m_ValidElements; }
		inline bool IsFull() const { return m_ValidElements == m_MaxSize; }

		inline NodeType* FirstNode() { return m_FirstNode; }
		inline NodeType* LastNode() { return m_LastNode; }

		template<typename... Args>
		NodeType* AddNode(Args&&... args)
		{
			if (m_FreeNodes.size() > 0)
			{
				FreeNode freeSpace = m_FreeNodes.back();
				m_FreeNodes.pop_back();

				NodeType* node = new(freeSpace.Node) NodeType(
					this,
					freeSpace.Previous,
					freeSpace.Next,
					std::forward<Args>(args)...
				);

				if (freeSpace.Previous != nullptr)
				{
					freeSpace.Previous->m_Next = node;
				}
				else
				{
					m_FirstNode = node;
				}

				if (freeSpace.Next != nullptr)
				{
					freeSpace.Next->m_Previous = node;
				}
				else
				{
					m_LastNode = node;
				}

				m_ValidElements += 1;
				return node;
			}

			if (m_CurrentAllocationIndex < m_MaxSize)
			{

				NodeType* node = new(&m_NodesArray[m_CurrentAllocationIndex]) NodeType(this, m_LastNode, nullptr, std::forward<Args>(args)...);

				if (m_FirstNode == nullptr) m_FirstNode = node;

				if (m_LastNode == nullptr)
				{
					m_LastNode = node;
				}
				else
				{
					m_LastNode->m_Next = node;
					m_LastNode = node;
				}

				m_ValidElements += 1;
				m_CurrentAllocationIndex += 1;
				return node;
			}

			return nullptr;
		}

		bool RemoveNode(NodeType* node)
		{
			if (node == nullptr || m_ValidElements == 0 || node->m_Bucket != this) return false;

			if (m_ValidElements == 1)
			{
				m_FreeNodes.clear();
				m_CurrentAllocationIndex = 0;
				m_FirstNode = nullptr;
				m_LastNode = nullptr;
				m_ValidElements = 0;
			}
			else
			{
				FreeNode& freeSpace = m_FreeNodes.emplace_back(node, node->m_Previous, node->m_Next);

				if (node == m_LastNode)
				{
					m_LastNode = node->m_Previous;
					m_LastNode->m_Next = nullptr;
				}
				else if (node == m_FirstNode)
				{
					m_FirstNode = node->m_Next;
					m_FirstNode->m_Previous = nullptr;
				}
				else
				{
					freeSpace.Next->m_Previous = freeSpace.Previous;
					freeSpace.Previous->m_Next = freeSpace.Next;
				}

				m_ValidElements -= 1;
			}

			node->~BucketNode();

			return true;
		}

	private:
		uint64_t m_MaxSize; // number of elements:
		NodeType* m_NodesArray;

		std::vector<FreeNode> m_FreeNodes;

		NodeType* m_FirstNode = nullptr;
		NodeType* m_LastNode = nullptr;

		// 
		uint64_t m_CurrentAllocationIndex = 0;

		//
		uint64_t m_ValidElements = 0;


	};

	template<typename DataType>
	class BucketAllocator
	{
		typedef Chunk<DataType> BucketType;
		typedef BucketNode<DataType> NodeType;

	public:
		BucketAllocator() {}

		BucketAllocator(uint64_t elementsInBucket, bool bAutoRemoveEmptyBuckets = true) :
			m_bAutoRemoveEmptyBuckets(bAutoRemoveEmptyBuckets)
		{
			if (elementsInBucket == 0)
			{
				m_DefaultBucketElementCount = 1;
			}
			else
			{
				m_DefaultBucketElementCount = elementsInBucket;
			}
		}

		~BucketAllocator()
		{
			for (uint64_t bIdx = 0; bIdx < m_Buckets.size(); bIdx++)
			{
				delete m_Buckets[bIdx];
			}
		}

		template<typename... Args>
		NodeType* AddNode(Args&&... args)
		{
			NodeType* newNode = nullptr;

			if (m_BucketsWithFreeSpace.size() > 0)
			{
				BucketType* bucket = m_BucketsWithFreeSpace.front();

				newNode = bucket->AddNode(std::forward<Args>(args)...);

				if (!bucket->HasFreeSpace())
				{
					RemoveBucketFromFreeSpaceBuckets(bucket);
				}
			}
			else
			{
				BucketType* newBucket = AddBucket();
				newNode = newBucket->AddNode(std::forward<Args>(args)...);


				if (!newBucket->HasFreeSpace())
				{
					RemoveBucketFromFreeSpaceBuckets(newBucket);
				}
			}

			if (newNode != nullptr)
			{
				m_CreatedElements += 1;
			}

			return newNode;
		}

		bool RemoveNode(NodeType* node)
		{
			BucketType* bucket = node->m_Bucket;
			auto it = m_BucketIndexes.find(bucket);

			if (it == m_BucketIndexes.end()) return false;

			bool wasBucketFull = bucket->IsFull();

			bucket->RemoveNode(node);

			if (m_bAutoRemoveEmptyBuckets && m_BucketsWithFreeSpace.size() > 1 && bucket->IsEmpty())
			{
				RemoveBucket(bucket);
			}
			else if (wasBucketFull)
			{
				AddBucketToFreeSpaceBuckets(bucket);
			}

			m_CreatedElements -= 1;
			return true;
		}

		inline uint64_t Size() const { return m_CreatedElements; }

		inline uint64_t BucketsCount() const { return m_Buckets.size(); }

		inline BucketType** Buckets() { return m_Buckets.data(); }

		void DestroyFreeBuckets()
		{
			if (m_BucketsWithFreeSpace.size() == 1) return;

			for (uint64_t idx = 0; idx < m_BucketsWithFreeSpace.size(); idx++)
			{
				auto bucket = m_BucketsWithFreeSpace[idx];
				if (bucket->IsEmpty())
				{
					RemoveBucket(bucket);
					idx -= 1;
					if (m_BucketsWithFreeSpace.size() == 1)
					{
						break;
					}
				}
			}
		}

	private:
		uint64_t m_CreatedElements = 0;

		uint64_t m_DefaultBucketElementCount = 1;

		std::unordered_map<BucketType*, uint64_t> m_BucketIndexes;
		std::vector<BucketType*> m_Buckets;

		std::unordered_map<BucketType*, uint64_t> m_BucketWithFreeSpaceIndexes;
		std::vector<BucketType*> m_BucketsWithFreeSpace;

		bool m_bAutoRemoveEmptyBuckets = true;


		BucketType* AddBucket()
		{
			BucketType* newBucket = new BucketType(m_DefaultBucketElementCount);
			m_BucketIndexes[newBucket] = m_Buckets.size();
			m_Buckets.push_back(newBucket);

			m_BucketWithFreeSpaceIndexes[newBucket] = m_BucketsWithFreeSpace.size();
			m_BucketsWithFreeSpace.push_back(newBucket);

			return newBucket;
		}

		void RemoveBucket(BucketType* bucket)
		{
			auto it = m_BucketIndexes.find(bucket);

			// all buckets removing:
			BucketType* lastBucket = m_Buckets.back();
			if (lastBucket != bucket)
			{
				m_Buckets[it->second] = lastBucket;
				m_BucketIndexes[lastBucket] = it->second;
			}
			m_Buckets.pop_back();
			m_BucketIndexes.erase(it);

			// buckets with free spaces removing:
			auto fsIt = m_BucketWithFreeSpaceIndexes.find(bucket);
			if (fsIt != m_BucketWithFreeSpaceIndexes.end())
			{
				lastBucket = m_BucketsWithFreeSpace.back();
				if (lastBucket != bucket)
				{
					m_BucketWithFreeSpaceIndexes[lastBucket] = fsIt->second;
					m_BucketsWithFreeSpace[fsIt->second] = lastBucket;
				}

				m_BucketsWithFreeSpace.pop_back();
				m_BucketWithFreeSpaceIndexes.erase(fsIt);
			}

			delete bucket;
		}

		inline void RemoveBucketFromFreeSpaceBuckets(BucketType* bucket)
		{
			// buckets with free spaces removing:
			auto fsIt = m_BucketWithFreeSpaceIndexes.find(bucket);
			if (fsIt != m_BucketWithFreeSpaceIndexes.end())
			{
				BucketType* lastBucket = m_BucketsWithFreeSpace.back();
				if (lastBucket != bucket)
				{
					m_BucketWithFreeSpaceIndexes[lastBucket] = fsIt->second;
					m_BucketsWithFreeSpace[fsIt->second] = lastBucket;
				}

				m_BucketsWithFreeSpace.pop_back();
				m_BucketWithFreeSpaceIndexes.erase(fsIt);
			}
		}

		inline void AddBucketToFreeSpaceBuckets(BucketType* bucket)
		{
			auto it = m_BucketWithFreeSpaceIndexes.find(bucket);
			if (it == m_BucketWithFreeSpaceIndexes.end())
			{
				m_BucketWithFreeSpaceIndexes[bucket] = m_BucketsWithFreeSpace.size();
				m_BucketsWithFreeSpace.push_back(bucket);
			}
		}

		// ITERATOR:
	public:
		class iterator
		{
		public:
			iterator(BucketAllocator* bucketAllocator)
			{
				m_BucketsCount = bucketAllocator->BucketsCount();
				if (m_BucketsCount > 0)
				{
					m_Buckets = bucketAllocator->Buckets();
					m_BucketIndex = 0;

					for (; m_BucketIndex < m_BucketsCount; m_BucketIndex++)
					{
						m_Bucket = m_Buckets[m_BucketIndex];
						if (m_Bucket->ValidElements() > 0)
						{
							m_Node = m_Bucket->m_FirstNode;
							return;
						}
					}
				}

				Invalidate();
			}

			~iterator() {}

			inline void Advance()
			{
				m_Node = m_Node->m_Next;
			}

			inline NodeType* Node() { return m_Node; }

			inline DataType& Data() { return m_Node->Data(); }

			inline bool IsValid()
			{
				if (m_Node != nullptr)
				{
					return true;
				}
				else
				{
					m_BucketIndex += 1;
					for (; m_BucketIndex < m_BucketsCount; m_BucketIndex++)
					{
						m_Bucket = m_Buckets[m_BucketIndex];
						if (m_Bucket->ValidElements() > 0)
						{
							m_Node = m_Bucket->m_FirstNode;
							return true;
						}
					}
				}

				return false;
			}

			inline NodeType* operator->()
			{
				return m_Node;
			}


			/*NodeType* operator->()
			{
				return m_Node;
			}*/

		private:
			BucketType** m_Buckets = nullptr;
			uint64_t m_BucketsCount = 0;
			BucketType* m_Bucket = nullptr;
			uint64_t m_BucketIndex = 0;
			NodeType* m_Node = nullptr;

			inline void Invalidate()
			{
				m_Buckets = nullptr;
				m_BucketsCount = 0;
				m_Bucket = nullptr;
				m_BucketIndex = 0;
				m_Node = nullptr;
			}

		};

		inline iterator GetInterator() { return iterator(this); }

	};
}