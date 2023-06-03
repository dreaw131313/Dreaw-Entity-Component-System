#pragma once
#include <vector>

namespace decs
{
	template<typename T>
	class TChunkedVector
	{
	private:
#pragma region Chunk class
		class Chunk final
		{
		public:
			Chunk()
			{

			}

			~Chunk()
			{

			}

			static void Create(Chunk& chunk, uint64_t capacity = 100)
			{
				if (capacity == 0) chunk.m_Capacity = 1;
				else chunk.m_Capacity = capacity;

				chunk.m_Size = 0;

				chunk.m_Data = (T*) ::operator new(chunk.m_Capacity * sizeof(T));
			}

			static void Destroy(Chunk& chunk)
			{
				for (uint64_t idx = 0; idx < chunk.m_Size; idx++)
				{
					chunk.m_Data[idx].~T();
				}

				uint64_t size = chunk.m_Capacity * sizeof(T);
				::operator delete(chunk.m_Data, size);
			}

			static void Copy(const Chunk& from, Chunk& to)
			{
				uint64_t sourceChunkSize = from.Size();
				for (uint64_t eIdx = 0; eIdx < sourceChunkSize; eIdx++)
				{
					to.EmplaceBack(from.m_Data[eIdx]);
				}
			}
		public:
			inline bool IsFull() const noexcept { return m_Size == m_Capacity; }
			inline bool IsEmpty() const noexcept { return m_Size == 0; }
			inline uint64_t Size() const noexcept { return m_Size; }
			inline uint64_t Capacity() const noexcept { return m_Capacity; }
			inline T* Data() { return m_Data; }

			template<typename... Args>
			T& EmplaceBack(Args&&... args)
			{
				T* value = new(&m_Data[m_Size])T(std::forward<Args>(args)...);
				m_Size += 1;
				return *value;
			}

			bool RemoveSwapBack(uint64_t index)
			{
				uint64_t lastIndex = m_Size - 1;
				if (index == lastIndex)
				{
					m_Data[index].~T();
					m_Size -= 1;
					return true;
				}
				else if (index <= m_Size)
				{
					m_Data[index] = std::move(m_Data[lastIndex]);

					m_Data[lastIndex].~T();

					m_Size -= 1;
					return true;
				}

				return false;
			}

			void PopBack()
			{
				m_Size -= 1;
				m_Data[m_Size].~T();
			}

			inline T& At(uint64_t index) { return m_Data[index]; }

			inline T& operator[](uint64_t index)
			{
				return m_Data[index];
			}

			inline T& operator[](uint64_t index) const
			{
				return m_Data[index];
			}

			T& Back()
			{
				return m_Data[m_Size - 1];
			}

			void Clear()
			{
				for (uint64_t idx = 0; idx < m_Size; idx++)
				{
					m_Data[idx].~T();
				}
				m_Size = 0;
			}

			// OPERATORS:
		public:
			uint64_t m_Capacity = 10;
			uint64_t m_Size = 0;
			T* m_Data = nullptr;
		};

#pragma endregion

	public:
		struct EmplaceBackData final
		{
		public:
			uint64_t ChunkIndex;
			uint64_t ElementIndex;
			T* Data;
		public:
			EmplaceBackData(
				uint64_t chunkIndex,
				uint64_t elementIndex,
				T* data
			) :
				ChunkIndex(chunkIndex),
				ElementIndex(elementIndex),
				Data(data)
			{

			}
			~EmplaceBackData() {}
		};


	public:
		TChunkedVector()
		{
			AddChunk();
		}

		~TChunkedVector()
		{
			for (auto& chunk : m_Chunks)
				Chunk::Destroy(chunk);
		}

		TChunkedVector(uint64_t chunkCapacity) :
			m_ChunkCapacity(chunkCapacity)
		{
			AddChunk();
		}

		TChunkedVector(const TChunkedVector& other)
		{
			m_ChunkCapacity = other.m_ChunkCapacity;
			m_ChunksCount = other.m_ChunksCount;
			m_CreatedElements = other.m_CreatedElements;
			m_Chunks.reserve(m_ChunksCount);

			for (uint64_t i = 0; i < m_ChunksCount; i++)
			{
				const Chunk& sourceChunk = other.m_Chunks[i];
				Chunk newChunk = m_Chunks.emplace_back();
				Chunk::Create(newChunk, m_ChunkCapacity);
				Chunk::Copy(sourceChunk, newChunk);
			}
		}

		TChunkedVector(TChunkedVector&& other) noexcept :
			m_ChunkCapacity(other.m_ChunkCapacity),
			m_ChunksCount(other.m_ChunksCount),
			m_CreatedElements(other.m_CreatedElements)
		{
			m_Chunks.reserve(m_ChunksCount);

			m_Chunks = std::move(other.m_Chunks);

			other.m_Chunks.clear();
			other.m_ChunkCapacity = 0;
			other.m_ChunksCount = 0;
			other.m_CreatedElements = 0;
		}

		TChunkedVector& operator=(const TChunkedVector& other)
		{
			return *this = TChunkedVector(other);
		}

		TChunkedVector& operator=(TChunkedVector&& other)
		{
			m_ChunkCapacity = other.m_ChunkCapacity;
			m_ChunksCount = other.m_ChunksCount;
			m_CreatedElements = other.m_CreatedElements;
			m_Chunks.reserve(m_ChunksCount);

			for (auto& chunk : m_Chunks)
				Chunk::Destroy(chunk);

			m_Chunks.clear();
			m_Chunks = std::move(other.m_Chunks);

			other.m_Chunks.clear();
			other.m_ChunkCapacity = 0;
			other.m_ChunksCount = 0;
			other.m_CreatedElements = 0;
			return *this;
		}

