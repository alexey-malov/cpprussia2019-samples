#include "pch.h"
#include "RefCounted.h"

using boost::intrusive_ptr;

namespace
{

struct Obj : DetachableRefCounted
{
	using Destructor = std::function<void()>;
	Obj(Destructor d = {})
		: destructor(d)
	{
	}

	void RemoveChild(size_t index)
	{
		auto it = children.begin() + index;
		it->release()->DetachFromOwner();
		children.erase(it);
	}

	void AddChild(Obj* obj)
	{
		children.emplace_back(obj);
		children.back()->SetOwner(*this);
	}

	~Obj()
	{
		children.clear();
		if (destructor)
			destructor();
	}

private:
	std::vector<std::unique_ptr<Obj>> children;
	Destructor destructor;
};

std::function<void()> Increment(int& i)
{
	return [&i] { ++i; };
}

intrusive_ptr<Obj> MakeObj(int& destructionCounter)
{
	return { new Obj(Increment(destructionCounter)) };
}

} // namespace

TEST_CASE("DetachableRefCounted is destroyed when its ref count is 0")
{
	int destructionCount = 0;

	auto testable = MakeObj(destructionCount);

	auto testableCopy = testable;
	CHECK(destructionCount == 0);

	testable.reset();
	CHECK(destructionCount == 0);

	testableCopy.reset();
	CHECK(destructionCount == 1);
}

TEST_CASE("Object attachment")
{
	GIVEN("Inner object within outer object")
	{
		int boxDeaths = 0;
		auto box = MakeObj(boxDeaths);
		int thingDeaths = 0;
		auto thing = MakeObj(thingDeaths);

		box->AddChild(thing.get());

		WHEN("there are no outer refs")
		{
			box.reset();
			THEN("outer lives while there are inner object refs")
			{
				CHECK(boxDeaths == 0);
				CHECK(thingDeaths == 0);

				auto thingCopy = thing;

				thing.reset();
				CHECK(boxDeaths == 0);
				CHECK(thingDeaths == 0);

				thingCopy.reset();
				CHECK(boxDeaths == 1);
				CHECK(thingDeaths == 1);
			}
		}

		WHEN("there are not inner refs")
		{
			thing.reset();
			THEN("inner object lives while outer is alive")
			{
				CHECK(boxDeaths == 0);
				CHECK(thingDeaths == 0);

				box.reset();
				CHECK(boxDeaths == 1);
				CHECK(thingDeaths == 1);
			}
		}

		WHEN("removed child object can be attached to another object")
		{
			int otherBoxDeaths = 0;
			auto otherBox = MakeObj(otherBoxDeaths);

			box->RemoveChild(0);

			otherBox->AddChild(thing.get());

			THEN("the old owner dies when there are no direct links to it")
			{
				CHECK(boxDeaths == 0);
				box.reset();
				CHECK(boxDeaths == 1);
				CHECK(thingDeaths == 0);
			}
		}
	}
}

struct Foo : RefCounted
{
	void DoSomething()
	{
	}
};

RefCountPtr<Obj> MakeRC(int& deathCounter)
{
	return { new Obj(Increment(deathCounter)) };
}

TEST_CASE("RefCountPtr is released on destruction")
{
	int cnt = 0;
	{
		auto p1 = MakeRC(cnt);
		CHECK(cnt == 0);
	}
	CHECK(cnt == 1);
}

SCENARIO("RefCountPtr assignment")
{
	int cnt1 = 0;
	int cnt2 = 0;

	GIVEN("two distinct pointers")
	{
		auto p1 = MakeRC(cnt1);
		auto p2 = MakeRC(cnt2);
		WHEN("one pointer is assigned to another")
		{
			p1 = p2;
			THEN("old ptr is released")
			{
				CHECK(cnt1 == 1);
				CHECK(cnt2 == 0);
			}
			AND_THEN("pointers become equal")
			{
				CHECK(p1 == p2);
			}
		}
	}
}

SCENARIO("RefCountPtr move assignment")
{
	GIVEN("two distinct pointers")
	{
		int cnt1 = 0;
		int cnt2 = 0;

		auto p1 = MakeRC(cnt1);
		auto p2 = MakeRC(cnt2);
		WHEN("one pointer is moved to another")
		{
			p1 = std::move(p2);
			THEN("old ptr is released")
			{
				CHECK(cnt1 == 1);
				CHECK(cnt2 == 0);
			}
			AND_THEN("right pointer becomes empty")
			{
				CHECK(!p2);
			}
			AND_THEN("left pointer is not empty")
			{
				CHECK(p1);
			}
		}
	}

	GIVEN("two equal pointers")
	{
		int cnt = 0;
		{
			auto p1 = MakeRC(cnt);
			auto p2 = p1;
			WHEN("one pointer is moved to another")
			{
				p1 = std::move(p2);
				THEN("rhs pointer becomes empty")
				{
					CHECK(!p2);
				}
			}
			CHECK(cnt == 0);
		}
		CHECK(cnt == 1);
	}

	GIVEN("a pointer")
	{
		int cnt = 0;
		auto p = MakeRC(cnt);
		WHEN("object is moved to itself")
		{
			p = std::move(p);
			THEN("it is not affected")
			{
				CHECK(p);
				CHECK(cnt == 0);
			}
		}
	}
}
