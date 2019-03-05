// make_shared_with_private_ctor.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

using namespace std;

struct Foo;

struct Inner
{
	Inner(weak_ptr<Foo> outer)
		: m_outer(move(outer))
	{
	}

private:
	weak_ptr<Foo> m_outer;
};

struct Foo
	: public enable_shared_from_this<Foo>
{
	static shared_ptr<Foo> Create(int arg)
	{
		struct Wrapper : Foo
		{
			Wrapper(int val)
				: Foo(val)
			{
			}
		};
		auto foo = make_shared<Wrapper>(arg); // one allocation
		foo->Init();
		return foo;
	}

	static shared_ptr<Foo> CreateBad(int arg)
	{
		shared_ptr<Foo> foo(new Foo(arg)); // 2 allocations
		foo->Init();
		return foo;
	}

private:
	void Init()
	{
		auto self = shared_from_this();
		m_inner = make_shared<Inner>(self);
	}

	explicit Foo(int val)
		: m_value(val)
	{
	}
	int m_value;
	shared_ptr<Inner> m_inner;
};

int main()
{
	auto foo = Foo::Create(42);
	return 0;
}
