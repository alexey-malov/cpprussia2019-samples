#pragma once
#include <memory>

template <typename T>
struct UniversalPtr
{
	constexpr UniversalPtr() noexcept = default;

	constexpr UniversalPtr(nullptr_t) noexcept
	{
	}

	template <typename U,
		typename = std::enable_if_t<std::is_convertible<U&, T&>::value>>
	UniversalPtr(U& ref)
		: m_ptr(std::shared_ptr<T>(), std::addressof(ref))
	{
	}

	template <typename U,
		typename = std::enable_if_t<std::is_convertible<U*, T*>::value>>
	UniversalPtr(U* ptr)
		: m_ptr(std::shared_ptr<T>(), ptr)
	{
	}

	template <typename U, typename D,
		typename = std::enable_if_t<std::is_convertible<typename std::unique_ptr<U, D>::pointer, T*>::value>>
	UniversalPtr(std::unique_ptr<U, D>&& unique)
		: m_ptr(std::move(unique))
	{
	}

	template <typename U,
		typename = std::enable_if_t<std::is_convertible<U*, T*>::value>>
	UniversalPtr(const std::shared_ptr<U>& shared) noexcept
		: m_ptr(shared)
	{
	}

	explicit operator bool() const noexcept { return m_ptr.get() != nullptr; }

	T& operator*() const noexcept { return *m_ptr; }
	T* operator->() const noexcept { return m_ptr.get(); }
	T* get() const noexcept { return m_ptr.get(); }

private:
	std::shared_ptr<T> m_ptr;
};

template <typename T, typename U>
bool operator==(const UniversalPtr<T>& t, const UniversalPtr<U>& u) noexcept
{
	return t.get() == u.get();
}

template <typename T>
bool operator==(const UniversalPtr<T>& t, nullptr_t) noexcept
{
	return !t.get();
}

template <typename T>
bool operator==(nullptr_t, const UniversalPtr<T>& t) noexcept
{
	return !t.get();
}

template <typename T, typename U>
bool operator!=(const UniversalPtr<T>& t, const UniversalPtr<U>& u) noexcept
{
	return !(t == u);
}

template <typename T>
bool operator!=(const UniversalPtr<T>& t, nullptr_t) noexcept
{
	return !(t == nullptr);
}

template <typename T>
bool operator!=(nullptr_t, const UniversalPtr<T>& t) noexcept
{
	return !(t == nullptr);
}
