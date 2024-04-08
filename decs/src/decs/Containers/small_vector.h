#pragma once

#include <cstdint>
#include <string>
#include <stdexcept>
#include <type_traits>
#include <format>

namespace decs
{
	template<typename T>
	class small_vector
	{
	public:
		using value_type = T;
		using size_type = uint32_t;
		using reference = value_type&;
		using const_reference = const value_type&;

	public:
		small_vector()
		{

		}

		~small_vector()
		{
			DestroyCurrentData();
		}

		small_vector(uint32_t capacity) :
			m_Data(CreateUnitializedArray(capacity)),
			m_Capacity(capacity)
		{
		}

		small_vector(const small_vector& other)
		{
			DestroyCurrentData();

			m_Capacity = other.capacity();
			m_Size = other.size();
			m_Data = CreateUnitializedArray(m_Capacity);

			if (!other.empty())
			{
				CopyData(other.data(), m_Data, m_Size);
			}
		}

		small_vector(small_vector&& other) noexcept
		{
			DestroyCurrentData();

			m_Data = other.m_Data;
			m_Size = other.m_Size;
			m_Capacity = other.m_Capacity;

			other.ResetMembers();
		}

		small_vector& operator=(const small_vector& other)
		{
			DestroyCurrentData();

			m_Capacity = other.capacity();
			m_Size = other.size();
			m_Data = CreateUnitializedArray(m_Capacity);

			if (!other.empty())
			{
				CopyData(other.data(), m_Data, m_Size);
			}
			return *this;
		}

		small_vector& operator=(small_vector&& other) noexcept
		{
			DestroyCurrentData();

			m_Data = other.m_Data;
			m_Size = other.m_Size;
			m_Capacity = other.m_Capacity;

			other.ResetMembers();

			return *this;
		}

		T& operator[](uint32_t idx)
		{
			return m_Data[idx];
		}

		const T& operator[](uint32_t idx) const
		{
			return m_Data[idx];
		}

		inline T& at(uint32_t idx)
		{
			if (idx < m_Size)
			{
				return m_Data[idx];
			}

			throw std::out_of_range(std::format("idx ({0}) in small_vector is greater than size ({1})!", idx, m_Size));
		}

		inline const T& at(uint32_t idx) const
		{
			if (idx < m_Size)
			{
				return m_Data[idx];
			}
			throw std::out_of_range(std::format("idx ({0}) in small_vector is greater than size ({1})!", idx, m_Size));
		}

		inline uint32_t size() const
		{
			return m_Size;
		}

		inline uint32_t capacity() const
		{
			return m_Capacity;
		}

		inline T* data()
		{
			return m_Data;
		}

		inline const T* data() const
		{
			return m_Data;
		}

		inline bool empty() const
		{
			return m_Size == 0;
		}

		inline T& front()
		{
			return *m_Data;
		}

		inline const T& front() const
		{
			return *m_Data;
		}

		inline T& back()
		{
			return m_Data[m_Size - 1];
		}

		inline const T& back() const
		{
			return m_Data[m_Size - 1];
		}

		void reserve(uint32_t newCapacity)
		{
			if (newCapacity > m_Capacity)
			{
				ReserveUnsafe(newCapacity);
			}
		}

		void resize(uint32_t size)
		{
			if (size < m_Size)
			{
				InvokeDestructorsFromTo(m_Data, m_Size - size, m_Size);
				m_Size = size;
			}
			else if (size > m_Size)
			{
				if (size > m_Capacity)
				{
					T* newData = CreateUnitializedArray(size);
					MoveData(m_Data, newData, m_Size);
					DestroyInitializedMemory(m_Data, m_Size);

					InitializeDefault(newData, m_Size, size);

					m_Data = newData;
					m_Capacity = size;
					m_Size = size;
				}
				else
				{
					InitializeDefault(m_Data, m_Size, size);
					m_Size = size;
				}
			}
		}

		void shrink_to_fit()
		{
			if (m_Size < m_Capacity)
			{
				T* newData = CreateUnitializedArray(m_Size);
				MoveData(m_Data, newData, m_Size);
				DestroyInitializedMemory(m_Data, m_Size);

				m_Data = newData;
				m_Capacity = m_Size;
			}
		}

