#pragma once

#include <stdint.h>
#include <vector>

namespace decs
{
	template<typename NumberType>
	class Bitmask
	{
	public:
		using ValueType = NumberType;
		inline static constexpr NumberType BitCount = sizeof(NumberType) * NumberType(8);

	public:
		constexpr Bitmask() = default;
		constexpr Bitmask(NumberType maskValue):
			m_Mask(maskValue)
		{

		}

		constexpr bool operator ==(const Bitmask& other)
		{
			return m_Mask == other.m_Mask;
		}

		constexpr bool operator !=(const Bitmask& other)
		{
			return m_Mask != other.m_Mask;
		}

		inline constexpr Bitmask operator |(Bitmask rhs) const noexcept
		{
			return { m_Mask | rhs.m_Mask };
		}

		inline constexpr Bitmask& operator |=(Bitmask rhs) noexcept
		{
			m_Mask |= rhs.m_Mask;
			return *this;
		}

		inline constexpr Bitmask operator &(Bitmask rhs) const noexcept
		{
			return { m_Mask & rhs.m_Mask };
		}

		inline constexpr Bitmask& operator &=(Bitmask rhs) noexcept
		{
			m_Mask &= rhs.m_Mask;
			return *this;
		}

		inline constexpr Bitmask operator |(NumberType rhs) const noexcept
		{
			return { m_Mask | rhs };
		}

		inline constexpr Bitmask& operator |=(NumberType rhs) noexcept
		{
			m_Mask |= rhs;
			return *this;
		}

		inline constexpr Bitmask operator &(NumberType rhs) const noexcept
		{
			return { m_Mask & rhs };
		}

		inline constexpr Bitmask& operator &=(NumberType rhs) noexcept
		{
			m_Mask &= rhs;
			return *this;
		}

		inline constexpr Bitmask operator~() const noexcept
		{
			return { ~m_Mask };
		}

		inline constexpr void SetBit(NumberType bitIndex, bool bitValue)
		{
			constexpr NumberType bitsCount = sizeof(NumberType) * NumberType(8);
			if (bitIndex < bitsCount)
			{
				if (bitValue)
				{
					m_Mask |= NumberType(1) << bitIndex;
				}
				else
				{
					m_Mask &= ~(NumberType(1) << bitIndex);
				}
			}
		}

		inline constexpr bool GetBit(NumberType bitIndex) const
		{
			return (m_Mask & (NumberType(1) << bitIndex)) > NumberType(0);
		}

		explicit constexpr operator NumberType() const noexcept
		{
			return m_Mask;
		}

		inline constexpr NumberType GetValue() const noexcept
		{
			return m_Mask;
		}

		inline constexpr static Bitmask<NumberType> FullMask()
		{
			return Bitmask<NumberType>(std::numeric_limits<NumberType>::max());
		}
	private:
		NumberType m_Mask = NumberType(0);
	};

	template<typename NumberType, typename EnumType>
	class EnumBitmask
	{
	public:
		using ValueType = NumberType;
		using BitmaskType = Bitmask<NumberType>;
		inline static constexpr NumberType BitCount = BitmaskType::BitCount;

	public:
		EnumBitmask() = default;

		constexpr EnumBitmask(ValueType maskValue):
			m_Bitmask(maskValue)
		{

		}

		constexpr EnumBitmask(EnumType enumValue):
			m_Bitmask(static_cast<ValueType>(enumValue))
		{

		}

		constexpr EnumBitmask(BitmaskType bitmask):
			m_Bitmask(bitmask)
		{

		}

		bool operator ==(const EnumBitmask& other)
		{
			return m_Bitmask == other.m_Bitmask;
		}

		bool operator !=(const EnumBitmask& other)
		{
			return m_Bitmask != other.m_Bitmask;
		}

		inline constexpr EnumBitmask operator |(ValueType rhs) const noexcept
		{
			return { m_Bitmask | rhs };
		}

		inline constexpr EnumBitmask& operator |=(NumberType rhs) noexcept
		{
			m_Bitmask |= rhs;
			return *this;
		}

