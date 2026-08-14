#pragma once

#include "Archetype.h"

namespace decs
{

	class ArchetypeAllocator
	{
	public:
		ArchetypeAllocator(uint32_t chunkSize);

		~ArchetypeAllocator() = default;

		/// <summary>
		/// </summary>
		/// <returns>created archetypes span</returns>
		inline std::span<const Archetype* const> GetCreatedArchetypes() const noexcept
		{
			return m_Created;
		}

		/// <summary>
		/// </summary>
		/// <returns>created archetypes span</returns>
		inline std::span<Archetype* const> GetCreatedArchetypes()
		{
			return m_Created;
		}

		const std::vector<Archetype*>& GetCreatedArchetypesVector() const 
		{
			return m_Created;
		}

		Archetype* CreateArchetype();

		bool Destroy(Archetype* archetype);

		template<typename Func>
		void IterateOverAllArchetypes(Func&& func)
		{
			uint64_t chunksCount = m_Archetypes.ChunkCount();
			for (uint64_t chunkIdx = 0; chunkIdx < chunksCount; chunkIdx++)
			{
				uint64_t chunkSize = m_Archetypes.GetChunkSize(chunkIdx);
				Archetype* chunk = m_Archetypes.GetChunk(chunkIdx);

				for (uint64_t idx = 0; idx < chunkSize; idx++)
				{
					func(chunk[idx]);
				}
			}
		}

	private:
		TChunkedVector<Archetype> m_Archetypes{ 100 };
		ecsVector<Archetype*> m_Created{};
		ecsVector<Archetype*> m_FreeList{};

	private:
		void OnDestroyArchetype(Archetype& archetype);
	};
}