		void clear()
		{
			if (m_Size != 0)
			{
				InvokeDestructorsFromTo(m_Data, 0, m_Size);
				m_Size = 0;
			}
		}

		void push_back(const T& value)
		{
			uint32_t newSize = m_Size + 1;

			if (newSize > m_Capacity)
			{
				ReserveUnsafe(CalculateNewCapacity(m_Capacity));
			}

			m_Size = newSize;
			new(&m_Data[m_Size - 1])T(value);
		}

		template<typename... Args>
		T& emplace_back(Args&&...args)
		{
			uint32_t newSize = m_Size + 1;

			if (newSize > m_Capacity)
			{
				ReserveUnsafe(CalculateNewCapacity(m_Capacity));
			}

			m_Size = newSize;
			return *new(&m_Data[m_Size - 1])T(std::forward<Args>(args)...);
		}

		void pop_back()
		{
			if (m_Size > 0)
			{
				m_Size -= 1;
				m_Data[m_Size].~T();
			}
		}

	private:
		T* m_Data = nullptr;
		uint32_t m_Size = 0;
		uint32_t m_Capacity = 0;

	private:
		void ReserveUnsafe(uint32_t newCapacity)
		{
			T* newData = CreateUnitializedArray(newCapacity);
			MoveData(m_Data, newData, m_Size);
			DestroyInitializedMemory(m_Data, m_Size);

			m_Data = newData;
			m_Capacity = newCapacity;
		}

		inline void ResetMembers()
		{
			m_Data = nullptr;
			m_Size = 0;
			m_Capacity = 0;
		}

		void DestroyCurrentData()
		{
			if (m_Data != nullptr)
			{
				DestroyInitializedMemory(m_Data, m_Size);
			}
		}

	private:
		inline static uint32_t CalculateNewCapacity(uint32_t oldCapacity)
		{
			if (oldCapacity > 1)
			{
				return oldCapacity + oldCapacity / 2;
			}

			return oldCapacity + 1;
		}

		inline static T* CreateUnitializedArray(uint32_t size)
		{
			if (size == 0)
			{
				return nullptr;
			}
			return (T*) ::operator new[](size * sizeof(T));
		}

		inline static void DestroyInitializedMemory(T* data, uint32_t size)
		{
			if (data != nullptr)
			{
				if constexpr (!std::is_arithmetic_v<T> && !std::is_pointer_v<T>)
				{
					// invoke destructors
					for (uint32_t i = 0; i < size; i++)
					{
						data[i].~T();
					}
				}

				::operator delete[](data);
			}
		}

		inline static void MoveData(T* from, T* to, uint32_t size)
		{
			if (size > 0)
			{
				if constexpr (std::is_arithmetic_v<T> || std::is_pointer_v<T>)
				{
					// simple types use memcpy
					memcpy(
						static_cast<void*>(to),
						static_cast<void*>(from),
						static_cast<uint64_t>(size) * sizeof(T)
					);
				}
				else
				{
					for (uint32_t i = 0; i < size; i++)
					{
						new(&to[i])T(std::move(from[i]));
					}
				}
			}
		}

		inline static void CopyData(T* from, T* to, uint32_t size)
		{
			if constexpr (std::is_arithmetic_v<T> || std::is_pointer_v<T>)
			{
				// simple types use memcpy
				memcpy(
					static_cast<void*>(to),
					static_cast<void*>(from),
					static_cast<uint64_t>(size) * sizeof(T)
				);
			}
			else
			{
				for (uint32_t i = 0; i < size; i++)
				{
					to[i] = from[i];
				}
			}
		}

		inline static void InvokeDestructorsFromTo(T* data, uint32_t fromIndex, uint32_t toIndex)
		{
			for (uint32_t i = fromIndex; i < toIndex; i++)
			{
				data[i].~T();
			}
		}

		inline static void InitializeDefault(T* data, uint32_t fromIndex, uint32_t toIndex)
		{
			for (uint32_t i = fromIndex; i < toIndex; i++)
			{
				T* value = new(&data[fromIndex])T();
			}
		}

	};
}