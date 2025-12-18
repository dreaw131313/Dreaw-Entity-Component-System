#pragma once

#include "Core.h"

#include <atomic>

namespace decs
{
	class RefCountedObject
	{
		template<typename>
		friend struct TRefCounterHandle;

	public:
		RefCountedObject() = default;

		RefCountedObject(const RefCountedObject& other)
		{

		}

		RefCountedObject(RefCountedObject&& other) noexcept
		{

		}

		virtual ~RefCountedObject() = default;

		RefCountedObject& operator =(const RefCountedObject& other)
		{
			return *this;
		}

		RefCountedObject& operator =(RefCountedObject&& other) noexcept
		{
			return *this;
		}

	private:
		std::atomic<uint64_t> m_RefCounter{ 0 };
	};

	template<typename TObject>
	struct TRefCounterHandle
	{
		static_assert(std::derived_from<TObject, RefCountedObject>, "TObject must derive from RefCountedObject");

	public:
		TRefCounterHandle() = default;

		TRefCounterHandle(TObject* entityData):
			m_Object(entityData)
		{
			IncrementRefCount();
		}

		TRefCounterHandle(const TRefCounterHandle& other)
		{
			OnCopy(other);
		}

		TRefCounterHandle(TRefCounterHandle&& other) noexcept
		{
			OnMove(std::move(other));
		}

		~TRefCounterHandle()
		{
			DecrementRefCount();
		}

		TRefCounterHandle& operator = (const TRefCounterHandle& other)
		{
			if (&other != this)
			{
				OnCopy(other);
			}
			return *this;
		}

		TRefCounterHandle& operator=(TRefCounterHandle&& other) noexcept
		{
			if (&other != this)
			{
				OnMove(std::move(other));
			}
			return *this;
		}

		bool operator==(const TRefCounterHandle& rhs)const
		{
			return this->m_Object == rhs.m_Object;
		}

		bool operator!=(const TRefCounterHandle& rhs) const noexcept
		{
			return m_Object != rhs.m_Object;
		}

		TObject* operator->() const noexcept
		{
			return m_Object;
		}

		TObject& operator*() const noexcept
		{
			return *m_Object;
		}

		inline TObject* Get() const
		{
			return m_Object;
		}

		inline bool IsValid() const
		{
			return m_Object != nullptr;
		}

		void Reset()
		{
			DecrementRefCount();
		}

		template<typename...TArgs>
		inline static TRefCounterHandle<TObject> Make(TArgs&&...args)
		{
			return TRefCounterHandle<TObject>(new TObject(std::forward<TArgs>(args)...));
		}

	private:
		TObject* m_Object = nullptr;

	private:
		void IncrementRefCount()
		{
			RefCountedObject* refCountedObject = m_Object;
			if (refCountedObject != nullptr)
			{
				refCountedObject->m_RefCounter.fetch_add(1ull, std::memory_order_relaxed);
			}
		}

		void DecrementRefCount()
		{
			RefCountedObject* refCountedObject = m_Object;
			if (refCountedObject != nullptr && refCountedObject->m_RefCounter.fetch_sub(1ull, std::memory_order_acq_rel) == 1)
			{
				delete m_Object;
				m_Object = nullptr;
			}
		}

		void OnMove(TRefCounterHandle&& other)
		{
			DecrementRefCount();
			m_Object = other.m_Object;
			other.m_Object = nullptr;
		}

		void OnCopy(const TRefCounterHandle& other)
		{
			DecrementRefCount();
			m_Object = other.m_Object;
			IncrementRefCount();
		}

	};
}