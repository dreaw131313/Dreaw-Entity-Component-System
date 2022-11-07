#pragma once 
#include "decspch.h"

namespace decs
{
	namespace MemorySize
	{
		constexpr uint64_t KiloByte = 1024;
		constexpr uint64_t MegaByte = 1048576;
		constexpr uint64_t GigaByte = 1073741824;
	}

	using EntityID = uint64_t;
	namespace Null
	{
		constexpr EntityID ID = std::numeric_limits<EntityID>::max();
	}
	using TypeID = uint32_t;

	template<typename Key, typename Value>
	//using ecsMap = std::unordered_map<Key, Value>;
	//using ecsMap = ska::flat_hash_map<Key, Value>;
	using ecsMap = ska::bytell_hash_map<Key, Value>;
	//using ecsMap = ska::unordered_map<Key, Value>;

	template<typename Key>
	using ecsSet = std::unordered_set<Key>;

#if defined _MSC_VER
#   define FULL_FUNCTION_NAME __FUNCSIG__
#elif defined __clang__ || (defined __GNUC__)
#   define FULL_FUNCTION_NAME __PRETTY_FUNCTION__
#endif

	// FNV-1a 32bit hashing algorithm.
	constexpr TypeID fnv1a_32(char const* s, uint64_t count)
	{
		return ((count ? fnv1a_32(s, count - 1) : 2166136261u) ^ s[count]) * 16777619u;
	}

	constexpr uint64_t c_string_lenght(char const* s)
	{
		uint64_t i = 0;
		if (s != nullptr)
			while (s[i] != '\0')
				i += 1;
		return i;
	}

	constexpr TypeID c_string_hash(char const* s)
	{
		return fnv1a_32(s, c_string_lenght(s));
	}

	// Forward declaration:
	class EntityData;
	class Archetype;
	class ArchetypeEdge;
	class ComponentContainer;
	class ContainerSettings;
	class Container;
	class Entity;
}