// intrusive_ptr.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "RefCounted.h"
//#include <atlcom.h>

class Outer;

class Inner : public DetachableRefCounted<Inner>
{
public:
	Inner(Outer* outer)
		: m_outer(outer)
	{
	}

	boost::intrusive_ptr<Outer> GetOuter() const;

	~Inner()
	{
		std::cout << "~Inner\n";
	}

private:
	Outer* m_outer;
};

class Outer : public DetachableRefCounted<Outer>
{
public:
	Outer()
	{
		auto inner = new Inner(this);
		inner->SetParent(this);
		m_inner = inner;
	}

	boost::intrusive_ptr<Inner> GetInner() const
	{
		return m_inner;
	}

	void RemoveInner()
	{
		m_inner->DetachFromParent();
	}

	~Outer()
	{
		std::cout << "~Outer\n";
	}

private:
	Inner* m_inner = nullptr;
};

boost::intrusive_ptr<Outer> Inner::GetOuter() const
{
	return m_outer;
}

int main()
{
	boost::intrusive_ptr<Outer> outer(new Outer());
	auto inner = outer->GetInner();
	outer.reset();
	outer = inner->GetOuter();
	//outer->RemoveInner();
	outer.reset();
	inner.reset();
	std::cout << "Hello World!\n";
}
