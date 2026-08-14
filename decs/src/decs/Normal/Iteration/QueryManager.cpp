#include "QueryManager.h"

#include "decs/Normal/Container.h"

namespace decs
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
		for (auto query : m_Queries.GetQueries())
		{
			query->OnQueryManagerDestroy();
		}

		for (auto multiQuery : m_MultiQueries.GetQueries())
		{
			multiQuery->OnDestroyContainer(m_ParentContainer);
		}

		m_Queries.Clear();
		m_MultiQueries.Clear();

		m_bAcceptQueries = false;
	}

	bool QueryManager::AddQuery(IQuery* query)
	{
		if (!m_bAcceptQueries)
		{
			return false;
		}

		return m_Queries.AddQuery(query);
	}

	bool QueryManager::RemoveQuery(IQuery* query)
	{
		if (!m_bAcceptQueries)
		{
			return false;
		}

		return m_Queries.RemoveQuery(query);
	}

	bool QueryManager::AddMultiQuery(IMultiQuery* query)
	{
		if (!m_bAcceptQueries)
		{
			return false;
		}

		return m_MultiQueries.AddQuery(query);
	}

	bool QueryManager::RemoveMultiQuery(IMultiQuery* query)
	{
		if (!m_bAcceptQueries)
		{
			return false;
		}

		return m_MultiQueries.RemoveQuery(query);
	}

	void QueryManager::OnCreateArchetype(const Archetype* archetype)
	{
		if (archetype == nullptr)
		{
			return;
		}

		for (auto query : m_Queries.GetQueries())
		{
			query->TryAddArchetype(*archetype);
		}

		for (auto multiQuery : m_MultiQueries.GetQueries())
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

		for (auto query : m_Queries.GetQueries())
		{
			query->TryRemoveArchetpye(*archetype);
		}

		for (auto multiQuery : m_MultiQueries.GetQueries())
		{
			multiQuery->TryRemoveArchetpye(*m_ParentContainer, *archetype);
		}
	}
}