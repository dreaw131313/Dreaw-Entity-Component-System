#pragma once

#include <stdint.h>

namespace decs::hash
{

	inline uint64_t Mix(uint64_t hash1, uint64_t hash2)
	{
		return hash1 ^ (hash2 + 0x9e3779b97f4a7c15ULL + (hash1 << 6) + (hash1 >> 2));
	}

	// FNV-1a 32bit hashing algorithm.
	inline constexpr uint32_t fnv1a_32(char const* s, uint64_t count)
	{
		return ((count ? fnv1a_32(s, count - 1) : 2166136261u) ^ s[count]) * 16777619u;
	}

	inline constexpr uint64_t fnv1a_64(char const* s, uint64_t count)
	{
		return ((count ? fnv1a_64(s, count - 1) : 14695981039346656037u) ^ s[count]) * 1099511628211u;
	}

	inline constexpr uint64_t c_string_length(char const* s)
	{
		uint64_t i = 0;
		if (s != nullptr)
			while (s[i] != '\0')
				i += 1;
		return i;
	}

	inline constexpr uint32_t c_string_hash_32(char const* s)
	{
		return fnv1a_32(s, c_string_length(s));
	}

	inline constexpr uint64_t c_string_hash_64(char const* s)
	{
		return fnv1a_64(s, c_string_length(s));
	}

	template<typename T>
	inline constexpr size_t GetHash(const T& t)
	{
		return std::hash<std::decay_t<T>>{}(t);
	}

	template<typename FirstType, typename... Args>
	inline constexpr size_t Combine(FirstType&& first, Args&&... args)
	{
		size_t hash = ::decs::hash::GetHash(first);

		if constexpr (sizeof...(Args) > 0)
		{
			((hash = ::decs::hash::Mix(hash, ::decs::hash::GetHash<Args>(args))), ...);
		}

		return hash;
	}

}