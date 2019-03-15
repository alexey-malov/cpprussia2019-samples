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
		assert(!m_parent && outer);
		m_parent = outer;
		m_parent->AddRefs(m_counter.GetCount());
	}

	void DetachFromParent()
	{
		if (m_parent)
		{
			m_parent->ReleaseRefs(m_counter.GetCount());
			m_parent = nullptr;
			ReleaseRefs(0);
		}
	}

private:
	void AddParentRef(int refCount) const
	{
		if (m_parent)
		{
			m_parent->AddRefs(refCount);
		}
	}

	bool ReleaseParent(int refCount) const
	{
		if (m_parent)
		{
			m_parent->ReleaseRefs(refCount);
			return false;
		}
		return true;
	}

	void AddRefs(int refCount) const
	{
		m_counter.IncrementBy(refCount);
		AddParentRef(refCount);
	}

	void ReleaseRefs(int refCount) const
	{
		if (m_counter.DecrementBy(refCount))
		{
			if (ReleaseParent(refCount))
			{
				delete this;
			}
		}
		else
		{
			ReleaseParent(refCount);
		}
	}

	DetachableRefCounted* m_parent = nullptr;
	detail::RefCounter m_counter;
};
