#include "LQueryManager.h"

#include "decs/Light/LContainer.h"

namespace decs::light
{
	QueryManager::QueryManager(Container* parentContainer):
		m_ParentContainer(parentContainer)
	{

	}

	QueryManager::~QueryManager()
	{
	}

	void QueryManager::OnDestroyContainer()
	{
		for (auto query : m_Queries)
		{
			query->OnQueryManagerDestroy();
		}

		for (auto multiQuery : m_MultiQueries)
		{
			multiQuery->OnDestroyContainer(m_ParentContainer);
		}

		m_Queries.clear();
		m_MultiQueries.clear();
		m_MultiQueryIndices.clear();

		m_bAcceptQueries = false;
	}

	bool QueryManager::AddQuery(IQuery* query)
	{
		if (!m_bAcceptQueries)
		{
			return false;
		}

		DECS_ASSERT(query != nullptr && query->m_ParentManager == nullptr, "Query must not be nullptr!");

		query->m_ParentManager = this;
		query->m_IndexInParentManager = m_Queries.size();

		m_Queries.push_back(query);
		query->OnAddToManager();

		return true;
	}

	bool QueryManager::RemoveQuery(IQuery* query)
	{
		if (!m_bAcceptQueries)
		{
			return false;
		}

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

	bool QueryManager::AddMultiQuery(IMultiQuery* query)
	{
		if (!m_bAcceptQueries)
		{
			return false;
		}

		DECS_ASSERT(query != nullptr && !m_MultiQueryIndices.contains(query), "Query must not be nullptr and must not be added to this query!");

		m_MultiQueryIndices[query] = m_MultiQueryIndices.size();
		m_MultiQueries.push_back(query);

		return true;
	}

	bool QueryManager::RemoveMultiQuery(IMultiQuery* query)
	{
		if (!m_bAcceptQueries)
		{
			return false;
		}
		DECS_ASSERT(query != nullptr && m_MultiQueryIndices.contains(query), "Query must not be nullptr!");

		auto it = m_MultiQueryIndices.find(query);
		if (it == m_MultiQueryIndices.end())
		{
			return false;
		}

		size_t index = it->second;
		m_MultiQueryIndices.erase(query);

		if (index < (m_MultiQueries.size() - 1))
		{
			auto lastMultiQuery = m_MultiQueries.back();
			m_MultiQueryIndices[lastMultiQuery] = index;
			m_MultiQueries[index] = lastMultiQuery;
		}
		m_MultiQueries.pop_back();

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

		for (auto multiQuery : m_MultiQueries)
		{
			multiQuery->TryAddArchetype(*m_ParentContainer, *archetype);
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

		for (auto multiQuery : m_MultiQueries)
		{
			multiQuery->TryRemoveArchetpye(*m_ParentContainer, *archetype);
		}
	}
}
