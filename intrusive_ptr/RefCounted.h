#pragma once

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

	virtual ~RefCountedBase() = default;
};

inline void intrusive_ptr_add_ref(const RefCountedBase* p) noexcept
{
	p->AddRef();
}

inline void intrusive_ptr_release(const RefCountedBase* p) noexcept
{
	p->Release();
}

} // namespace detail

class RefCounted : public detail::RefCountedBase
{
public:
	void AddRef() const noexcept final
	{
		m_counter.IncrementBy(1);
	}

	void Release() const noexcept final
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
	void AddRef() const noexcept final
	{
		AddRefs(1);
	}

	void Release() const noexcept final
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
