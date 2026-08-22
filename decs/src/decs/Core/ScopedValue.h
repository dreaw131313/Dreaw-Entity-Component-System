#pragma once

namespace decs
{
	template<typename T>
	class ScopedValue
	{
	public:
		ScopedValue() = delete;
		ScopedValue(const ScopedValue&) = delete;
		ScopedValue& operator=(const ScopedValue&) = delete;
		ScopedValue(ScopedValue&&) noexcept = delete;
		ScopedValue& operator=(ScopedValue&&) noexcept = delete;

		ScopedValue(T& v, const T& newValue, const T& valueToRestore) :
			m_Value(v),
			m_ValueToRestore(valueToRestore)
		{
			m_Value = newValue;
		}

		ScopedValue(T& v, const T& newValue) :
			m_Value(v),
			m_ValueToRestore(v)
		{
			m_Value = newValue;
		}

		~ScopedValue()
		{
			m_Value = m_ValueToRestore;
		}

	private:
		T& m_Value;
		T m_ValueToRestore;
	};
}