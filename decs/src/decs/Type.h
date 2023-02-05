#pragma once
#include "Core.h"

namespace decs
{
	namespace
	{
		template<typename T, typename TypeIDintType>
		class Type_Base;

		template<typename T>
		class Type_Base<T, uint32_t>
		{
		public:
			static constexpr TypeID ID()
			{
				constexpr auto id = c_string_hash_32(FULL_FUNCTION_NAME);
				return id;
			}
		};

		template<typename T>
		class Type_Base<T, uint64_t>
		{
		public:
			static constexpr TypeID ID()
			{
				constexpr auto id = c_string_hash_64(FULL_FUNCTION_NAME);
				return id;
			}
		};
	}
	template<typename T>
	class Type
	{
	public:
		static constexpr TypeID ID()
		{
			return Type_Base<T, TypeID>::ID();
		}

		static std::string StringToHash()
		{
			return FULL_FUNCTION_NAME;
		}

		/*static TypeID ID()
		{
			return reinterpret_cast<TypeID>(m_TypeInfo);
		}

	private:
		static const std::type_info* m_TypeInfo;*/
	};

	//template<typename T>
	//const std::type_info* Type<T>::m_TypeInfo = &typeid(T);


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

		TypeID operator[](const uint64_t index) const { return m_TypesIDs[index]; }

		inline const TypeID* IDs() const { return m_TypesIDs; }
		constexpr uint64_t Size() const { return sizeof...(Args); }

	private:
		TypeID m_TypesIDs[sizeof...(Args)];
		uint64_t m_Size = sizeof...(Args);

	private:
	};
}