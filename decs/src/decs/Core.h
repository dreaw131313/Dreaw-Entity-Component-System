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

#include "ThirdParty/skarupke/flat_hash_map.hpp"
#include "ThirdParty/skarupke/bytell_hash_map.hpp"
#include "ThirdParty/skarupke/unordered_map.hpp"

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


	template<typename Key, typename Value>
	//using ecsMap = ska::bytell_hash_map<Key, Value>;
	//using ecsMap = ska::unordered_map<Key, Value>;
	//using ecsMap = ska::flat_hash_map<Key, Value>;
	using ecsMap = std::unordered_map<Key, Value>;

	template<typename Key>
	//using ecsSet = ska::bytell_hash_set<Key>;
	//using ecsSet = std::unordered_set<Key>;
	//using ecsSet = ska::flat_hash_set<Key>;
	using ecsSet = std::unordered_set<Key>;

	using EntityID = uint32_t;
	using EntityVersion = uint32_t;
	using TypeID = uint64_t;


	namespace Limits
	{
		constexpr uint32_t MaxComponentCount = std::numeric_limits<uint32_t>::max();
		constexpr uint64_t MinComponentsInArchetypeToPerformMapLookup = 20;
	}
}