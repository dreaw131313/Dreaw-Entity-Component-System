#pragma once
#include "decspch.h"
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

		template<typename T>
		class Type
		{
		public:
			static constexpr TypeID ID()
			{
				return Type_Base<T, TypeID>::ID();
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
	}
}