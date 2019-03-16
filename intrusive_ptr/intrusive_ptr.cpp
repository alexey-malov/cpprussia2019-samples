// intrusive_ptr.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "RefCounted.h"

class Outer;

class Innermost : public DetachableRefCounted
{
public:
	~Innermost()
	{
		std::cout << "~Innermost\n";
	}
};

class Inner : public DetachableRefCounted
{
public:
	Inner(Outer* outer)
		: m_outer(outer)
	{
		m_innermost = std::make_unique<Innermost>();
		m_innermost->SetOwner(*this);
	}

	boost::intrusive_ptr<Innermost> GetInnermost() const
	{
		return m_innermost.get();
	}

	boost::intrusive_ptr<Outer> GetOuter() const;

	~Inner()
	{
		std::cout << "~Inner\n";
	}

private:
	Outer* m_outer;
	std::unique_ptr<Innermost> m_innermost;
};

class Outer : public DetachableRefCounted
{
public:
	Outer()
	{
		m_inner = std::make_unique<Inner>(this);
		m_inner->SetOwner(*this);
	}

	boost::intrusive_ptr<Inner> GetInner() const
	{
		return m_inner.get();
	}

	void AddExtraInner(Inner* inner)
	{
		m_extraInner.reset(inner);
		m_extraInner->SetOwner(*this);
	}

	void RemoveInner()
	{
		m_inner.release()->DetachFromOwner();
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

/*
int main()
{
	boost::intrusive_ptr<Outer> outer(new Outer());
	auto inner = outer->GetInner();

	//boost::intrusive_ptr<Inner> extraInner(new Inner(nullptr));
	//outer->AddExtraInner(extraInner.get());

	//extraInner.reset();
	auto innermost = inner->GetInnermost();
	outer->RemoveInner();
	outer.reset();
	inner.reset();
	innermost.reset();

}
*/