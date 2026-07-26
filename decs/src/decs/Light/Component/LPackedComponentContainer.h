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

		virtual size_t Capacity() = 0;

		virtual size_t Size() = 0;

		virtual void Reserve(size_t newCapacity) = 0;

		/// <summary>
		/// 
		/// </summary>
		/// <returns>Component size in bytes.</returns>
		virtual size_t GetComponentSize() const = 0;

		virtual void* GetComponentBasePtr(size_t index) = 0;

		virtual void* GetBackComponentBasePtr() = 0;

		virtual void RemoveSwapBack(size_t index) = 0;

		virtual void PushBack(void* componentPtr) = 0;

		virtual void MoveBack(void* componentPtr) = 0;

		virtual IPackedLightComponentContainer* CloneEmpty() const = 0;

		/// <summary>
		/// Adds default constructed new component on end of container
		/// </summary>
		virtual void PushBackDefault() = 0;
	};

	template<typename ComponentType>
	class PackedLightComponentContainer final : public IPackedLightComponentContainer
	{
		friend class Container;
		friend class Archetype;

	public:
		using component_type = ComponentType;
		using get_span_result = std::span<typename component_type>;

	private:
		ecsVector<ComponentType> m_Data{};

	public:
		PackedLightComponentContainer() = default;

		~PackedLightComponentContainer() = default;

		inline size_t GetComponentSize() const override
		{
			return sizeof(ComponentType);
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

		inline size_t Capacity() override
		{
			return m_Data.capacity();
		}

		inline size_t Size() override
		{
			return m_Data.size();
		}

		inline void Reserve(size_t newCapacity) override
		{
			m_Data.reserve(newCapacity);
		}

		inline void* GetComponentBasePtr(size_t index) override
		{
			return &m_Data[index];
		}

		inline void* GetBackComponentBasePtr() override
		{
			DECS_ASSERT(!m_Data.empty(), "m_Data must not be empty!");
			return &m_Data.back();
		}

		inline void RemoveSwapBack(size_t index) override
		{
			size_t dataSize = m_Data.size();
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
			m_Data.emplace_back(*static_cast<ComponentType*>(componentPtr));
		}

		inline void MoveBack(void* componentPtr) override
		{
			m_Data.emplace_back(std::move(*static_cast<ComponentType*>(componentPtr)));
		}

		inline ComponentType& GetAsRef(size_t index)
		{
			return m_Data[index];
		}

		inline ComponentType* GetAsPtr(size_t index)
		{
			return &m_Data[index];
		}

		inline void Set(size_t index, const ComponentType& component)
		{
			m_Data[index] = component;
		}

		template<typename... Args>
		inline ComponentType& EmplaceBack(Args&&...args)
		{
			return m_Data.emplace_back(std::forward<Args>(args)...);
		}

		inline IPackedLightComponentContainer* CloneEmpty() const override
		{
			return new PackedLightComponentContainer<ComponentType>();
		}

		inline void PushBackDefault() override
		{
			m_Data.emplace_back();
		}

		inline std::span<ComponentType> GetAsSpan()
		{
			return { m_Data };
		}
	};

}