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

	void InternalAddRef() const
	{
		++m_refCount;
	}

	[[nodiscard]] bool InternalRelease() const
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

class Detachable
{
public:
	void SetParent(Detachable* parent)
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
	virtual ~Detachable()
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
	void InternalDestroy() const
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

	void InternalRemoveChild(Detachable* child)
	{
		m_children.erase(child);
		DeleteSelfIfNeeded();
	}

	void InternalAddChild(Detachable const* child)
	{
		m_children.insert(child);
	}

	bool InternalTryDeleteIfNotEqualTo(const Detachable* detachable) const
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

	mutable std::unordered_set<const Detachable*> m_children;
	Detachable* m_parent = nullptr;
};

} // namespace detail

template <typename T>
class RefCounted : public detail::RefCountedBase
{
public:
	virtual void AddRef() const
	{
		InternalAddRef();
	}

	virtual void Release() const
	{
		if (InternalRelease())
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
	, public detail::Detachable
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
