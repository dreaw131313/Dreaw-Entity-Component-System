#include "LQueryManager.h"


namespace decs::light
{
	bool QueryManager::AddQuery(IQuery* query)
	{
		DECS_ASSERT(query!= nullptr && query->m_ParentManager == nullptr, "Query must not be nullptr!");

		query->m_ParentManager = this;
		query->m_IndexInParentManager = m_Queries.size();

		m_Queries.push_back(query);
		query->OnAddToManager();

		return true;
	}

	bool QueryManager::RemoveQuery(IQuery* query)
	{
		DECS_ASSERT(query != nullptr && query->m_ParentManager == this && query->m_IndexInParentManager < m_Queries.size(), "Query invalid!");

		size_t index = query->m_IndexInParentManager;
		if (index < (m_Queries.size() - 1))
		{
			auto lastQuery = m_Queries.back();
			lastQuery->m_IndexInParentManager = index;
			m_Queries[index] = lastQuery;
		}
		m_Queries.pop_back();

		query->m_ParentManager = nullptr;
		query->m_IndexInParentManager = std::numeric_limits<size_t>::max();
		query->OnRemoveFromManager();

		return true;
	}

	void QueryManager::OnCreateArchetype(const Archetype* archetype)
	{
		if (archetype == nullptr)
		{
			return;
		}

		for (auto query : m_Queries)
		{
			query->TryAddArchetype(*archetype);
		}
	}

	void QueryManager::OnDestroyArchetype(const Archetype* archetype)
	{
		if (archetype == nullptr)
		{
			return;
		}

		for (auto query : m_Queries)
		{
			query->TryRemoveArchetpye(*archetype);
		}
	}
}
