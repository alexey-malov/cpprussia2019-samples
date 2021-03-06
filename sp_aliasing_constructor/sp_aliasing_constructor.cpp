// sp_aliasing_constructor.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "UniversalPtr.h"
#include "UniversalPtrBenchmark.h"

using namespace std;

//struct Outer;

struct Inner
{
	friend struct Outer;
	Inner(string name, weak_ptr<Outer> outer)
		: m_name(move(name))
		, m_outer(move(outer))
	{
	}
	Inner(const Inner&) = delete;
	Inner& operator=(const Inner&) = delete;
	~Inner()
	{
		cout << "~Internal()\n";
	}

	string_view GetName() const
	{
		return m_name;
	}

	shared_ptr<Outer> GetOuter() const
	{
		return m_outer.lock();
	}

private:
	string m_name;
	weak_ptr<Outer> m_outer;
};

struct Outer : enable_shared_from_this<Outer>
{
	Outer() = default;
	Outer(const Outer&) = delete;
	Outer& operator=(const Outer&) = delete;

	~Outer()
	{
		cout << "~External()\n";
	}

	shared_ptr<Inner> CreateNewPart(string name)
	{
		m_parts.emplace_back(make_unique<Inner>(move(name), shared_from_this()));
		return GetPart(m_parts.size() - 1);
	}

	shared_ptr<Inner> GetPart(size_t index) const
	{
		auto part = m_parts.at(index).get();
		return shared_ptr<Inner>(shared_from_this(), part);
	}

	void RemovePart(size_t index)
	{
		m_removedParts.emplace_back(std::move(m_parts.at(index)));
		m_removedParts.back()->m_outer.reset();
		m_parts.erase(m_parts.begin() + index);
	}

	vector<unique_ptr<Inner>> m_parts;
	vector<unique_ptr<Inner>> m_removedParts;
};

template <typename T>
shared_ptr<T> MakeUnowningSP(T& value)
{
	return { shared_ptr<T>(), &value };
}

struct Bar
{
};

void Foo(shared_ptr<Bar> bar)
{

}

template <typename Ptr>
void BenchmarkPtr(Ptr p)
{
	auto start = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < 1'000'000'000; ++i)
	{
		p = TestPtr(move(p));
	}
	auto duration = std::chrono::high_resolution_clock::now() - start;
	std::cout << "  Result:" << *p << ", duration: " << chrono::duration_cast<chrono::milliseconds>(duration).count() << "ms\n";
}

void BenchmarkUniversalPtr()
{
	{
		cout << "Universal ptr(raw)\n";
		unsigned v = 0;
		BenchmarkPtr((UniversalPtr<unsigned>(&v)));
	}
	{
		cout << "Universal ptr(shared)\n";
		BenchmarkPtr(UniversalPtr<unsigned>{ make_shared<unsigned>(0) });
	}
	{
		cout << "Raw ptr\n";
		unsigned v = 0;
		BenchmarkPtr(&v);
	}
	{
		cout << "Unique ptr\n";
		BenchmarkPtr(make_unique<unsigned>(0));
	}
	{
		cout << "Shared ptr\n";
		BenchmarkPtr(make_shared<unsigned>(0));
	}

}


int main()
{
	{
		BenchmarkUniversalPtr();
	}

	{
		Bar bar;
		auto sp = MakeUnowningSP(bar);
		weak_ptr weakBar(sp);
		assert(weakBar.expired());
		assert(!weakBar.lock());
		Foo(MakeUnowningSP(bar));
	}

	auto external = make_shared<Outer>();
	auto part = external->CreateNewPart("part 1");
	assert(part);
	assert(external->GetPart(0) == part);
	weak_ptr weakPart{ part };
	part.reset();
	assert(!weakPart.expired());

	part = external->GetPart(0);
	assert(part);
	assert(weakPart.lock() == part);

	external.reset();
	external = part->GetOuter();
	assert(external);

	external->RemovePart(0);
	assert(part->GetName() == "part 1");
	assert(!part->GetOuter());
	part.reset();
	assert(!weakPart.expired());
}
