#pragma once
#include "decspch.h"
#include "../Archetype.h"

namespace decs
{
	template<typename... Args>
	using Include = TypeGroup<Args...>;
	template<typename... Args>
	using Exclude = TypeGroup<Args...>;
	template<typename... Args>
	using Requires = TypeGroup<Args...>;

	template<uint64_t TypesCount>
	struct ViewArchetypeContext
	{
	public:
		Archetype* Arch = nullptr;
		uint32_t TypeIndexes[TypesCount]; // indexy kontenerów z archetypu z których ma ko¿ystaæ widok - shit

	public:

		inline uint64_t EntitiesCount() const { return m_EntitiesCount; }

		inline void ValidateEntitiesCount() { m_EntitiesCount = Arch->EntitiesCount(); }
	private:
		uint64_t m_EntitiesCount;
	};
}