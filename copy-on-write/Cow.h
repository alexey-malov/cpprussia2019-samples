#pragma once

namespace detail
{

template <typename U>
struct CopyConstr
{
	static auto Copy(U const& other)
	{
		return std::make_shared<U>(other);
	}
};

template <typename U>
struct CloneConstr
{
	static auto Copy(U const& other)
	{
		return other.Clone();
	}
};

} // namespace detail

template <typename T, typename CopyClass = typename std::conditional_t<!std::is_abstract_v<T> && std::is_copy_constructible_v<T>, detail::CopyConstr<T>, detail::CloneConstr<T>>>
class Cow
{
	template <typename U, typename V>
	friend class Cow;

public:
	class WriteProxy
	{
	public:
		T* operator->()
		{
			return m_p;
		}

	private:
		friend class Cow;
		WriteProxy(WriteProxy const&) = default;
		WriteProxy& operator=(WriteProxy const&) = default;

		WriteProxy(T* p)
			: m_p(p)
		{
		}

		T* m_p;
	};

	template <typename... Args, typename = std::enable_if_t<!std::is_abstract_v<T>>>
	Cow(Args&&... args)
		: m_shared(std::make_shared<T>(std::forward<Args>(args)...))
	{
	}

	Cow(Cow<T, CopyClass>&& rhs)
		: m_shared(std::move(rhs.m_shared))
	{
	}

	template <typename U, typename Deleter>
	Cow(std::unique_ptr<U, Deleter> pUniqueObj)
		: m_shared(std::move(pUniqueObj))
	{
	}


	template <typename U, typename V>
	Cow(Cow<U, V>& rhs)
		: m_shared(rhs.m_shared)
	{
	}

	Cow(Cow const& rhs) = default;

	template <typename U, typename V>
	Cow& operator=(Cow<U, V>& rhs)
	{
		m_shared = rhs.m_shared;
		return *this;
	}
	Cow& operator=(Cow&& rhs) = default;

	template <typename U, typename V>
	Cow& operator=(Cow<U, V> const& rhs)
	{
		m_shared = rhs.m_shared;
		return *this;
	}

	T const& operator*() const
	{
		assert(m_shared);
		return *m_shared;
	}

	T const* operator->() const
	{
		assert(m_shared);
		return m_shared.get();
	}

	WriteProxy operator--(int)
	{
		assert(m_shared);
		EnsureUnique();
		return WriteProxy(m_shared.get());
	}

	T& Write()
	{
		assert(m_shared);
		EnsureUnique();
		return *m_shared;
	}

private:
	void EnsureUnique()
	{
		if (m_shared.use_count() > 1)
		{
			m_shared = CopyClass::Copy(*m_shared);
		}
	}

private:
	std::shared_ptr<T> m_shared;
};
