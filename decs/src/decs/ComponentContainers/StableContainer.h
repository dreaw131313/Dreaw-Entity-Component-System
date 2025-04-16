#pragma once
#include "decs/Core.h"
#include "decs/Containers/TChunkedVector.h"
#include "decs/Type.h"

#include "decs/Component/Component.h"


namespace decs
{
	class ChunkBase
	{
	public:
		virtual uint32_t GetChunkIndex() const = 0;
	};

	template<typename TComponentType>
	class TChunk final : public ChunkBase
	{
		struct RecordData final
		{
		public:
			union
			{
				TComponentType Component;
			};
			bool bIsAllocated = false;

		public:
			RecordData():
				bIsAllocated(false)
			{
			}

			~RecordData()
			{
				if (bIsAllocated)
				{
					Component.~TComponentType();
				}
			}
		};

	public:
		uint32_t m_Index = 0;
		uint32_t m_IndexInFreeSpaces = 0;
		bool m_IsInFreeSpaces = 0;

	public:
		TChunk(uint32_t capacity):
			m_Capacity(capacity > 0 ? capacity : 100)
		{
			//m_Data = (RecordData*)::operator new(capacity * sizeof(RecordData));

			m_Data = new RecordData[m_Capacity];
		}

		~TChunk()
		{
			delete[] m_Data;
		}

		virtual uint32_t GetChunkIndex() const override
		{
			return m_Index;
		}

		bool IsEmpty() const
		{
			return m_Size == 0;
		}

		bool IsFull() const
		{
			return m_Capacity == m_Size;
		}

		TComponentType& operator[](uint32_t index) const
		{
			return m_Data[index];
		}

		bool IsAllocatedAt(uint32_t index) const
		{
			return m_Data[index].bIsAllocated;
		}

		template<typename... Args>
		TComponentType* Emplace(Args&&... args)
		{
			if (IsFull()) return nullptr;

			m_Size += 1;

			if (m_FreeSpaces.size() > 0)
			{
				uint32_t freeSpaceIndex = m_FreeSpaces.back();
				m_FreeSpaces.pop_back();

				RecordData& record = m_Data[freeSpaceIndex];
				record.bIsAllocated = true;
				TComponentType* data = new(&record.Component)TComponentType(std::forward<Args>(args)...);

				ComponentBase* componentBase = data;
				componentBase->SetChunkAndIndex(this, freeSpaceIndex);

				return data;
			}

			{

				uint32_t allocationIndex = m_CurrentAllocationOffset;
				RecordData& record = m_Data[m_CurrentAllocationOffset];
				record.bIsAllocated = true;
				TComponentType* data = new(&record.Component)TComponentType(std::forward<Args>(args)...);

				ComponentBase* componentBase = data;
				componentBase->SetChunkAndIndex(this, m_CurrentAllocationOffset);

				m_CurrentAllocationOffset += 1;

				return data;
			}
		}

		bool Remove(TComponentType* component)
		{
			if (component == nullptr || component->GetParentChunk() != this)
			{
				return true;
			}

			uint32_t index = component->GetIndexInChunk();

			if (index < m_Capacity)
			{
				RecordData& record = m_Data[index];
				if (record.bIsAllocated && component == &record.Component)
				{
					m_Size -= 1;
					if (index == (m_CurrentAllocationOffset - 1))
					{
						m_CurrentAllocationOffset -= 1;
					}
					else
					{
						m_FreeSpaces.push_back(index);
					}

					if (IsEmpty())
					{
						m_FreeSpaces.clear();
						m_CurrentAllocationOffset = 0;
					}

					record.bIsAllocated = false;
					record.Component.~TComponentType();
					return true;
				}
			}
			return false;

		}

	private:
		std::vector<uint32_t> m_FreeSpaces;

		RecordData* m_Data = nullptr;
		uint32_t m_Capacity = 0;

		uint32_t m_CurrentAllocationOffset = 0;
		uint32_t m_Size = 0;
	};

	class StableContainerBase
	{
	public:
		virtual ~StableContainerBase()
		{

		}

		inline virtual TypeID GetTypeID()const noexcept = 0;
		virtual StableContainerBase* Clone(uint32_t withChunkSize) = 0;

		virtual bool Remove(ComponentBase* component) = 0;
		virtual ComponentBase* EmplaceFromBaseComponent(ComponentBase* ptr) = 0;
		virtual uint32_t GetChunkSize() const noexcept = 0;
		virtual void Clear() = 0;
	};

	template<typename TComponentType>
	class StableContainer : public StableContainerBase
	{
		using ChunkType = TChunk<TComponentType>;
	private:
		NON_COPYABLE(StableContainer);
		NON_MOVEABLE(StableContainer);

	public:
		StableContainer()
		{

		}

		StableContainer(uint32_t chunkCapacity):
			m_ChunkCapacity(chunkCapacity)
		{
		}

		~StableContainer()
		{
			for (auto& chunk : m_Chunks)
			{
				if (chunk != nullptr)
				{
					delete chunk;
				}
			}
		}

