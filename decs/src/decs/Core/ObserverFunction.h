#pragma once

#include <functional>
#include <vector>
#include <cstdint>


namespace decs
{
	template<typename...>
	struct TObserverFunction;

	struct ObserverFunctionID final
	{
		template<typename...>
		friend struct TObserverFunction;
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
		};

	public:
		template<typename Func>
		ObserverFunctionID AddFunction(Func&& func)
		{
			ObserverFunctionID id = GenerateID();

			m_Functions.push_back({ FunctionType(func), id });

			return id;
		}

		bool RemoveFunction(ObserverFunctionID id)
		{
			for (size_t i = 0; i < m_Functions.size(); i++)
			{
				ItemRecord& item = m_Functions[i];
				if (item.m_ID == id)
				{
					m_Functions.erase(m_Functions.begin() + i);
					return true;
				}
			}

			return false;
		}

		void Invoke(Args&&... args)
		{
			for (ItemRecord& record : m_Functions)
			{
				record.m_Function(std::forward<Args>(args)...);
			}
		}

	private:
		std::vector<ItemRecord> m_Functions{};
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
