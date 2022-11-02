#pragma once
#include "decspch.h"
#include "Core.h"

namespace decs
{
	template<typename T = void, typename... Args>
	void findIds(std::vector<TypeID>& ids)
	{
		ids.push_back(Type<T>::ID());
		if (sizeof ... (Args) == 0)
			return;
		else
			findIds<Args...>(ids);
	}

	template<typename T = void, typename... Args>
	void findIds(TypeID ids[], uint64_t index)
	{
		ids[index] = Type<T>::ID();
		if (sizeof ... (Args) == 0)
			return;
		else
			findIds<Args...>(ids, index + 1);
	}

	template<typename... Args>
	class TypeGroup
	{
	public:
		TypeGroup()
		{
			findIds<Args...>(m_TypesIDs, 0);
		}

		inline const TypeID* IDs() const { return m_TypesIDs; }
		inline uint64_t Size() const { return m_Size; }

	private:
		TypeID m_TypesIDs[sizeof...(Args)];
		uint64_t m_Size = sizeof...(Args);

	private:
	};
}