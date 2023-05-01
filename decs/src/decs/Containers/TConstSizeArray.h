#pragma once
#include <cstdint>

namespace decs
{
	template<typename T>
	class TConstSizeArray
	{
	public:
		TConstSizeArray(uint64_t size = 10)
		{
			if (size > 0)
			{
				m_Size = size;
				m_Data = new T[m_Size];
			}
		}

		~TConstSizeArray()
		{
			if (m_Data != nullptr)
			{
				delete[] m_Data;
			}
		}

		TConstSizeArray(const TConstSizeArray& other)
		{
			if (other.m_Data != nullptr)
			{
				this->m_Data = new T[other.m_Size];
				this->m_Size = other.m_Size;

				if (this->m_Data != nullptr)
				{
					for (uint64_t i = 0; i < other.m_Size; i++)
					{
						this->m_Data[i] = other.m_Data[i];
					}
				}
			}
			else
			{
				this->m_Data = nullptr;
				this->m_Size = 0;
			}

			for (uint64_t i = 0; i < other.m_Size; i++)
			{
				m_Data[i] = other.m_Data[i];
			}
		}

		TConstSizeArray(TConstSizeArray&& other) noexcept :
			m_Data(other.m_Data),
			m_Size(other.m_Size)
		{
			other.m_Data = nullptr;
			other.m_Size = 0;
		}

		TConstSizeArray& operator=(const TConstSizeArray& other)
		{
			if (m_Data != nullptr)
			{
				delete[] this->m_Data;
			}

			if (other.m_Data != nullptr)
			{
				this->m_Data = new T[other.m_Size];
				this->m_Size = other.m_Size;

				if (this->m_Data != nullptr)
				{
					for (uint64_t i = 0; i < other.m_Size; i++)
					{
						this->m_Data[i] = other.m_Data[i];
					}
				}
			}
			else
			{
				this->m_Data = nullptr;
				this->m_Size = 0;
			}

			return *this;
		}

		TConstSizeArray& operator=(TConstSizeArray&& other) noexcept
		{
			if (m_Data != nullptr)
			{
				delete m_Data;
			}

			this->m_Data = other.m_Data;
			this->m_Size = other.m_Size;
			other.m_Data = nullptr;
			other.m_Size = 0;

			return *this;
		}


		T& operator[](uint64_t idx)
		{
			return m_Data[idx];
		}

		const T& operator[](uint64_t idx) const
		{
			return m_Data[idx];
		}

		inline uint64_t size() const
		{
			return m_Size;
		}

		inline const T* data() const
		{
			return m_Data;
		}

	private:
		T* m_Data = nullptr;
		uint64_t m_Size = 0;
	};
}
