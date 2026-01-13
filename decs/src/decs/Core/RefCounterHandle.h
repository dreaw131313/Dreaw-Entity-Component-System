#pragma once

#include "Core.h"

#include <atomic>

namespace decs
{
	class RefCountedObject
	{
		template<typename TObject>
			requires std::derived_from<TObject, RefCountedObject>
		friend struct TRefCountHandle;

	public:
		RefCountedObject() = default;

		RefCountedObject(const RefCountedObject&) = delete;

		RefCountedObject(RefCountedObject&& other) noexcept
		{

		}

		virtual ~RefCountedObject() = default;

		RefCountedObject& operator =(const RefCountedObject& other) = delete;

		RefCountedObject& operator =(RefCountedObject&& other) noexcept
		{
			return *this;
		}

	private:
		std::atomic<uint64_t> m_RefCounter{ 0 };
	};

	template<typename TObject>
		requires std::derived_from<TObject, RefCountedObject>
	struct TRefCountHandle
	{
		static_assert(std::derived_from<TObject, RefCountedObject>, "TObject must derive from RefCountedObject");

		template<typename TObject>
			requires std::derived_from<TObject, RefCountedObject>
		friend struct TRefCountHandle;

	public:
		TRefCountHandle() = default;

		TRefCountHandle(TObject* entityData):
			m_Object(entityData)
		{
			IncrementRefCount();
		}

		TRefCountHandle(const TRefCountHandle& other):
			m_Object(other.m_Object)
		{
			IncrementRefCount();
		}

		TRefCountHandle(TRefCountHandle&& other) noexcept:
			m_Object(other.m_Object)
		{
			other.m_Object = nullptr;
		}

		~TRefCountHandle()
		{
			DecrementRefCount();
		}

		TRefCountHandle& operator = (const TRefCountHandle& other)
		{
			if (&other != this)
			{
				OnCopy(other);
			}
			return *this;
		}

		TRefCountHandle& operator=(TRefCountHandle&& other) noexcept
		{
			if (&other != this)
			{
				DecrementRefCount();

				m_Object = other.m_Object;
				other.m_Object = nullptr;
			}
			return *this;
		}

		bool operator==(const TRefCountHandle& rhs)const
		{
			return this->m_Object == rhs.m_Object;
		}

		bool operator!=(const TRefCountHandle& rhs) const noexcept
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

		inline operator bool() const noexcept
		{
			return IsValid();
		}

		inline TObject* Get() const noexcept
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
		inline static TRefCountHandle<TObject> Make(TArgs&&...args)
		{
			return TRefCountHandle<TObject>(new TObject(std::forward<TArgs>(args)...));
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
			}
			m_Object = nullptr;
		}

		void OnCopy(const TRefCountHandle& other)
		{
			DecrementRefCount();
			m_Object = other.m_Object;
			IncrementRefCount();
		}

	};

	template<typename TObject, typename...TArgs>
	inline TRefCountHandle<TObject> MakeRefCounted(TArgs&&...args)
	{
		return TRefCountHandle<TObject>(new TObject(std::forward<TArgs>(args)...));
	}
}

template<typename T>
struct std::hash<::decs::TRefCountHandle<T>>
{
	std::size_t operator()(const ::decs::TRefCountHandle<T>& ref) const
	{
		return std::hash<T*>{}(ref.Get());
	}
};