#pragma once
#include "decs/Core.h"
#include "decs/Containers/TChunkedVector.h"
#include "StableContainer.h"

#include "decs/Component/Component.h"

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

		//inline virtual PackedContainerBase* Clone() const = 0;

		inline virtual void PopBack() = 0;

		inline virtual void Clear() = 0;

		inline virtual void ShrinkToFit() = 0;

		inline virtual uint64_t Capacity() = 0;

		inline virtual uint64_t Size() = 0;

		inline virtual void Reserve(uint64_t newCapacity) = 0;

		/// <summary>
		/// 
		/// </summary>
		/// <returns>Component size in bytes.</returns>
		inline virtual uint64_t GetComponentSize() const = 0;

		inline virtual EntityComponent* GetComponentBasePtr(uint64_t index) = 0;

		inline virtual void RemoveSwapBack(uint64_t index) = 0;

		inline virtual void PushBack(EntityComponent* componentBase) = 0;
	};

	template<typename TComponent>
	class StablePackedContainer final : public PackedContainerBase
	{
		static_assert(!is_const_v<TComponent> && "Component must not be const!");

		friend class Container;
		friend class Archetype;
	private:
		std::vector<TComponent*> m_Data;

	public:
		StablePackedContainer()
		{

		}

		~StablePackedContainer()
		{

		}

		inline virtual uint64_t GetComponentSize() const override
		{
			return sizeof(TComponent);
		}

		/*virtual PackedContainerBase* Clone() const  override
		{
			return new StablePackedContainer<TComponent>();
		}*/

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

		inline virtual EntityComponent* GetComponentBasePtr(uint64_t index)  override
		{
			return m_Data[index];
		}

		inline virtual void RemoveSwapBack(uint64_t index) override
		{
			uint64_t dataSize = m_Data.size();
			if (dataSize > 0)
			{
				if (index < (dataSize - 1))
				{
					m_Data[index] = m_Data.back();
				}
				m_Data.pop_back();
			}
		}

		inline void PushBack(EntityComponent* componentBase) override
		{
			m_Data.push_back(static_cast<TComponent*>(componentBase));
		}

		inline TComponent& GetAsRef(uint64_t index)
		{
			return *m_Data[index];
		}

		inline TComponent* GetAsPtr(uint64_t index)
		{
			return m_Data[index];
		}
	};
}