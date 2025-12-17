#pragma once
#include "decs/Core.h"
#include "decs/Containers/TChunkedVector.h"
#include"decs/check_cast.h"

namespace decs
{
	class IPackedComponentContainer
	{
	public:
		IPackedComponentContainer()
		{

		}

		virtual ~IPackedComponentContainer()
		{

		}

		//inline virtual IPackedComponentContainer* Clone() const = 0;

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

		inline virtual void* GetComponentBasePtr(uint64_t index) = 0;

		inline virtual void RemoveSwapBack(uint64_t index) = 0;

		inline virtual void PushBack(void* componentPtr) = 0;

		inline virtual void MoveBack(void* componentPtr) = 0;

		inline virtual IPackedComponentContainer* CloneEmpty() const = 0;
	};

	template<typename TComponent>
	class PackedComponentContainer final : public IPackedComponentContainer
	{
		static_assert(!is_const_v<TComponent> && "Component must not be const!");

		friend class Container;
		friend class Archetype;
	private:
		std::vector<TComponent> m_Data{};

	public:
		PackedComponentContainer()
		{

		}

		~PackedComponentContainer()
		{

		}

		inline  uint64_t GetComponentSize() const override
		{
			return sizeof(TComponent);
		}

		inline  void PopBack() override
		{
			if (m_Data.size() > 0)
			{
				m_Data.pop_back();
			}
		}

		inline  void Clear() override
		{
			m_Data.clear();
		}

		inline  void ShrinkToFit() override
		{
			m_Data.shrink_to_fit();
		}

		inline  uint64_t Capacity() override
		{
			return m_Data.capacity();
		}

		inline  uint64_t Size() override
		{
			return m_Data.size();
		}

		inline  void Reserve(uint64_t newCapacity) override
		{
			m_Data.reserve(newCapacity);
		}

		inline  void* GetComponentBasePtr(uint64_t index)  override
		{
			return &m_Data[index];
		}

		inline  void RemoveSwapBack(uint64_t index) override
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

		inline void PushBack(void* componentPtr) override
		{
			m_Data.emplace_back(*static_cast<TComponent*>(componentPtr));
		}

		inline void MoveBack(void* componentPtr) override
		{
			m_Data.emplace_back(std::move(*static_cast<TComponent*>(componentPtr)));
		}

		inline TComponent& GetAsRef(uint64_t index)
		{
			return m_Data[index];
		}

		inline TComponent* GetAsPtr(uint64_t index)
		{
			return &m_Data[index];
		}

		template<typename... Args>
		inline TComponent& EmplaceBack(Args&&...args)
		{
			return m_Data.emplace_back(std::forward<Args>(args)...);
		}

		inline virtual IPackedComponentContainer* CloneEmpty() const override
		{
			return new PackedComponentContainer<TComponent>();
		}
	};

}