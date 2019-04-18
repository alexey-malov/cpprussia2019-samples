// make_shared_with_private_ctor.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

using namespace std;

struct Outer;

struct Inner
{
	Inner(weak_ptr<Outer> outer)
		: m_outer(move(outer))
	{
	}

private:
	weak_ptr<Outer> m_outer;
};

struct Outer
	: public enable_shared_from_this<Outer>
{
	static shared_ptr<Outer> Create(int arg)
	{
		struct Wrapper : Outer
		{
			Wrapper(int val)
				: Outer(val)
			{
			}
		};
		auto foo = make_shared<Wrapper>(arg); // one allocation
		foo->Init();
		return foo;
	}

	static shared_ptr<Outer> CreateBad(int arg)
	{
		shared_ptr<Outer> foo(new Outer(arg)); // 2 allocations
		foo->Init();
		return foo;
	}

private:
	void Init()
	{
		auto self = shared_from_this();
		m_inner = make_shared<Inner>(self);
	}

	explicit Outer(int val)
		: m_value(val)
	{
	}
	int m_value;
	shared_ptr<Inner> m_inner;
};

struct Foo : enable_shared_from_this<Foo>
{
	static shared_ptr<Foo> Create(int arg)
	{
		shared_ptr<Foo> foo{ new Foo(arg) };
		foo->Init();
		return foo;
	}

private:
	void Init()
	{
		auto self = shared_from_this();
	}
	explicit Foo(int arg)
	{

	}
};

int main()
{
	auto foo = Outer::Create(42);
	return 0;
}
