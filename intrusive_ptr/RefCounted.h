#pragma once

namespace detail
{

class RefCountedBase
{
public:
	RefCountedBase(const RefCountedBase&) = delete;
	RefCountedBase& operator=(const RefCountedBase&) = delete;

protected:
	RefCountedBase() = default;
	virtual ~RefCountedBase() = default;

	void AddRef() const
	{
		++m_refCount;
	}

	[[nodiscard]] bool Release() const
	{
		if (--m_refCount == 0)
		{
			return true;
		}
		return false;
	}

	bool RefCountIsZero() const
	{
		return m_refCount == 0;
	}

private:
	mutable int m_refCount = 0;
};

class IDetachable
{
public:
	virtual ~IDetachable() = default;
	virtual bool InternalTryDeleteIfNotEqualTo(const IDetachable* detachable) const = 0;
};

class IDetachableParent : public IDetachable
{
public:
	virtual void InternalDestroy() const = 0;
	virtual void InternalAddChild(const IDetachable* child) = 0;
	virtual void InternalRemoveChild(IDetachable* child) = 0;
};

class DetachableParentImpl : protected IDetachableParent
{
public:
	void SetParent(IDetachableParent* parent)
	{
		assert(!m_parent && parent);
		m_parent = parent;
		m_parent->InternalAddChild(this);
	}

	void DetachFromParent()
	{
		if (m_parent)
		{
			m_parent->InternalRemoveChild(this);
			m_parent = nullptr;
			DeleteSelfIfNeeded();
		}
	}

protected:
	~DetachableParentImpl()
	{
		for (auto* child : m_children)
		{
			delete child;
		}
	}

	void DeleteSelfIfNeeded() const
	{
		if (InternalTryDeleteIfNotEqualTo(this))
		{
			InternalDestroy();
		}
	}

private:
	void InternalDestroy() const override
	{
		if (m_parent)
		{
			m_parent->InternalDestroy();
		}
		else
		{
			delete this;
		}
	}

	void InternalRemoveChild(IDetachable* child) override
	{
		m_children.erase(child);
		DeleteSelfIfNeeded();
	}

	void InternalAddChild(IDetachable const* child) override
	{
		m_children.insert(child);
	}

	bool InternalTryDeleteIfNotEqualTo(const IDetachable* detachable) const override
	{
		if (!InternalRefCountIsZero())
		{
			return false;
		}
		if (m_parent && m_parent != detachable && !m_parent->InternalTryDeleteIfNotEqualTo(this))
		{
			return false;
		}
		for (auto& child : m_children)
		{
			if (child != detachable && !child->InternalTryDeleteIfNotEqualTo(this))
			{
				return false;
			}
		}
		return true;
	}

	virtual bool InternalRefCountIsZero() const = 0;

	mutable std::unordered_set<const IDetachable*> m_children;
	IDetachableParent* m_parent = nullptr;
};

} // namespace detail

template <typename T>
class RefCounted : public detail::RefCountedBase
{
public:
	virtual void AddRef() const
	{
		RefCountedBase::AddRef();
	}

	virtual void Release() const
	{
		if (RefCountedBase::Release())
		{
			OnFinalRelease();
		}
	}

	RefCounted(const RefCounted&) = delete;
	RefCounted& operator=(const RefCounted&) = delete;

protected:
	RefCounted() = default;
	virtual ~RefCounted() = default;

private:
	virtual void OnFinalRelease() const
	{
		delete this;
	}
};

template <typename T>
void intrusive_ptr_add_ref(const RefCounted<T>* p)
{
	p->AddRef();
}

template <typename T>
void intrusive_ptr_release(const RefCounted<T>* p)
{
	p->Release();
}

template <typename T>
class DetachableRefCounted
	: public RefCounted<T>
	, public detail::DetachableParentImpl
{
private:
	void OnFinalRelease() const override
	{
		DeleteSelfIfNeeded();
	}

	bool InternalRefCountIsZero() const override
	{
		return this->RefCountIsZero();
	}
};