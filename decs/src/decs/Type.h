#pragma once
#include "Core.h"

namespace decs
{
	// FNV-1a 32bit hashing algorithm.
	constexpr TypeID fnv1a_32(char const* s, uint64_t count)
	{
		return ((count ? fnv1a_32(s, count - 1) : 2166136261u) ^ s[count]) * 16777619u;
	}

	constexpr TypeID fnv1a_64(char const* s, uint64_t count)
	{
		return ((count ? fnv1a_64(s, count - 1) : 14695981039346656037u) ^ s[count]) * 1099511628211u;
	}

	constexpr uint64_t c_string_length(char const* s)
	{
		uint64_t i = 0;
		if (s != nullptr)
			while (s[i] != '\0')
				i += 1;
		return i;
	}

	constexpr TypeID c_string_hash_32(char const* s)
	{
		return fnv1a_32(s, c_string_length(s));
	}

	constexpr TypeID c_string_hash_64(char const* s)
	{
		return fnv1a_64(s, c_string_length(s));
	}

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

#ifdef USE_CONSTEXPR_TYPE_ID
		inline static constexpr TypeID ID()
		{
			return Type_Base<T, TypeID>::ID();
		}
#else
		static TypeID ID()
		{
			return reinterpret_cast<TypeID>(m_TypeInfo);
		}

	private:
		static const std::type_info* m_TypeInfo;
#endif

		inline static constexpr std::string Name()
		{
			static std::string className = FindName();
			return className;
		}

	private:
		static std::string FindName()
		{
			auto erase = [](std::string& from, const std::string& erasedString, uint32_t aditionalOffset)
			{
				if (erasedString.empty())
				{
					return;
				}

				uint64_t index = from.find(erasedString);
				while (index != std::string::npos)
				{
					from.erase(index + aditionalOffset, erasedString.size() - aditionalOffset);
					index = from.find(erasedString);
				}
			};

			constexpr const char* class1 = "<class ";
			constexpr const char* class2 = ",class ";
			constexpr const char* struct1 = "<struct ";
			constexpr const char* struct2 = ",struct ";

			std::string funcsig = __FUNCTION__;

			erase(funcsig, class1, 1);
			erase(funcsig, class2, 1);
			erase(funcsig, struct1, 1);
			erase(funcsig, struct2, 1);
			erase(funcsig, " ", 0);

			funcsig.erase(funcsig.begin(), funcsig.begin() + 11);
			funcsig.erase(funcsig.end() - 11, funcsig.end());

			return funcsig;
		}
	};

#ifndef USE_CONSTEXPR_TYPE_ID
	template<typename T>
	const std::type_info* Type<T>::m_TypeInfo = &typeid(T);
#endif

	template<typename T = void, typename... Args>
	void findIdsInVector(std::vector<TypeID>& ids)
	{
		ids.push_back(Type<T>::ID());
		if (sizeof ... (Args) == 0)
			return;
		else
			findIds<Args...>(ids);
	}

	template<typename... Args>
	void findIds(std::vector<TypeID>& ids)
	{
		constexpr uint64_t typeCount = sizeof...(Args);
		if (ids.capacity() != typeCount)
		{
			ids.reserve(typeCount);
		}
		findIdsInVector<Args...>(ids);
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
	};
}