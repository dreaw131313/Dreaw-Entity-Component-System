#pragma once

#include <functional>
#include <vector>
#include <cstdint>

#include "Core.h"


namespace decs
{
	template<typename...>
	struct TObserverFunction;

	struct ObserverFunctionID final
	{
		template<typename...>
		friend struct TObserverFunction;

	public:
		bool operator == (const ObserverFunctionID& other) const noexcept
		{
			return this->m_Value == other.m_Value;
		}

		bool operator != (const ObserverFunctionID& other) const noexcept
		{
			return this->m_Value != other.m_Value;
		}
	private:
		size_t m_Value = std::numeric_limits<size_t>::max();
	};

	template<typename... Args>
	struct TObserverFunction;

	template<typename... Args>
	struct TObserverFunction<void(Args...)> final
	{
		using ObserverFunctionType = void(Args...);
		using FunctionType = std::function<ObserverFunctionType>;

	private:
		struct ItemRecord
		{
		public:
			FunctionType m_Function{};
			ObserverFunctionID m_ID{};
			int m_Order = 0;
		};

	public:
		template<typename Func>
		ObserverFunctionID AddFunction(Func&& func, int order = 0)
		{
			ObserverFunctionID id = GenerateID();

			for (size_t idx = 0; idx < m_Records.size(); idx++)
			{
				const ItemRecord& record = m_Records[idx];
				if (order < record.m_Order)
				{
					m_Records.insert(m_Records.begin() + idx, { FunctionType(func), id , order });
					return id;
				}
			}

			m_Records.push_back({ FunctionType(func), id , order });

			return id;
		}

		bool RemoveFunction(ObserverFunctionID id)
		{
			for (size_t i = 0; i < m_Records.size(); i++)
			{
				ItemRecord& item = m_Records[i];
				if (item.m_ID == id)
				{
					m_Records.erase(m_Records.begin() + i);
					return true;
				}
			}

			return false;
		}

		void Invoke(Args&&... args)
		{
			for (ItemRecord& record : m_Records)
			{
				record.m_Function(std::forward<Args>(args)...);
			}
		}

		inline bool Empty() const noexcept
		{
			return m_Records.empty();
		}

	private:
		ecsVector<ItemRecord> m_Records{};
		size_t m_IDGenerator = 0;

	private:
		ObserverFunctionID GenerateID()
		{
			ObserverFunctionID newID{};
			newID.m_Value = m_IDGenerator;

			m_IDGenerator++;

			return newID;
		}
	};

}