		virtual TypeID GetTypeID()const noexcept override { return Type<TComponentType>::ID(); }

		virtual StableContainerBase* Clone(uint32_t withChunkSize) override
		{
			return new StableContainer<TComponentType>(withChunkSize);
		}

		virtual uint32_t GetChunkSize() const noexcept override
		{
			return m_ChunkCapacity;
		}

		template<typename... Args>
		TComponentType* Emplace(Args&&... args)
		{
			ChunkType* chunk = GetCurrentChunk();
			auto result = chunk->Emplace(std::forward<Args>(args)...);

			if (chunk->IsFull())
			{
				RemoveChunkFromFreeSpaces(chunk);
			}

			return result;
		}

		bool Remove(ComponentBase* componentBase) override
		{
			TComponentType* component = static_cast<TComponentType*>(componentBase);
			uint32_t chunkIndex = componentBase->GetParentChunk()->GetChunkIndex();
			if (chunkIndex < m_Chunks.size())
			{
				ChunkType* chunk = m_Chunks[chunkIndex];
				bool wasChunkFull = chunk->IsFull();
				if (chunk->Remove(component))
				{
					if (chunk->IsEmpty())
					{
						RemoveChunk(chunk);
					}
					else
					{
						AddChunkToFreeSpaces(chunk);
					}
					return true;
				}
			}
			return false;
		}

		bool Remove(TComponentType* component)
		{
			ComponentBase* componentBase = component;

			uint32_t chunkIndex = componentBase->GetParentChunk()->GetChunkIndex();
			if (chunkIndex < m_Chunks.size())
			{
				ChunkType* chunk = m_Chunks[chunkIndex];
				bool wasChunkFull = chunk->IsFull();
				if (chunk->Remove(component))
				{
					if (chunk->IsEmpty())
					{
						RemoveChunk(chunk);
					}
					else
					{
						AddChunkToFreeSpaces(chunk);
					}
					return true;
				}
			}
			return false;
		}

		virtual ComponentBase* EmplaceFromBaseComponent(ComponentBase* ptr)override
		{
			return Emplace(*static_cast<TComponentType*>(ptr));
		}

		virtual void Clear() override
		{
			for (auto& chunk : m_Chunks)
			{
				delete chunk;
			}
			m_CurrentChunk = nullptr;
			m_Chunks.clear();
			m_ChunksWithFreeSpace.clear();
		}

	private:
		std::vector<ChunkType*> m_Chunks;
		std::vector<ChunkType*> m_ChunksWithFreeSpace;
		ChunkType* m_CurrentChunk = nullptr;
		uint32_t m_ChunkCapacity = 1000;

	private:
		inline ChunkType* GetCurrentChunk()
		{
			if (m_CurrentChunk == nullptr || m_CurrentChunk->IsFull())
			{
				if (m_ChunksWithFreeSpace.size() != 0)
				{
					m_CurrentChunk = m_ChunksWithFreeSpace[0];
				}
				else
				{
					m_CurrentChunk = new ChunkType(m_ChunkCapacity);
					m_CurrentChunk->m_IsInFreeSpaces = true;
					m_CurrentChunk->m_IndexInFreeSpaces = static_cast<uint32_t>(m_ChunksWithFreeSpace.size());
					m_CurrentChunk->m_Index = static_cast<uint32_t>(m_Chunks.size());

					m_Chunks.push_back(m_CurrentChunk);
					m_ChunksWithFreeSpace.push_back(m_CurrentChunk);
				}
			}

			return m_CurrentChunk;
		}

		void RemoveChunk(ChunkType* chunk)
		{
			RemoveChunkFromFreeSpaces(chunk);

			if (chunk != m_Chunks.back())
			{
				auto lastChunk = m_Chunks.back();
				m_Chunks[chunk->m_Index] = lastChunk;
				lastChunk->m_Index = chunk->m_Index;
			}
			m_Chunks.pop_back();

			if (chunk == m_CurrentChunk)
			{
				m_CurrentChunk = nullptr;
			}

			delete chunk;
		}

		bool RemoveChunkFromFreeSpaces(ChunkType* chunk)
		{
			if (!chunk->m_IsInFreeSpaces) return false;

			if (m_ChunksWithFreeSpace.back() != chunk)
			{
				m_ChunksWithFreeSpace.back()->m_IndexInFreeSpaces = chunk->m_IndexInFreeSpaces;
				m_ChunksWithFreeSpace[chunk->m_IndexInFreeSpaces] = m_ChunksWithFreeSpace.back();
			}
			m_ChunksWithFreeSpace.pop_back();
			chunk->m_IsInFreeSpaces = false;

			return true;
		}

		bool AddChunkToFreeSpaces(ChunkType* chunk)
		{
			if (chunk->m_IsInFreeSpaces || chunk->IsFull()) return false;

			chunk->m_IsInFreeSpaces = true;
			chunk->m_IndexInFreeSpaces = static_cast<uint32_t>(m_ChunksWithFreeSpace.size());
			m_ChunksWithFreeSpace.push_back(chunk);

			return true;
		}
	};
}