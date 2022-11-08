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
		inline bool IsEmpty()const { m_CreatedElements == 0; }

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
	class StableComponentAllocator : public ComponentAllocator<ComponentType>
	{
		using AllocationData = ComponentAllocationData<ComponentType>;
		using NodeInfo = ComponentNodeInfo<ComponentType>;
	public:
		StableComponentAllocator()
		{

		}

		StableComponentAllocator(const uint64_t& bucketCapacity) :
			m_Components(bucketCapacity),
			m_Nodes(bucketCapacity)
		{

		}

		~StableComponentAllocator()
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
			using ContainerType = StableComponentAllocator<ComponentType>;
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
}