#pragma once 
#include <stdint.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <string>
#include <typeinfo>

#include "traits.h"

namespace decs
{
#if defined _MSC_VER
#   define FULL_FUNCTION_NAME __FUNCSIG__
#elif defined __clang__ || (defined __GNUC__)
#   define FULL_FUNCTION_NAME __PRETTY_FUNCTION__
#endif

#define USE_CONSTEXPR_TYPE_ID
#ifdef USE_CONSTEXPR_TYPE_ID
#define TYPE_ID_CONSTEXPR constexpr
#else
#define TYPE_ID_CONSTEXPR
#endif


#define NON_COPYABLE(className)						\
className(const className&) = delete;				\
className& operator=(const className&) = delete;	\

#define NON_MOVEABLE(className)				\
className(className&&) = delete;			\
className& operator=(className&&) = delete;	\

	template<typename Key, typename Value>
	using ecsMap = std::unordered_map<Key, Value>;

	template<typename Key>
	using ecsSet = std::unordered_set<Key>;

	using EntityID = uint32_t;
	using EntityVersion = uint32_t;
	using TypeID = uint64_t;

	namespace Limits
	{
		inline constexpr uint32_t MaxComponentCount = std::numeric_limits<uint32_t>::max();
		inline constexpr uint64_t MinComponentsInArchetypeToPerformMapLookup = 20;
		inline constexpr EntityVersion MaxVersion = std::numeric_limits<uint32_t>::max();
	}
}