		inline constexpr EnumBitmask operator &(ValueType rhs) const noexcept
		{
			return { m_Bitmask & rhs };
		}

		inline constexpr EnumBitmask& operator &=(ValueType rhs) noexcept
		{
			m_Bitmask &= rhs;
			return *this;
		}

		inline constexpr EnumBitmask operator |(EnumBitmask rhs) const noexcept
		{
			return { m_Bitmask | static_cast<ValueType>(rhs.m_Bitmask) };
		}

		inline constexpr EnumBitmask& operator |=(EnumBitmask rhs) noexcept
		{
			m_Bitmask |= static_cast<ValueType>(rhs.m_Bitmask);
			return *this;
		}

		inline constexpr EnumBitmask operator &(EnumBitmask rhs) const noexcept
		{
			return { m_Bitmask & static_cast<ValueType>(rhs.m_Bitmask) };
		}

		inline constexpr EnumBitmask& operator &=(EnumBitmask rhs) noexcept
		{
			m_Bitmask &= static_cast<ValueType>(rhs.m_Bitmask);
			return *this;
		}

		inline constexpr EnumBitmask operator |(EnumType rhs) const noexcept
		{
			return { m_Bitmask | static_cast<ValueType>(rhs) };
		}

		inline constexpr EnumBitmask& operator |=(EnumType rhs) noexcept
		{
			m_Bitmask |= static_cast<ValueType>(rhs);
			return *this;
		}

		inline constexpr EnumBitmask operator &(EnumType rhs) const noexcept
		{
			return { m_Bitmask & static_cast<ValueType>(rhs) };
		}

		inline constexpr EnumBitmask& operator &=(EnumType rhs) noexcept
		{
			m_Bitmask &= static_cast<ValueType>(rhs);
			return *this;
		}

		inline constexpr EnumBitmask operator~() const noexcept
		{
			return { ~m_Bitmask };
		}

		inline constexpr static EnumBitmask<NumberType, EnumType> FullMask() noexcept
		{
			return EnumBitmask<BitmaskType, EnumType>(std::numeric_limits<ValueType>::max());
		}

		explicit constexpr operator NumberType() const noexcept
		{
			return m_Bitmask.GetValue();
		}

		inline constexpr ValueType GetValue() const noexcept
		{
			return m_Bitmask.GetValue();
		}

		inline constexpr BitmaskType GetBitmask() const
		{
			return m_Bitmask;
		}

		template<typename T>
		inline constexpr T GetValueAs() const noexcept
		{
			return static_cast<T>(m_Bitmask.GetValue());
		}

		template<typename T>
		inline constexpr T as() const noexcept
		{
			return static_cast<T>(m_Bitmask.GetValue());
		}

		inline constexpr bool Has(EnumType value) const
		{
			return (m_Bitmask & static_cast<NumberType>(value)).GetValue();
		}

		inline constexpr void Add(EnumType value)
		{
			m_Bitmask |= static_cast<NumberType>(value);
		}

		inline constexpr void Remove(EnumType value)
		{
			m_Bitmask &= ~static_cast<NumberType>(value);
		}

	private:
		BitmaskType m_Bitmask = NumberType(0);
	};

	template<typename NumberType, typename EnumType>
	bool operator ==(const EnumBitmask<NumberType, EnumType>& lhs, const EnumBitmask<NumberType, EnumType>& rhs)
	{
		return lhs.GetValue() == rhs.GetValue();
	}

}

namespace std
{

	template<typename TNumberType>
	struct hash<decs::Bitmask<TNumberType>>
	{
		std::size_t operator()(const decs::Bitmask<TNumberType>& value) const
		{
			return std::hash<TNumberType>{}(value.GetValue());
		}
	};

	template<typename TNumberType, typename TEnumType>
	struct hash<decs::EnumBitmask<TNumberType, TEnumType>>
	{
		std::size_t operator()(const decs::EnumBitmask<TNumberType, TEnumType>& value) const
		{
			return std::hash<TNumberType>{}(value.GetValue());
		}
	};
}