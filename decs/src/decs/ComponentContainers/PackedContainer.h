#pragma once
#include "Core.h"
#include "Containers/TChunkedVector.h"
#include "StableContainer.h"

#include "Component/Component.h"

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

		inline virtual PackedContainerBase* Clone() const = 0;

		inline virtual void PopBack() = 0;

		inline virtual void Clear() = 0;

		inline virtual void ShrinkToFit() = 0;

		inline virtual uint64_t Capacity() = 0;

		inline virtual uint64_t Size() = 0;

		inline virtual void Reserve(uint64_t newCapacity) = 0;

		inline virtual bool HasStableComponents() const = 0;

		/// <summary>
		/// 
		/// </summary>
		/// <returns>Component size in bytes.</returns>
		inline virtual uint64_t GetComponentSize() const = 0;

		inline virtual ComponentBase* GetComponentBasePtr(uint64_t index) = 0;

		inline virtual StableComponentRef* GetStableComponentRef(uint64_t index) = 0;

		inline virtual void RemoveSwapBack(uint64_t index) = 0;

		inline virtual ComponentBase* EmplaceFromBaseComponent(ComponentBase* data) = 0;

		inline virtual ComponentBase* MoveEmplaceBackFromComponentBase(ComponentBase* data) = 0;

		inline virtual ComponentBase* EmplaceFromBaseComponent(StableComponentRef* componentRef) = 0;

		inline virtual ComponentBase* MoveEmplaceBackFromStableComponentRef(StableComponentRef* componentRef) = 0;

	};

	template<typename TComponent>
	class PackedContainer final : public PackedContainerBase
	{
		friend class Container;
		friend class Archetype;
	private:
		std::vector<TComponent> m_Data;

	public:
		PackedContainer()
		{

		}

		~PackedContainer()
		{

		}

		inline virtual bool HasStableComponents() const override
		{
			return false;
		}

		inline virtual uint64_t GetComponentSize() const override
		{
			return sizeof(TComponent);
		}

		virtual PackedContainerBase* Clone() const  override
		{
			return new PackedContainer<TComponent>();
		}

		inline TComponent& GetAsRef(uint64_t index)
		{
			return m_Data[index];
		}

		inline TComponent* GetAsPtr(uint64_t index)
		{
			return &m_Data[index];
		}

		inline virtual void PopBack() override
		{
			if (m_Data.size() > 0)
			{
				m_Data.pop_back();
			}
		}

		inline virtual void Clear() override
		{
			m_Data.clear();
		}

		inline virtual void ShrinkToFit() override
		{
			m_Data.shrink_to_fit();
		}

		inline virtual uint64_t Capacity() override
		{
			return m_Data.capacity();
		}

		inline virtual uint64_t Size() override
		{
			return m_Data.size();
		}

		inline virtual void Reserve(uint64_t newCapacity) override
		{
			m_Data.reserve(newCapacity);
		}

		inline virtual ComponentBase* GetComponentBasePtr(uint64_t index)  override
		{
			return &m_Data[index];
		}

		inline virtual StableComponentRef* GetStableComponentRef(uint64_t index) override
		{
			throw std::runtime_error("Packed container must not use methods with StableComponentRef");
			return nullptr;
		}

		inline virtual void RemoveSwapBack(uint64_t index)override
		{
			if (m_Data.size() > 0)
			{
				if (m_Data.size() > 1) m_Data[index] = std::move(m_Data.back());
				m_Data.pop_back();
			}
		}

		inline virtual ComponentBase* EmplaceFromBaseComponent(ComponentBase* data)   override
		{
			return &m_Data.emplace_back(*static_cast<TComponent*>(data));
		}

		inline virtual ComponentBase* MoveEmplaceBackFromComponentBase(ComponentBase* data) override
		{
			return &m_Data.emplace_back(std::move(*static_cast<TComponent*>(data)));
		}

		inline virtual ComponentBase* EmplaceFromBaseComponent(StableComponentRef* componentRef) override
		{
			throw std::runtime_error("Packed container must not use methods with StableComponentRef");
		}

		inline virtual ComponentBase* MoveEmplaceBackFromStableComponentRef(StableComponentRef* componentRef) override
		{
			throw std::runtime_error("Packed container must not use methods with StableComponentRef");
		}
	};

	template<typename TComponent>
	class StablePackedContainer final : public PackedContainerBase
	{
		friend class Container;
		friend class Archetype;
	private:
		std::vector<StableComponentRef> m_Data;

	public:
		StablePackedContainer()
		{

		}

		~StablePackedContainer()
		{

		}

		inline virtual bool HasStableComponents() const override
		{
			return true;
		}

		inline virtual uint64_t GetComponentSize() const override
		{
			return sizeof(TComponent);
		}

		virtual PackedContainerBase* Clone() const  override
		{
			return new StablePackedContainer<TComponent>();
		}

		inline virtual void PopBack() override
		{
			if (m_Data.size() > 0)
			{
				m_Data.pop_back();
			}
		}

		inline virtual void Clear() override
		{
			m_Data.clear();
		}

		inline virtual void ShrinkToFit() override
		{
			m_Data.shrink_to_fit();
		}

		inline virtual uint64_t Capacity() override
		{
			return m_Data.capacity();
		}

		inline virtual uint64_t Size() override
		{
			return m_Data.size();
		}

		inline virtual void Reserve(uint64_t newCapacity) override
		{
			m_Data.reserve(newCapacity);
		}

		inline virtual ComponentBase* GetComponentBasePtr(uint64_t index)  override
		{
			return m_Data[index].m_ComponentPtr;
		}

		inline virtual StableComponentRef* GetStableComponentRef(uint64_t index)  override
		{
			return &m_Data[index];
		}

		inline virtual void RemoveSwapBack(uint64_t index) override
		{
			uint64_t dataSize = m_Data.size();
			if (dataSize > 0)
			{
				if (dataSize > 1)
				{
					m_Data[index] = m_Data.back();
				}
				m_Data.pop_back();
			}
		}

		inline virtual ComponentBase* EmplaceFromBaseComponent(ComponentBase* data) override
		{
			throw std::runtime_error("Stable Packed container must not use methods with ComponentBase");
		}

		inline virtual ComponentBase* MoveEmplaceBackFromComponentBase(ComponentBase* data) override
		{
			throw std::runtime_error("Stable Packed container must not use methods with ComponentBase");
		}

		inline virtual ComponentBase* EmplaceFromBaseComponent(StableComponentRef* componentRef) override
		{
			m_Data.emplace_back(componentRef->m_ComponentPtr, componentRef->m_ChunkIndex, componentRef->m_Index);
			return componentRef->m_ComponentPtr;
		}

		inline virtual ComponentBase* MoveEmplaceBackFromStableComponentRef(StableComponentRef* componentRef) override
		{
			m_Data.emplace_back(componentRef->m_ComponentPtr, componentRef->m_ChunkIndex, componentRef->m_Index);
			return componentRef->m_ComponentPtr;
		}

		inline TComponent& GetAsRef(uint64_t index)
		{
			return *static_cast<TComponent*>(m_Data[index].m_ComponentPtr);
		}

		inline TComponent* GetAsPtr(uint64_t index)
		{
			return static_cast<TComponent*>(m_Data[index].m_ComponentPtr);
		}

		inline StableComponentRef& EmplaceBack(TComponent* componentPtr, uint64_t chunkIndex, uint64_t elementIndex)
		{
			return m_Data.emplace_back(componentPtr, chunkIndex, elementIndex);
		}
	};
}