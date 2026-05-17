#pragma once
#include "decs/Core/Core.h"
#include "decs/Core/trait.h"
#include "decs/Core/check_cast.h"

namespace decs::light
{
	class IPackedLightComponentContainer
	{
	public:
		IPackedLightComponentContainer() = default;

		virtual ~IPackedLightComponentContainer() = default;

		virtual void PopBack() = 0;

		virtual void Clear() = 0;

		virtual void ShrinkToFit() = 0;

		virtual uint64_t Capacity() = 0;

		virtual uint64_t Size() = 0;

		virtual void Reserve(uint64_t newCapacity) = 0;

		/// <summary>
		/// 
		/// </summary>
		/// <returns>Component size in bytes.</returns>
		virtual uint64_t GetComponentSize() const = 0;

		virtual void* GetComponentBasePtr(uint64_t index) = 0;

		virtual void* GetBackComponentBasePtr() = 0;

		virtual void RemoveSwapBack(uint64_t index) = 0;

		virtual void PushBack(void* componentPtr) = 0;

		virtual void MoveBack(void* componentPtr) = 0;

		virtual IPackedLightComponentContainer* CloneEmpty() const = 0;

		/// <summary>
		/// Adds default constructed new component on end of container
		/// </summary>
		virtual void PushBackDefault() = 0;
	};

	template<typename TComponent>
	class PackedLightComponentContainer final : public IPackedLightComponentContainer
	{
		static_assert(!is_const_v<TComponent> && "Component must not be const!");

		friend class Container;
		friend class Archetype;
	private:
		std::vector<TComponent> m_Data{};

	public:
		PackedLightComponentContainer() = default;

		~PackedLightComponentContainer() = default;

		inline uint64_t GetComponentSize() const override
		{
			return sizeof(TComponent);
		}

		inline void PopBack() override
		{
			if (!m_Data.empty())
			{
				m_Data.pop_back();
			}
		}

		inline void Clear() override
		{
			m_Data.clear();
		}

		inline void ShrinkToFit() override
		{
			m_Data.shrink_to_fit();
		}

		inline uint64_t Capacity() override
		{
			return m_Data.capacity();
		}

		inline uint64_t Size() override
		{
			return m_Data.size();
		}

		inline void Reserve(uint64_t newCapacity) override
		{
			m_Data.reserve(newCapacity);
		}

		inline void* GetComponentBasePtr(uint64_t index) override
		{
			return &m_Data[index];
		}

		inline void* GetBackComponentBasePtr() override
		{
			DECS_ASSERT(!m_Data.empty(), "m_Data must not be empty!");
			return &m_Data.back();
		}

		inline void RemoveSwapBack(uint64_t index) override
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

		inline TComponent& GetAsRef(size_t index)
		{
			return m_Data[index];
		}

		inline TComponent* GetAsPtr(size_t index)
		{
			return &m_Data[index];
		}

		template<typename... Args>
		inline TComponent& EmplaceBack(Args&&...args)
		{
			return m_Data.emplace_back(std::forward<Args>(args)...);
		}

		inline IPackedLightComponentContainer* CloneEmpty() const override
		{
			return new PackedLightComponentContainer<TComponent>();
		}

		inline void PushBackDefault() override
		{
			m_Data.emplace_back();
		}

		inline std::span<TComponent> GetAsSpan()
		{
			return { m_Data };
		}
	};

}