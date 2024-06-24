#pragma once

namespace decs
{
	template<typename NumberType>
	class BitMask
	{
	public:
		using ValueType = NumberType;
		inline static constexpr NumberType BitCount = sizeof(NumberType) * 8;

	public:
		constexpr BitMask() = default;
		constexpr BitMask(NumberType maskValue) :
			m_Mask(maskValue)
		{

		}

		constexpr operator bool() const 
		{
			return m_Mask;
		}

		constexpr bool operator ==(const BitMask& other)
		{
			return m_Mask == other.m_Mask;
		}

		constexpr bool operator !=(const BitMask& other)
		{
			return m_Mask != other.m_Mask;
		}

		inline constexpr BitMask operator |(BitMask rhs) const noexcept
		{
			return { m_Mask | rhs.m_Mask };
		}

		inline constexpr BitMask& operator |=(BitMask rhs) noexcept
		{
			m_Mask |= rhs.m_Mask;
			return *this;
		}

		inline constexpr BitMask operator &(BitMask rhs) const noexcept
		{
			return { m_Mask & rhs.m_Mask };
		}

		inline constexpr BitMask& operator &=(BitMask rhs) noexcept
		{
			m_Mask &= rhs.m_Mask;
			return *this;
		}

		inline constexpr BitMask operator |(NumberType rhs) const noexcept
		{
			return { m_Mask | rhs };
		}

		inline constexpr BitMask& operator |=(NumberType rhs) noexcept
		{
			m_Mask |= rhs;
			return *this;
		}

		inline constexpr BitMask operator &(NumberType rhs) const noexcept
		{
			return { m_Mask & rhs };
		}

		inline constexpr BitMask& operator &=(NumberType rhs) noexcept
		{
			m_Mask &= rhs;
			return *this;
		}

		inline constexpr BitMask operator~() const noexcept
		{
			return { ~m_Mask };
		}

		inline constexpr void SetBit(bool bitValue, NumberType bitIndex)
		{
			constexpr NumberType bitsCount = sizeof(NumberType) * 8;
			if (bitIndex < bitsCount)
			{
				if (bitValue)
				{
					m_Mask |= 1 << bitIndex;
				}
				else
				{
					m_Mask &= ~(1 << bitIndex);
				}
			}
		}

		inline constexpr bool GetBit(NumberType bitIndex) const
		{
			return (m_Mask & (1 << bitIndex)) > 0;
		}

		explicit constexpr operator NumberType() const noexcept
		{
			return m_Mask;
		}

		inline constexpr NumberType GetValue() const noexcept
		{
			return m_Mask;
		}

		inline constexpr static BitMask<NumberType> FullMask()
		{
			return BitMask<NumberType>(std::numeric_limits<NumberType>::max());
		}
	private:
		NumberType m_Mask = 0;
	};
}