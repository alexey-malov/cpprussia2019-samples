#pragma once

//#include <atlcom.h>

class DetachableRefCounted;

namespace detail
{

class RefCounter final
{
public:
	RefCounter(const RefCounter&) = delete;
	RefCounter& operator=(const RefCounter&) = delete;

	RefCounter() = default;

	void IncrementBy(int delta) noexcept
	{
		m_refCount += delta;
	}

	[[nodiscard]] bool DecrementBy(int delta) noexcept
	{
		if ((m_refCount -= delta) == 0)
		{
			return true;
		}
		return false;
	}

	int GetCount() const noexcept
	{
		return m_refCount;
	}

private:
	int m_refCount = 0;
};

class RefCountedBase
{
public:
	virtual void AddRef() const noexcept = 0;

	virtual void Release() const noexcept = 0;

	RefCountedBase(const RefCountedBase&) = delete;
	RefCountedBase& operator=(const RefCountedBase&) = delete;

protected:
	RefCountedBase() = default;
	virtual ~RefCountedBase() {}
};

inline void intrusive_ptr_add_ref(const RefCountedBase* p) noexcept
{
	p->AddRef();
}

inline void intrusive_ptr_release(const RefCountedBase* p) noexcept
{
	p->Release();
}

template <typename T>
class NoAddRefRelease : public T
{
	void AddRef() const noexcept override = 0;
	void Release() const noexcept override = 0;

public:
	void SetOwner(DetachableRefCounted& owner) = delete;
	void DetachFromOwner() noexcept = delete;
};

} // namespace detail

class RefCounted : public detail::RefCountedBase
{
public:
	void AddRef() const noexcept override
	{
		m_counter.IncrementBy(1);
	}

	void Release() const noexcept override
	{
		if (m_counter.DecrementBy(1))
		{
			delete this;
		}
	}

private:
	mutable detail::RefCounter m_counter;
};

class DetachableRefCounted : public detail::RefCountedBase
{
public:
	void AddRef() const noexcept override
	{
		AddRefs(1);
	}

	void Release() const noexcept override
	{
		ReleaseRefs(1);
	}

	void SetOwner(DetachableRefCounted& owner)
	{
		if (m_owner)
		{
			throw std::logic_error("already owned");
		}
		m_owner = &owner;
		m_owner->AddRefs(m_counter.GetCount());
	}

	void DetachFromOwner() noexcept
	{
		if (m_owner)
		{
			m_owner->ReleaseRefs(m_counter.GetCount());
			m_owner = nullptr;
			if (m_counter.GetCount() == 0)
			{
				delete this;
			}
		}
	}

private:
	void AddRefsToOwner(int refCount) const noexcept
	{
		if (m_owner)
		{
			m_owner->AddRefs(refCount);
		}
	}

	bool ReleaseOwnerRefs(int refCount) const noexcept
	{
		if (m_owner)
		{
			m_owner->ReleaseRefs(refCount);
			return false;
		}
		return true;
	}

	void AddRefs(int refCount) const noexcept
	{
		m_counter.IncrementBy(refCount);
		AddRefsToOwner(refCount);
	}

	void ReleaseRefs(int refCount) const noexcept
	{
		bool noSelfRefs = m_counter.DecrementBy(refCount);
		bool isSelfOwned = ReleaseOwnerRefs(refCount);
		if (noSelfRefs && isSelfOwned)
		{
			delete this;
		}
	}

	DetachableRefCounted* m_owner = nullptr;
	mutable detail::RefCounter m_counter;
};

template <typename T>
class RefCountPtr final
{
public:
	RefCountPtr(T* p = nullptr) noexcept
		: m_ptr(p)
	{
		if (m_ptr)
		{
			m_ptr->AddRef();
		}
	}

	RefCountPtr(const RefCountPtr& other) noexcept
		: m_ptr(other.m_ptr)
	{
		if (m_ptr)
		{
			m_ptr->AddRef();
		}
	}

	RefCountPtr(RefCountPtr&& other) noexcept
		: m_ptr(other.m_ptr)
	{
		other.m_ptr = nullptr;
	}

	RefCountPtr& operator=(const RefCountPtr& other) noexcept
	{
		if (m_ptr != other.m_ptr)
		{
			RefCountPtr(other).swap(*this);
		}
		return *this;
	}

	RefCountPtr& operator=(RefCountPtr&& other) noexcept
	{
		if (this != std::addressof(other))
		{
			RefCountPtr(std::move(other)).swap(*this);
		}
		return *this;
	}

	~RefCountPtr() noexcept
	{
		if (m_ptr)
			m_ptr->Release();
	}

	void swap(RefCountPtr& other) noexcept
	{
		std::swap(m_ptr, other.m_ptr);
	}

	detail::NoAddRefRelease<T>& operator*() const noexcept
	{
		return static_cast<detail::NoAddRefRelease<T>&>(*m_ptr);
	}

	detail::NoAddRefRelease<T>* operator->() const noexcept
	{
		return static_cast<detail::NoAddRefRelease<T>*>(m_ptr);
	}

	T* Get() const noexcept // для любителей экстрима
	{
		return m_ptr;
	}

	bool operator!() const noexcept
	{
		return m_ptr == nullptr;
	}

	bool operator==(const RefCountPtr& other) const noexcept
	{
		return m_ptr == other.m_ptr;
	}

	bool operator!=(const RefCountPtr& other) const noexcept
	{
		return m_ptr != other.m_ptr;
	}

	explicit operator bool() const noexcept
	{
		return m_ptr != nullptr;
	}

	void Reset() noexcept
	{
		if (m_ptr)
		{
			m_ptr->Release();
			m_ptr = nullptr;
		}
	}

private:
	T* m_ptr;
};
