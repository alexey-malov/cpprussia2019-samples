// intrusive_ptr.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "RefCounted.h"
//#include <atlcom.h>

class Outer;

class Inner : public DetachableRefCounted
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

class Outer : public DetachableRefCounted
{
public:
	Outer()
	{
		m_inner = std::make_unique<Inner>(this);
		m_inner->SetParent(this);
	}

	boost::intrusive_ptr<Inner> GetInner() const
	{
		return m_inner.get();
	}

	void AddExtraInner(Inner* inner)
	{
		m_extraInner.reset(inner);
		m_extraInner->SetParent(this);
	}

	void RemoveInner()
	{
		m_inner.release()->DetachFromParent();
	}

	~Outer()
	{
		std::cout << "~Outer\n";
	}

private:
	std::unique_ptr<Inner> m_inner;
	std::unique_ptr<Inner> m_extraInner;
};

boost::intrusive_ptr<Outer> Inner::GetOuter() const
{
	return m_outer;
}

int main()
{
	boost::intrusive_ptr<Outer> outer(new Outer());
	auto inner = outer->GetInner();

	boost::intrusive_ptr<Inner> extraInner(new Inner(nullptr));
	outer->AddExtraInner(extraInner.get());

	extraInner.reset();
	outer.reset();
	//inner.reset();
	//outer = inner->GetOuter();
	//outer->RemoveInner();
	inner.reset();
	//outer.reset();
}
