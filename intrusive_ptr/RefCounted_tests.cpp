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
