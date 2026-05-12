#pragma once

#include "decs/Light/Archetypes/LArchetype.h"

namespace decs::light
{
	class QueryManager;

	class ILightQueryImpl
	{
		friend class QueryManager;

	public:
		ILightQueryImpl() = default;

		virtual ~ILightQueryImpl();

		virtual bool TryAddArchetype(const Archetype& archetype) = 0;

		virtual bool TryRemoveArchetpye(const Archetype& archetype) = 0;

	private:
		QueryManager* m_ParentManager = nullptr;
		size_t m_IndexInParentManager = std::numeric_limits<size_t>::max();

	};

	class QueryManager final
	{
	public:
		QueryManager() = default;

		~QueryManager() = default;

		bool AddQuery(ILightQueryImpl* query);

		bool RemoveQuery(ILightQueryImpl* query);

		void OnCreateArchetype(const Archetype* archetype);

		void OnDestroyArchetype(const Archetype* archetype);

	private:
		std::vector<ILightQueryImpl*> m_Queries{};

	};

}
