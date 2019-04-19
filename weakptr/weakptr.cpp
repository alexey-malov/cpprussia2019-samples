// weakptr.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

using namespace std;

struct IOwner
{
	virtual ~IOwner() = default;
	virtual void Invalidate() = 0;
};

struct Child
{
	Child(int value, weak_ptr<IOwner> owner)
		: m_owner(move(owner))
		, m_value(value)
	{
	}

	int GetValue() const
	{
		return m_value;
	}

	void SetValue(int value)
	{
		if (value != m_value)
		{
			m_value = value;
			if (auto owner = m_owner.lock())
			{
				owner->Invalidate();
			}
		}
	}

private:
	weak_ptr<IOwner> m_owner;
	int m_value;
};

struct Owner : IOwner
	, enable_shared_from_this<Owner>
{
	shared_ptr<Child> AddChild(int val)
	{
		m_subordinates.emplace_back(make_shared<Child>(val, weak_from_this()));
		m_value.reset();
		return m_subordinates.back();
	}

	int GetValue() const
	{
		if (!m_value)
		{
			int sum = 0;
			for (auto&& s : m_subordinates)
			{
				sum += s->GetValue();
			}
			m_value = sum;
		}
		return *m_value;
	}

private:
	void Invalidate() override
	{
		m_value.reset();
	}
	mutable optional<int> m_value;
	vector<shared_ptr<Child>> m_subordinates;
};

int main()
{
	auto pint = make_shared<int>(42);
	auto pint1 = pint;
	weak_ptr wp = pint;
	auto owner = make_shared<Owner>();
	auto s1 = owner->AddChild(3);
	auto s2 = owner->AddChild(2);

	cout << "Old value: " << owner->GetValue() << "\n";

	s2->SetValue(42);
	cout << "New value:" << owner->GetValue() << "\n";

	owner.reset();

	s1->SetValue(10); // The owner won't be invalidated
}
