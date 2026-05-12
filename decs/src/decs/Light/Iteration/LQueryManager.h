#pragma once

#include "decs/Light/Archetypes/LArchetype.h"

namespace decs::light
{
	class QueryManager;

	class IQuery
	{
		friend class QueryManager;

	public:
		IQuery() = default;

		virtual ~IQuery() = default;

	private:
		QueryManager* m_ParentManager = nullptr;
		size_t m_IndexInParentManager = std::numeric_limits<size_t>::max();

	protected:
		virtual void TryAddArchetype(const Archetype& archetype) = 0;

		virtual void TryRemoveArchetpye(const Archetype& archetype) = 0;

		virtual void OnAddToManager() = 0;

		virtual void OnRemoveFromManager() = 0;

		/// <summary>
		/// Called when query manager destructor is invoked and query manager has queries.
		/// </summary>
		virtual void OnQueryManagerDestroy() = 0;

	};

	class QueryManager final
	{
	public:
		QueryManager() = default;

		~QueryManager();

		bool AddQuery(IQuery* query);

		bool RemoveQuery(IQuery* query);

		void OnCreateArchetype(const Archetype* archetype);

		void OnDestroyArchetype(const Archetype* archetype);

	private:
		std::vector<IQuery*> m_Queries{};

	};

}
