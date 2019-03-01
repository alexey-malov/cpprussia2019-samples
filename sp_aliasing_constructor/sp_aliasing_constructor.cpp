// sp_aliasing_constructor.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

using namespace std;

struct External;

struct Internal
{
	Internal(string name, weak_ptr<External> owner)
		: m_name(move(name))
		, m_owner(move(owner))
	{
	}
	~Internal()
	{
		cout << "~Internal()\n";
	}

	string_view GetName() const
	{
		return m_name;
	}

	shared_ptr<External> GetOwner() const
	{
		return m_owner.lock();
	}

private:
	string m_name;
	weak_ptr<External> m_owner;
};

struct External : enable_shared_from_this<External>
{
	~External()
	{
		cout << "~External()\n";
	}
	shared_ptr<Internal> GetPart(size_t index) const
	{
		auto part = m_parts.at(index).get();
		return shared_ptr<Internal>(shared_from_this(), part);
	}

	shared_ptr<Internal> CreateNewPart(string name)
	{
		auto self = shared_from_this();
		auto part = make_unique<Internal>(move(name), self);
		m_parts.emplace_back(move(part));
		return GetPart(m_parts.size() - 1);
	}

	vector<unique_ptr<Internal>> m_parts;
};

int main()
{
	auto external = make_shared<External>();
	auto part = external->CreateNewPart("part 1");
	external.reset();
	external = part->GetOwner();
	assert(external);
}
