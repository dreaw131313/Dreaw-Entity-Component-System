#pragma once
#include "Core.h"

#include <source_location>
#include "Hash.h"

namespace decs
{
	template<typename T, typename TypeIDintType>
	class Type_Base;

	template<typename T>
	class Type_Base<T, uint32_t>
	{
	public:
		static constexpr TypeID ID()
		{
			constexpr auto id = hash::c_string_hash_32(std::source_location::current().function_name());
			return id;
		}
	};

	template<typename T>
	class Type_Base<T, uint64_t>
	{
	public:
		static constexpr TypeID ID()
		{
			constexpr auto id = hash::c_string_hash_64(std::source_location::current().function_name());
			return id;
		}

	};

	template<typename T>
	class Type
	{
	public:

	#ifdef USE_CONSTEXPR_TYPE_ID
		inline static consteval TypeID ID()
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
		constexpr static std::string FindName()
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

		constexpr static std::string FindNameWithoutNamespace()
		{
			std::string nameWithNamespaces = Name();
			int index = static_cast<int>(nameWithNamespaces.size());
			while (index >= 0)
			{
				index -= 1;
				if (nameWithNamespaces[index] == ':')
				{
					break;
				}
			}
			index += 1;
			return std::string(nameWithNamespaces.begin() + index, nameWithNamespaces.end());
		}
	};

#ifndef USE_CONSTEXPR_TYPE_ID
	template<typename TComponent>
	const std::type_info* Type<TComponent>::m_TypeInfo = &typeid(TComponent);
#endif


	template<typename... Args>
	class TypeGroup
	{
	public:
		constexpr TypeGroup() = default;

		constexpr TypeGroup(const TypeGroup&) = default;
		constexpr TypeGroup(TypeGroup&&)noexcept = default;

		constexpr TypeGroup& operator=(const TypeGroup&) = default;
		constexpr TypeGroup& operator=(TypeGroup&&) noexcept = default;

		constexpr TypeID operator[](const uint64_t index) const
		{
			return s_TypesIDs[index];
		}

		constexpr uint64_t Size() const
		{
			return sizeof...(Args);
		}

	private:
		inline static constexpr const TypeID s_TypesIDs[sizeof...(Args)] = { Type<Args>::ID()... };
	};

	template<>
	class TypeGroup<>
	{
	public:
		constexpr TypeID operator[](const uint64_t index) const
		{
			return InvalidTypeID;
		}

		constexpr uint64_t Size() const
		{
			return 0;
		}
	};
}