		inline T& operator[](uint64_t index) noexcept
		{
			uint64_t chunkIndex = index / m_ChunkCapacity;
			uint64_t elementIndex = index - chunkIndex * m_ChunkCapacity;
			return m_Chunks[chunkIndex][elementIndex];
		}

		inline T& operator[](uint64_t index) const noexcept
		{
			uint64_t chunkIndex = index / m_ChunkCapacity;
			uint64_t elementIndex = index - chunkIndex * m_ChunkCapacity;
			return m_Chunks[chunkIndex][elementIndex];
		}

		inline T& operator()(uint64_t chunkIndex, uint64_t elementIndex)
		{
			return m_Chunks[chunkIndex][elementIndex];
		}

		inline uint64_t Capacity() const { return m_ChunksCount * m_ChunkCapacity; }
		inline uint64_t ChunkCount() const { return m_ChunksCount; }
		inline uint64_t ChunkCapacity() const { return m_ChunkCapacity; }
		inline uint64_t Size() const { return m_CreatedElements; }
		inline bool IsEmpty()const { return m_CreatedElements == 0; }

		template<typename... Args>
		T& EmplaceBack(Args&&... args)
		{
			if (m_ChunksCount == 0)AddChunk();
			Chunk& b = m_Chunks.back();

			m_CreatedElements += 1;

			if (b.IsFull())
			{
				Chunk& newChunk = AddChunk();
				return newChunk.EmplaceBack(std::forward<Args>(args)...);
			}

			return b.EmplaceBack(std::forward<Args>(args)...);
		}

		template<typename... Args>
		EmplaceBackData EmplaceBack_CR(Args&&... args)
		{
			if (m_ChunksCount == 0)AddChunk();
			Chunk& b = m_Chunks.back();

			m_CreatedElements += 1;

			if (b.IsFull())
			{
				Chunk& newChunk = AddChunk();
				return EmplaceBackData(m_ChunksCount - 1, 0, &newChunk.EmplaceBack(std::forward<Args>(args)...));
			}

			uint64_t elementIndex = b.Size();
			return EmplaceBackData(m_ChunksCount - 1, elementIndex, &b.EmplaceBack(std::forward<Args>(args)...));
		}

		inline bool RemoveSwapBack(uint64_t index)
		{
			uint64_t chunkIndex = index / m_ChunkCapacity;
			uint64_t elementIndex = index % m_ChunkCapacity;

			return RemoveSwapBack(chunkIndex, elementIndex);
		}

		bool RemoveSwapBack(uint64_t chunkIndex, uint64_t elementIndex)
		{
			auto& fromChunk = m_Chunks[chunkIndex];
			if (chunkIndex == (m_ChunksCount - 1))
			{
				fromChunk.RemoveSwapBack(elementIndex);

				if (fromChunk.IsEmpty())
					PopBackChunk();

				m_CreatedElements -= 1;
				return true;
			}
			m_CreatedElements -= 1;
			Chunk& lastChunk = m_Chunks.back();

			auto& oldElement = fromChunk[elementIndex];
			auto& newElement = lastChunk.Back();

			oldElement = newElement;

			lastChunk.PopBack();


			if (lastChunk.IsEmpty())
				PopBackChunk();
			return true;
		}

		void Clear()
		{
			while (m_ChunksCount > 1)
			{
				auto& lastChunk = m_Chunks.back();
				m_ChunksCount -= 1;
				Chunk::Destroy(lastChunk);
				m_Chunks.pop_back();
			}

			auto& chunk = m_Chunks.back();
			chunk.Clear();

			m_CreatedElements = 0;
		}

		T& Back()
		{
			return m_Chunks.back().Back();
		}

		void PopBack()
		{
			if (m_CreatedElements > 0)
			{
				auto& lastChunk = m_Chunks.back();
				lastChunk.PopBack();

				m_CreatedElements -= 1;

				if (lastChunk.IsEmpty())
					PopBackChunk();
			}
		}

		inline bool IsPositionValid(uint64_t chunkIndex, uint64_t elementIndex)
		{
			return chunkIndex < m_ChunksCount && elementIndex < m_Chunks[chunkIndex].Size();
		}

		inline T* GetChunk(uint64_t index) const { return m_Chunks[index].m_Data; }

		inline uint64_t GetChunkSize(uint64_t index)const { return m_Chunks[index].m_Size; }

	private:
		std::vector<Chunk> m_Chunks;
		uint64_t m_ChunkCapacity = 100;
		uint64_t m_ChunksCount = 0;

		uint64_t m_CreatedElements = 0;

	private:

		Chunk& AddChunk()
		{
			m_ChunksCount += 1;
			auto& newChunk = m_Chunks.emplace_back();
			Chunk::Create(newChunk, m_ChunkCapacity);
			return newChunk;
		}

		void PopBackChunk()
		{
			if (m_ChunksCount <= 1) return;

			auto& lastChunk = m_Chunks.back();
			Chunk::Destroy(lastChunk);
			m_Chunks.pop_back();
			m_ChunksCount -= 1;

			ShrinkChunksVector();
		}

		void ShrinkChunksVector()
		{
			constexpr float packFactor = 0.5f;

			float currentPackFactor = (float)m_Chunks.size() / (float)m_Chunks.capacity();
			if (currentPackFactor <= packFactor)
			{
				m_Chunks.shrink_to_fit();
			}
		}
	};
}