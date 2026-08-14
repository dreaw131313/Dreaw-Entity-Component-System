#pragma once

#include "decs/Normal/Archetypes/Archetype.h"

namespace decs
{
	class QueryManager;
	class Container;

	class IQuery
	{
		friend class QueryManager;

	public:
		IQuery() = default;

		virtual ~IQuery() = default;

		IQuery(const IQuery&)
		{

		}

		IQuery& operator=(const IQuery&)
		{
			return *this;
		}

		IQuery(IQuery&& other) noexcept
		{

		}

		IQuery& operator=(IQuery&&) noexcept
		{
			return *this;
		}

	protected:
		virtual void TryAddArchetype(const Archetype& archetype) = 0;

		virtual void TryRemoveArchetpye(const Archetype& archetype) = 0;

		/// <summary>
		/// Called when query manager destructor is invoked and query manager has queries.
		/// </summary>
		virtual void OnQueryManagerDestroy() = 0;

	};

	class IMultiQuery
	{
		friend class QueryManager;
	public:
		IMultiQuery() = default;

		virtual ~IMultiQuery() = default;

		IMultiQuery(const IMultiQuery&)
		{

		}

		IMultiQuery& operator=(const IMultiQuery&)
		{
			return *this;
		}

		IMultiQuery(IMultiQuery&& other) noexcept
		{

		}

		IMultiQuery& operator=(IMultiQuery&&) noexcept
		{
			return *this;
		}

		virtual bool AddContainer(Container* container, bool bIsEnabled = true) = 0;
		virtual bool RemoveContainer(Container* container) = 0;
		virtual void SetContainerEnabled(Container* container, bool isEnabled) = 0;

	protected:
		virtual void TryAddArchetype(Container& container, const Archetype& archetype) = 0;

		virtual void TryRemoveArchetpye(Container& container, const Archetype& archetype) = 0;

		virtual void OnDestroyContainer(Container* container) = 0;

	};

	class QueryManager
	{
		template<typename QueryType>
		struct QueryContainer
		{
		public:
			QueryContainer() = default;

			~QueryContainer()
			{

			}

			std::span<const QueryType*> GetQueries() const noexcept
			{
				return m_Queries;
			}

			std::span<QueryType*> GetQueries() noexcept
			{
				return m_Queries;
			}

			bool AddQuery(QueryType* query)
			{
				if (query == nullptr)
				{
					return false;
				}

				auto it = m_QueriesMap.find(query);
				if (it != m_QueriesMap.end())
				{
					return false;
				}

				m_QueriesMap[query] = m_Queries.size();
				m_Queries.push_back(query);

				return true;
			}

			bool RemoveQuery(QueryType* query)
			{
				if (query == nullptr)
				{
					return false;
				}

				auto it = m_QueriesMap.find(query);
				if (it == m_QueriesMap.end())
				{
					return false;
				}

				const size_t index = it->second;
				if (index < (m_Queries.size() - 1))
				{
					auto lastQuery = m_Queries.back();
					m_Queries[index] = lastQuery;
					m_QueriesMap[lastQuery] = index;
				}
				m_Queries.pop_back();
				m_QueriesMap.erase(it);

				return false;
			}

			void Clear()
			{
				m_QueriesMap.clear();
				m_Queries.clear();
			}

		private:
			ecsHashMap<QueryType*, size_t> m_QueriesMap{};
			ecsVector<QueryType*> m_Queries{};
		};

	public:
		QueryManager(Container* parentContainer);

		~QueryManager();

		void OnDestroyContainer();

		bool AddQuery(IQuery* query);

		bool RemoveQuery(IQuery* query);

		bool AddMultiQuery(IMultiQuery* query);

		bool RemoveMultiQuery(IMultiQuery* query);

		void OnCreateArchetype(const Archetype* archetype);

		void OnDestroyArchetype(const Archetype* archetype);

	private:
		Container* m_ParentContainer = nullptr;
		QueryContainer<IQuery> m_Queries{};
		QueryContainer<IMultiQuery> m_MultiQueries{};
		bool m_bAcceptQueries = true;
	};

}
