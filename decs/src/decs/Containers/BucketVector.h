#pragma once
#include "decspch.h"


namespace decs
{
	template<typename T>
	class BucketVector
	{
	private:
		// Bucket class
		class Bucket final
		{
		public:
			Bucket()
			{

			}

			~Bucket()
			{

			}

			static void Create(Bucket& bucket, const uint64_t& capacity = 100)
			{
				if (capacity == 0) bucket.m_Capacity = 1;
				else bucket.m_Capacity = capacity;

				bucket.m_Size = 0;

				bucket.m_Data = (T*) ::operator new(bucket.m_Capacity * sizeof(T));
			}

			static void Destroy(Bucket& bucket)
			{
				for (uint64_t idx = 0; idx < bucket.m_Size; idx++)
				{
					bucket.m_Data[idx].~T();
				}

				::operator delete(bucket.m_Data, bucket.m_Capacity * sizeof(T));
			}

		public:
			inline bool IsFull() { return m_Size == m_Capacity; }
			inline bool IsEmpty() { return m_Size == 0; }
			inline uint64_t Size() { return m_Size; }
			inline uint64_t Capacity() { return m_Capacity; }
			inline T* Data() { return m_Data; }

			template<typename... Args>
			T& EmplaceBack(Args&&... args)
			{
				T* value = new(&m_Data[m_Size])T(std::forward<Args>(args)...);
				m_Size += 1;
				return *value;
			}

			bool RemoveSwapBack(const uint64_t& index)
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

			inline T& At(const uint64_t& index) { return m_Data[index]; }

			inline T& operator[](const uint64_t& index)
			{
				if (index > m_Size) throw std::out_of_range("Index out of range in one of bucket of BucketVector!");
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

		// Bucket class end

	public:
		struct OperationData final
		{
		public:
			uint64_t BucketIndex;
			uint64_t ElementIndex;
			T* Data;
		public:
			OperationData(
				const uint64_t& bucketIndex,
				const uint64_t& elementIndex,
				T* data) :
				BucketIndex(bucketIndex),
				ElementIndex(elementIndex),
				Data(data)
			{

			}
			~OperationData() {}
		};

		using EmplaceBackData = OperationData;
		using SwapBackData = OperationData;

	public:
		BucketVector()
		{
			AddBucket();
		}

		BucketVector(uint64_t bucketCapacity) :
			m_BucketCapacity(bucketCapacity)
		{
			AddBucket();
		}

		~BucketVector()
		{
			for (auto& bucket : m_Buckets)
				Bucket::Destroy(bucket);
		}


		// Operators

		inline T& operator[](const uint64_t& index)
		{
			uint64_t bucketIndex = index / m_BucketCapacity;
			uint64_t elementIndex = index % m_BucketCapacity;
			return m_Buckets[bucketIndex][elementIndex];
		}

		inline T& operator()(const uint64_t& bucketIndex, const uint64_t& elementIndex)
		{
			return m_Buckets[bucketIndex][elementIndex];
		}

		// Operators - end

		inline uint64_t Capacity() const { return m_BucketsCount * m_BucketCapacity; }
		inline uint64_t BucketsCount() const { return m_BucketsCount; }
		inline uint64_t BucketCapacity() const { return m_BucketCapacity; }

		inline uint64_t Size() const { return m_CreatedElements; }
		inline bool IsEmpty()const { return m_CreatedElements == 0; }


		template<typename... Args>
		T& EmplaceBack(Args&&... args)
		{
			Bucket& b = m_Buckets.back();

			m_CreatedElements += 1;

			if (b.IsFull())
			{
				Bucket& newBucket = AddBucket();
				return newBucket.EmplaceBack(std::forward<Args>(args)...);
			}

			return b.EmplaceBack(std::forward<Args>(args)...);
		}

		template<typename... Args>
		EmplaceBackData EmplaceBack_CR(Args&&... args)
		{
			Bucket& b = m_Buckets.back();

			m_CreatedElements += 1;

			if (b.IsFull())
			{
				Bucket& newBucket = AddBucket();
				return EmplaceBackData(m_BucketsCount - 1, 0, &newBucket.EmplaceBack(std::forward<Args>(args)...));
			}

			uint64_t elementIndex = b.Size();
			return EmplaceBackData(m_BucketsCount - 1, elementIndex, &b.EmplaceBack(std::forward<Args>(args)...));
		}

		bool RemoveSwapBack(const uint64_t& index)
		{
			uint64_t bucketIndex = index / m_BucketCapacity;
			uint64_t elementIndex = index % m_BucketCapacity;

			return RemoveSwapBack(bucketIndex, elementIndex);
		}

		bool RemoveSwapBack(const uint64_t& bucketIndex, const uint64_t& elementIndex)
		{
			auto& fromBucket = m_Buckets[bucketIndex];
			if (bucketIndex == (m_BucketsCount - 1))
			{
				fromBucket.RemoveSwapBack(elementIndex);

				if (fromBucket.IsEmpty())
					PopBackBucket();

				m_CreatedElements -= 1;
				return true;
			}
			m_CreatedElements -= 1;
			Bucket& lastBucket = m_Buckets.back();

			auto& oldElement = fromBucket[elementIndex];
			auto& newElement = lastBucket.Back();

			oldElement = newElement;

			lastBucket.PopBack();


			if (lastBucket.IsEmpty())
				PopBackBucket();
			return true;
		}

		void Clear()
		{
			while (m_BucketsCount > 1)
			{
				auto& lastBucket = m_Buckets.back();
				m_BucketsCount -= 1;
				Bucket::Destroy(lastBucket);
				m_Buckets.pop_back();
			}

			auto& bucket = m_Buckets.back();
			bucket.Clear();

			m_CreatedElements = 0;
		}

		T& Back()
		{
			return m_Buckets.back().Back();
		}

		void PopBack()
		{
			auto& lastBucket = m_Buckets.back();
			lastBucket.PopBack();

			m_CreatedElements -= 1;

			if (lastBucket.IsEmpty())
				PopBackBucket();
		}

		inline bool IsPositionValid(const uint64_t& bucketIndex, const uint64_t& elementIndex)
		{
			return bucketIndex < m_BucketsCount&& elementIndex < m_Buckets[bucketIndex].Size();
		}

		inline T* GetBucket(const uint64_t& index) const { return m_Buckets[index].m_Data; }
		inline uint64_t GetBucketSize(const uint64_t& index)const { return m_Buckets[index].m_Size; }
	private:
		std::vector<Bucket> m_Buckets;
		uint64_t m_BucketCapacity = 100;
		uint64_t m_BucketsCount = 0;

		uint64_t m_CreatedElements = 0;

	private:

		Bucket& AddBucket()
		{
			m_BucketsCount += 1;
			auto& newBucket = m_Buckets.emplace_back();
			Bucket::Create(newBucket, m_BucketCapacity);
			return newBucket;
		}

		void PopBackBucket()
		{
			if (m_BucketsCount <= 1) return;

			auto& lastBucket = m_Buckets.back();
			Bucket::Destroy(lastBucket);
			m_Buckets.pop_back();
			m_BucketsCount -= 1;

			ShrinkBucketsVector();
		}

		void ShrinkBucketsVector()
		{
			constexpr float packFactor = 0.5f;

			float currentPackFactor = (float)m_Buckets.size() / (float)m_Buckets.capacity();
			if (currentPackFactor <= packFactor)
			{
				m_Buckets.shrink_to_fit();
			}
		}
	};
}