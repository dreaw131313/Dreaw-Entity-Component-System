#pragma once
#include "Core.h"

namespace decs
{
	class ArchetypesShrinkToFitConfig
	{
	public:
		size_t m_MaxArchetypeCountToCheck = 10;
		size_t m_MaxArchetypesToShrink = 5;
		/// <summary>
		/// If archetype load factor is less or equal than this value then archetype will be shrinked
		/// </summary>
		float m_MinArchetypeLoadFactor = 0.5f;
	};

	class ArchetypesShrinkToFitState
	{
	public:
		size_t m_LastArchetypeIndex = 0;
	};

	struct ArchetypeDestroyState
	{
	public:
		size_t m_LastCheckdArchetypeIndex = 0;
	};

	struct ArchetypeDestroyConfig
	{
	public:
		size_t m_MaxArchetypesToCheck = 10;
		size_t m_MaxArchetypesDestroy = 2;
		bool m_bDestroyOnlyArchetypesWithFilters = true;
	};

}