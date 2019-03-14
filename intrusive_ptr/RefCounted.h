#pragma once

namespace detail
{

class RefCounter
{
public:
	RefCounter(const RefCounter&) = delete;
	RefCounter& operator=(const RefCounter&) = delete;

	RefCounter() = default;

	void IncrementBy(int delta) const
	{
		m_refCount += delta;
	}

	[[nodiscard]] bool DecrementBy(int delta) const
	{
		if ((m_refCount -= delta) == 0)
		{
			return true;
		}
		return false;
	}

	int GetCount() const
	{
		return m_refCount;
	}

private:
	mutable int m_refCount = 0;
};

class RefCountedBase
{
public:
	virtual void AddRef() const = 0;

	virtual void Release() const = 0;

	RefCountedBase(const RefCountedBase&) = delete;
	RefCountedBase& operator=(const RefCountedBase&) = delete;

protected:
	RefCountedBase() = default;

	virtual ~RefCountedBase() = default;
};

void intrusive_ptr_add_ref(const RefCountedBase* p)
{
	p->AddRef();
}

void intrusive_ptr_release(const RefCountedBase* p)
{
	p->Release();
}

} // namespace detail

class RefCounted : public detail::RefCountedBase
{
public:
	void AddRef() const final
	{
		m_counter.IncrementBy(1);
	}

	void Release() const final
	{
		if (m_counter.DecrementBy(1))
		{
			delete this;
		}
	}

private:
	detail::RefCounter m_counter;
};

class DetachableRefCounted : public detail::RefCountedBase
{
public:
	void AddRef() const final
	{
		AddRefs(1);
	}

	void Release() const final
	{
		ReleaseRefs(1);
	}

	void SetParent(DetachableRefCounted* outer)
	{
		assert(!m_outer && outer);
		m_outer = outer;
		m_outer->AddRefs(m_counter.GetCount());
	}

	void DetachFromParent()
	{
		if (m_outer)
		{
			m_outer->ReleaseRefs(m_counter.GetCount());
			m_outer = nullptr;
			ReleaseRefs(0);
		}
	}

private:
	void OuterAddRef(int refCount) const
	{
		if (m_outer)
		{
			m_outer->AddRefs(refCount);
		}
	}

	bool OuterRelease(int refCount) const
	{
		if (m_outer)
		{
			m_outer->ReleaseRefs(refCount);
			return false;
		}
		return true;
	}

	void AddRefs(int refCount) const
	{
		OuterAddRef(refCount);
		m_counter.IncrementBy(refCount);
	}

	void ReleaseRefs(int refCount) const
	{
		if (m_counter.DecrementBy(refCount))
		{
			if (OuterRelease(refCount))
			{
				delete this;
			}
		}
		else
		{
			OuterRelease(refCount);
		}
	}

private:
	DetachableRefCounted* m_outer = nullptr;
	detail::RefCounter m_counter;
};
