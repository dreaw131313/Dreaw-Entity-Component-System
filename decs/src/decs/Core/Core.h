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
#include <span>
#include <atomic>

#ifdef DECS_DEBUG
#include <cassert>
#endif

#define USE_CONSTEXPR_TYPE_ID
#ifdef USE_CONSTEXPR_TYPE_ID
#define TYPE_ID_CONSTEXPR constexpr
#else
#define TYPE_ID_CONSTEXPR
#endif

#ifdef DECS_DEBUG
#define DECS_ASSERT(condition, message) assert((condition) && message)
#else
#define DECS_ASSERT(condition, message)
#endif // DECS_DEBUG

namespace decs
{
	template<typename Key, typename Value>
	using ecsMap = std::unordered_map<Key, Value>;

	template<typename Key>
	using ecsSet = std::unordered_set<Key>;

	using EntityID = uint32_t;
	using EntityVersion = uint32_t;
	using CombinedEntityID = uint64_t;
	using TypeID = uint64_t;
	inline constexpr TypeID InvalidTypeID = std::numeric_limits<TypeID>::min();

	namespace Limits
	{
		inline constexpr uint64_t MinComponentsInArchetypeToPerformMapLookup = 20;
		inline constexpr EntityVersion MaxVersion = std::numeric_limits<uint32_t>::max();
	}


	class NonCopyable
	{
	public:
		NonCopyable() = default;

		NonCopyable(const NonCopyable&) = delete;
		NonCopyable& operator=(const NonCopyable&) = delete;
	};

	class NonMoveable
	{
	public:
		NonMoveable() = default;

		NonMoveable(NonMoveable&&) noexcept = delete;
		NonMoveable& operator=(NonMoveable&&) noexcept = delete;
	};

	class NonCopyableNonMoveable :
		public NonCopyable,
		public NonMoveable
	{
	public:
		NonCopyableNonMoveable() = default;
	};
}