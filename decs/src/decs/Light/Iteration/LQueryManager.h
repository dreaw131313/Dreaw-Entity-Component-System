#pragma once

#include "decs/Light/Archetypes/LArchetype.h"

namespace decs::light
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

	private:
		QueryManager* m_ParentManager = nullptr;
		size_t m_IndexInParentManager = std::numeric_limits<size_t>::max();

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

	class QueryManager final
	{
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
		ecsVector<IQuery*> m_Queries{};

		ecsHashMap<IMultiQuery*, uint64_t> m_MultiQueryIndices{};
		ecsVector<IMultiQuery*> m_MultiQueries{};

		bool m_bAcceptQueries = true;

	};

}
