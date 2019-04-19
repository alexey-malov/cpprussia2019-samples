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

class Subject
{
public:
	int GetValue() const;
	void SetValue(int x);

private:
	int m_x = 0;
};

int Subject::GetValue() const
{
	return m_x;
}

void Subject::SetValue(int x)
{
	m_x = x;
}

using namespace std;

struct Shape
{
	virtual ~Shape() = default;
	virtual shared_ptr<Shape> Clone() const = 0;
};

struct Circle : public Shape
{
	Circle(double r)
		: m_radius(r)
	{
	}

	shared_ptr<Shape> Clone() const override
	{
		return make_shared<Circle>(*this);
	}

private:
	double m_radius;
};

int main()
{
	using namespace std;
	{
		Cow<Circle> circle(100.0);
		Cow<Shape> shape1{ circle };
		Cow<Shape> shape2{ shape1 };
	}
	{
		Cow<Subject> subj1;
		std::cout << subj1->GetValue();
	}
	{
		Cow<Subject> subj1;
		subj1.Write().SetValue(1);
	}
	{
		Cow<Subject> subj1;
		subj1--->SetValue(1);

		auto subj2 = subj1;
		subj2--->SetValue(2);

		assert(subj1->GetValue() == 1);
		assert(subj2->GetValue() == 2);
	}
	{
		Cow<vector<int>> v;
		v.Write().push_back(100);
		auto vCopy = v;
	}

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
