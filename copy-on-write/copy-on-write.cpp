// copy-on-write.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "pch.h"
#include "Cow.h"

struct Base
{
	virtual ~Base() = default;
	virtual std::shared_ptr<Base> Clone() const = 0;
	virtual void SetValue(int value) = 0;
	virtual int GetValue() const = 0;
};

struct Derived : Base
{
	void SetValue(int value) override
	{
		m_val = value;
	}

	int GetValue() const override
	{
		return m_val;
	}

	std::shared_ptr<Base> Clone() const override
	{
		return std::make_shared<Derived>(*this);
	}

private:
	int m_val = 0;
};

int main()
{
	using namespace std;
	{
		Cow<vector<int>> v1(100000u);
		auto v2 = v1;
		v2--->push_back(42);
	}

	{
		Cow<vector<int>> v(100000u, 42);
		Cow<Base> base(make_unique<Derived>());
	}

	{
		Cow<vector<int>> v;
		cout << v->size(); // read
		v--->push_back(42); // write
	}
}
