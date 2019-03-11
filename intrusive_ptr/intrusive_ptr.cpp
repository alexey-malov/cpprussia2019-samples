// intrusive_ptr.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

namespace detail
{

class RefCountedBase
{
public:
	virtual ~RefCountedBase() = default;

	void AddRef() const
	{
		++m_refCount;
	}

	bool Release() const
	{
		if (--m_refCount == 0)
		{
			return true;
		}
		return false;
	}

private:
	mutable int m_refCount = 1;
};

} // namespace detail

template <typename T>
class RefCounted : public RefCountedBase
{

};

int main()
{
	std::cout << "Hello World!\n";
}
