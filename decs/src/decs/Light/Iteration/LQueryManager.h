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

	class IMultiQuery
	{
		friend class QueryManager;
	public:
		virtual ~IMultiQuery()
		{

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
		std::vector<IQuery*> m_Queries{};

		ecsMap<IMultiQuery*, uint64_t> m_MultiQueryIndices{};
		std::vector<IMultiQuery*> m_MultiQueries{};

		bool m_bAcceptQueries = true;

	};

}
